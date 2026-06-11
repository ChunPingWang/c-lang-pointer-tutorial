# 07 — 常見地雷

> 寫指標的 90% bug 集中在這幾類。每個都附對應的 GPS 專案案例，看完之後請回頭審視你自己的程式碼。

---

## 7.1 野指標 (uninitialized pointer)

```c
int *p;
*p = 10;     // p 內容是隨機垃圾, 寫入會炸
```

`p` 沒被初始化，內含的「位址」是 stack 上的殘留位元，解參考結果不可預期（多半 segfault，最糟是踩到別的記憶體沒當下崩潰，bug 延後爆發更難 debug）。

**規則**：宣告就初始化。沒目標就先設 `NULL`。

```c
int *p = NULL;
```

---

## 7.2 懸空指標 (dangling pointer)

```c
char *make_field(void)
{
    char buf[32];
    strcpy(buf, "GPRMC");
    return buf;        // ❌ buf 是 stack 變數, 函式回傳後就消失
}
```

呼叫端拿到的指標指向已經被回收的 stack 區域，下一個函式呼叫就會覆蓋它。

**正確做法（嵌入式常用）**：呼叫端提供 buffer：

```c
void make_field(char *out, size_t cap) { /* 寫進 out */ }
```

GPS 專案的 `nmea_parser_parse_sentence` 就是這個模式 — 呼叫端給 `gps_fix_t *out_fix`，parser 寫進去。

---

## 7.3 `sizeof(指標) ≠ sizeof(陣列)`

```c
void print_size(int arr[10]) {
    printf("%zu\n", sizeof(arr));   // 印 8 (指標大小), 不是 40
}

int data[10];
printf("%zu\n", sizeof(data));      // 40
print_size(data);                   // 8
```

陣列傳入函式 **退化成指標**，sizeof 就只剩指標本身的大小。

**正確做法**：把長度當另一個參數傳：

```c
void print_size(int *arr, size_t n);
```

`ring_buffer_init(rb, storage, sizeof(storage))` 就是這個模式 — 呼叫端在還是陣列時量好長度，傳進去。

---

## 7.4 越界寫入 (buffer overflow)

```c
char buf[16];
strcpy(buf, "$GPRMC,123519,A,...");   // 超過 16 → 寫到 buf 之後的記憶體
```

C 不會幫你檢查邊界。要嘛 segfault，要嘛默默踩到別的變數，更糟是埋下安全漏洞。

**正確做法**：用 `strncpy` 並手動補 `\0`，或用 `snprintf`。

`src/gps/nmea_parser.c`：

```c
char working[NMEA_MAX_SENTENCE_LEN];
strncpy(working, sentence + 1, sizeof(working) - 1);
working[sizeof(working) - 1] = '\0';
```

每次都檢查長度。

---

## 7.5 字串字面值不可寫

```c
char *p = "hello";
p[0] = 'H';            // 未定義行為 (UB)
```

`"hello"` 在唯讀資料段，改它的後果是 segfault（多數平台）或損毀程式映像（少數）。

**正確做法**：

```c
const char *p = "hello";        // 表明唯讀, 編譯器擋你
char buf[] = "hello";           // 拷貝到可寫陣列
```

---

## 7.6 把結構體當二進位封包傳

```c
gps_fix_t fix;
HAL_UART_Transmit(uart, (uint8_t *)&fix, sizeof(fix), 100);
```

問題：

1. **padding** 隨編譯器/平台不同 → 接收端可能對不齊。
2. **endianness** 不同 (ARM 與 PC 都是 little-endian 但有些 MCU 是 big-endian)。
3. **結構體欄位順序變動** → 升級就壞。

**正確做法**：明確序列化，例如自己組 NMEA 字串、用 protobuf / cbor、或用 `#pragma pack` 並文件化封包格式。

---

## 7.7 釋放後使用 (use-after-free)

雖然 GPS 範例都用 stack，但動態配置時要特別小心：

