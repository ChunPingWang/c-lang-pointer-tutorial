# 06 — `const` 與 `volatile`

> 本章對應範例：[`examples/06_const_volatile/main.c`](../examples/06_const_volatile/main.c)
> 對應「真實」用法：`ring_buffer_t` 裡 `volatile size_t head/tail`、`const char *` 參數遍佈於 parser

---

## 6.1 `const` 的四種位置一張表記住

```c
const T *p     // (a) 指向 const T          ← 不能透過 p 改 *p
T const *p     // (b) 同 (a)，只是寫法不同
T *const p     // (c) const pointer to T    ← p 自己不能換指向
const T *const // (d) 兩者都不可改
```

讀法口訣：**從右往左唸**，例如 `const char *p` → 「p 是 pointer to const char」。

---

## 6.2 在 GPS 程式裡的實際應用

```c
nmea_status_t nmea_parser_parse_sentence(const char *sentence, gps_fix_t *out_fix);
```

- `const char *sentence`：parser 保證 **不會** 寫進你給的字串（呼叫端可以安心傳唯讀資料）。
- `gps_fix_t *out_fix`：parser 會 **寫進去**，所以沒有 const。

這就是 C API 的「契約」 — 看簽名就知道誰是 in、誰是 out。

---

## 6.3 為什麼 `const` 重要？

1. **編譯期擋錯**：寫錯了直接編譯失敗，比執行期 segfault 好太多。
2. **讀程式更快**：看 const 就知道「這條路徑只有讀」，理解函式不用再追蹤資料流。
3. **更高效**：編譯器看到 `const` 可以做更多最佳化（例如把唯讀資料放 ROM）。

> 嵌入式情境特別重要：唯讀資料可以由 linker 放到 flash 而非 SRAM，省掉昂貴的 RAM。

---

## 6.4 `volatile`：告訴編譯器「別自作聰明」

`volatile` 的意思：「這塊記憶體的值**可能在程式碼之外被改動**」。
編譯器看到 `volatile` 就**不會**：

- 把連續讀取最佳化成只讀一次
- 把寫入合併或捨棄
- 重新排列存取順序

兩個經典場景：

### (1) 中斷服務常式 (ISR) 改的變數

```c
volatile bool data_ready = false;

void HAL_UART_RxCpltCallback(...) {     // ISR
    data_ready = true;
}

void main_loop(void) {
    while (!data_ready) {                // 主迴圈
        // 沒 volatile 的話, 編譯器可能優化成 while(1) 永遠不退出
    }
}
```

### (2) 硬體暫存器 (MMIO)

```c
#define GPIOA_ODR (*(volatile uint32_t *)0x40020014)

GPIOA_ODR |= (1 << 5);    // 點亮 GPIO PA5
```

- `0x40020014` 是 GPIO 暫存器的硬體位址（不是普通 SRAM）。
- 寫進去會觸發實體電路動作。
- 不加 `volatile`，編譯器可能把多次寫入合併成一次 → bug。

---

## 6.5 `ring_buffer_t` 為什麼要 `volatile`？

```c
typedef struct {
    uint8_t       *buffer;
    size_t         capacity;
    volatile size_t head;
    volatile size_t tail;
} ring_buffer_t;
```

在 STM32 上：

- `head` 由 UART 中斷推進（ISR push）
- `tail` 由主迴圈推進（main pop）

兩個執行流程同時碰到同一個變數。若沒有 `volatile`：

```c
while (rb->head == rb->tail) { }   // ISR 改了 head, 主迴圈卻沒看到 → 死迴圈
```

> 注意：`volatile` **只擋編譯器最佳化**，並不保證原子性或多核同步。
> 多核或更嚴格的場景要用 atomic / memory barrier，但 STM32 單核情境下 volatile 通常夠。

---

## 6.6 `volatile` 與指標的四種組合

```c
volatile T *p       // p 指向 volatile T (常見: 指向暫存器或共享變數)
T volatile *p       // 同上
T *volatile p       // p 自己是 volatile (很少用)
volatile T *volatile p // 兩者都 volatile
```

最常用的是第一種。例如硬體暫存器陣列：

```c
volatile uint32_t * const reg_ptr = (volatile uint32_t *)0x40020014;
//                ^^^^^^ 不能換指向 (位址是固定的)
//  ^^^^^^^^                              透過 reg_ptr 讀寫實際會碰硬體
```

跑範例：

```bash
./build/examples/ex_06_const_volatile
```

```
24 47 50 52 4D 43        <-- "$GPRMC" 的 hex dump
讀暫存器: 0x12345678
寫入後再讀: 0xCAFEBABE
```

---

## 6.7 常見錯誤

| 想表達                       | 寫法                            | 易混淆寫法                   |
| ---------------------------- | ------------------------------- | ---------------------------- |
| 「我不會改你的字串」         | `const char *s`                 | `char const *s` (一樣)       |
| 「指標固定指向這個常數位址」 | `volatile uint32_t *const r`    | `volatile uint32_t * const`  |
| 「ISR 共享變數」             | `volatile size_t head`          | `volatile size_t *head` (錯)|

---

## 6.8 動手練習

1. 在 `ring_buffer_t` 拿掉 `volatile`，然後想像在 STM32 上會出什麼問題（host 上跑不出來，但你能不能解釋？）。
2. 把 `nmea_parser_parse_sentence` 的 `const char *` 改成 `char *`，看編譯器抱怨什麼。
3. 試著寫一個 `mmio_set_bit(volatile uint32_t *reg, int bit)`，把 `(1 << bit)` set 進去。

---

> 練習解答：[exercises-solutions/solutions-06.md](exercises-solutions/solutions-06.md)

## 下一章

→ [07 — 常見地雷](07-pitfalls.md)
