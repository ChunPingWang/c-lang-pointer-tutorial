# 03 — 結構體與指標

> 本章對應範例：[`examples/03_struct_pointer/main.c`](../examples/03_struct_pointer/main.c)
> 對應「真實」用法：`gps_fix_t *` 在 `nmea_parser_parse_sentence`、`on_fix` 回呼之間流動

---

## 3.1 結構體：把相關資料綁在一起

`include/gps/gps_types.h`：

```c
typedef struct {
    double   latitude;
    double   longitude;
    double   altitude_m;
    double   speed_knots;
    double   course_deg;
    gps_time_t time;
    uint8_t  satellites;
    gps_fix_quality_t fix_quality;
    bool     valid;
} gps_fix_t;
```

一筆 GPS 定位需要：經緯度、高度、速度、衛星數量、是否有效⋯⋯。
把它們全部塞進一個 `gps_fix_t`，傳遞起來方便也不容易漏。

---

## 3.2 兩種存取方式：`.` 和 `->`

```c
gps_fix_t  fix;
gps_fix_t *p = &fix;

fix.satellites = 8;     // 對「結構體實體」用 .
p->satellites  = 8;     // 對「結構體指標」用 ->
(*p).satellites = 8;    // 等同於 p->satellites，但太囉嗦
```

記憶口訣：**有箭頭就有指標，沒箭頭就是實體**。

---

## 3.3 為什麼要傳指標而不是傳值？

跑範例：

```bash
./build/examples/ex_03_struct_pointer
```

```
sizeof(gps_fix_t) = 56 bytes
[傳值] 函式內 sats=99
呼叫後 main 中 sats=8 (沒變)        <-- 修改丟失了
[傳指標] 函式內 sats=99 (透過 -> 存取)
呼叫後 main 中 sats=99 (被改了)     <-- 修改有效
```

兩個關鍵差別：

1. **能否影響外部**：傳值改不到呼叫端，傳指標可以。
2. **效能**：`gps_fix_t` 是 56 bytes。傳值會整個複製一份；傳指標只複製 8 bytes（64-bit 系統上）。
   對 STM32 這種 SRAM 寶貴的環境差異更明顯。

> 規則：**結構體幾乎一定傳指標**。
> 如果你不想被改，加 `const`：`void print_fix(const gps_fix_t *fix)`。

---

## 3.4 在 GPS parser 中的真實用法

`include/gps/nmea_parser.h`：

```c
nmea_status_t nmea_parser_parse_sentence(const char *sentence, gps_fix_t *out_fix);
```

兩個指標參數，意圖完全不同：

| 參數                       | `const` 與否        | 意圖                                |
| -------------------------- | ------------------- | ----------------------------------- |
| `const char *sentence`     | const 在 `char` 前  | 「我只讀，不改你的字串」            |
| `gps_fix_t *out_fix`       | 沒有 const          | 「我會把結果寫進你給我的這塊記憶體」|

呼叫端怎麼用？

```c
gps_fix_t fix = {0};
nmea_status_t st = nmea_parser_parse_sentence(line, &fix);
if (st == NMEA_OK) {
    printf("lat=%.4f\n", fix.latitude);
}
```

- `&fix` 把 fix 的地址傳進去 → parser 透過指標寫入欄位。
- 呼叫端收到 `fix`，欄位已經被填好了。

這個「out parameter」設計在 C 非常普遍 — 因為 C 函式只能 `return` 一個值，要回傳結構體又想避免複製，就用「給我一個指標，我幫你填」。

---

## 3.5 結構體裡放指標

`gps_fix_t` 沒有指標欄位（純資料），但 `nmea_parser_t` 有：

```c
typedef struct {
    char           sentence_buf[NMEA_MAX_SENTENCE_LEN];
    size_t         length;
    bool           collecting;
    gps_fix_t      latest_fix;
    nmea_fix_cb_t  on_fix;       // <-- 函式指標 (第 4 章詳談)
    void          *user_data;    // <-- 任意指標，給使用者塞 context
} nmea_parser_t;
```

`void *user_data` 是 C 表達「我不在乎這是什麼，使用者自己知道」的方式 — 後面在第 4 章會看到它怎麼用。

---

## 3.6 designated initializer（C99 之後可用）

```c
gps_fix_t fix = {
    .latitude   = 25.0334,
    .longitude  = 121.5645,
    .altitude_m = 12.5,
    .satellites = 8,
    .valid      = true
};
```

比起 `{25.0334, 121.5645, ...}` 順序記不清楚的寫法，這種寫法**直接寫欄位名**，加新欄位也不會壞掉。
**強烈建議養成習慣**。

---

## 3.7 結構體記憶體配置與 padding

執行範例看到 `sizeof(gps_fix_t) = 56 bytes`。手算：

```
double latitude    8
double longitude   8
double altitude_m  8
double speed_knots 8
double course_deg  8
gps_time_t time    ~6 (但會 padding 對齊)
uint8_t satellites 1
enum   fix_quality 4
bool   valid       1
------- 加上 padding 對齊到 8 bytes -------
total              56
```

編譯器會插「padding」讓欄位對齊。
**對 MCU 程式很重要的後果**：兩個看似一樣的結構體，**在不同編譯器/平台上 sizeof 可能不一樣** — 千萬不要直接把結構體當二進位封包透過 UART 傳！

第 7 章會再回到這個地雷。

---

## 3.8 動手練習

1. 在 `examples/03_struct_pointer/main.c` 加一個 `change_lat_by_value` 和 `change_lat_by_pointer`，驗證一樣結論。
2. 把 `gps_fix_t` 用 `printf` 印出每個欄位的位址（`&fix.latitude`、`&fix.satellites` 等），觀察排列。
3. 把 `gps_fix_t` 的欄位順序重排，看 sizeof 變化（嘗試把所有 `uint8_t` 放在一起 vs 散布）。

---

> 練習解答：[exercises-solutions/solutions-03.md](exercises-solutions/solutions-03.md)

## 下一章

→ [04 — 函式指標與回呼](04-function-pointer.md)