```c
char *buf = malloc(32);
strcpy(buf, "GPRMC");
free(buf);
printf("%s\n", buf);    // ❌ buf 已釋放
```

**規則**：`free` 完馬上 `buf = NULL;`，下次誤用會 NULL deref 直接崩 → 至少容易 debug。

---

## 7.8 重複釋放 (double free)

```c
free(buf);
free(buf);        // 第二次 free 是 UB
```

跟 use-after-free 同源 → `free` 後設 `NULL`，再 `free(NULL)` 是合法 no-op。

---

## 7.9 type-punning 與 strict aliasing

```c
uint32_t x = 0x12345678;
uint8_t *bytes = (uint8_t *)&x;        // 合法: char/uint8_t 可以別名任何型別

float f = 3.14f;
int   i = *(int *)&f;                  // ⚠️ 嚴格別名規則下是 UB
```

C 規範允許用 `char *` / `uint8_t *` 看任何物件的位元，但 **不允許** 用不相關型別的指標互看（例外：`union`、`memcpy`）。

**正確做法**：

```c
int i;
memcpy(&i, &f, sizeof(i));    // 安全
```

---

## 7.10 對齊 (alignment)

```c
uint8_t buf[16];
uint32_t *p = (uint32_t *)(buf + 1);    // 位址未對齊到 4 bytes
*p = 0xCAFEBABE;                          // ARM 上可能 BusFault
```

x86 容忍未對齊存取（慢但能跑），ARM Cortex-M 在某些設定下會直接 fault。

**正確做法**：`memcpy(p, &value, sizeof(value));`

---

## 7.11 ISR 共享資料缺 volatile / atomic

```c
size_t head;
void ISR(void) { head++; }
while (head == tail) { ... }
```

編譯器可能把 `head == tail` 變成只讀一次的 cache 值，主迴圈永遠看不到 ISR 的更新。

**正確做法**：第 6 章詳述 — `volatile`，必要時加記憶體屏障或 atomic。

---

## 7.12 NULL pointer deref 的防線

API 邊界 (公開函式) 通常該檢查 NULL；內部 helper 函式則依賴呼叫端契約即可。

過度檢查反而讓程式變慢、隱藏 bug。GPS 專案的選擇：

```c
// 公開 API: 檢查
nmea_status_t nmea_parser_parse_sentence(const char *sentence, gps_fix_t *out_fix) {
    if (sentence == NULL || out_fix == NULL) return NMEA_ERR_BAD_FIELD;
    ...
}

// 內部 static: 假設參數有效
static double parse_coord(const char *raw, char hemisphere) {
    if (raw == NULL || *raw == '\0') return 0.0;  // 業務層的空字串檢查, 不是 NULL safety
    ...
}
```

---

## 7.13 自我檢查清單

寫完一段使用指標的程式碼，請過一遍：

- [ ] 每個指標宣告時都初始化或設 NULL
- [ ] 傳入的指標都有檢查 NULL（公開 API）
- [ ] `sizeof(陣列)` 沒有寫在函式參數那邊
- [ ] 用 `strncpy` / `snprintf` 而非 `strcpy` / `sprintf`
- [ ] 字串字面值都標 `const char *`
- [ ] 動態配置的記憶體有對應 `free`，且 `free` 後設 NULL
- [ ] ISR 共享變數加 `volatile`
- [ ] 沒有把結構體當二進位封包直接傳

---

## 完課

如果這 7 章你都讀完並動手改過範例，恭喜，你已經跨過 C 語言最大的門檻。
建議下一步：

1. 把 `stm32/` 對照版骨架抓出來實際燒一塊板子 (見 [stm32/README.md](../stm32/README.md))。
2. 自己擴充 parser：支援 `$GPVTG`（速度向量）或 `$GPGSV`（衛星詳細）。
3. 試著把 `ring_buffer` 換成 lock-free 版本（單生產者單消費者場景 STM32 上很常用）。

> 後續學習指引：[exercises-solutions/solutions-07.md](exercises-solutions/solutions-07.md)

回到 → [README.md](../README.md)
