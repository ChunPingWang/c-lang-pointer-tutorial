# 04 — 函式指標與回呼

> 本章對應範例：[`examples/04_function_pointer/main.c`](../examples/04_function_pointer/main.c)
> 對應「真實」用法：`nmea_parser_t::on_fix` 在每筆 GPS 定位完成時通知上層

---

## 4.1 函式名就是位址

C 裡函式名稱在運算式中會「退化」成「指向那段機器碼的指標」：

```c
int add(int a, int b) { return a + b; }

int (*fp)(int, int);   // 宣告一個「指向 (int,int)->int 函式」的指標
fp = add;              // 或寫 fp = &add，效果相同
int r = fp(2, 3);      // 透過指標呼叫，等同 add(2,3)
```

`int (*fp)(int, int)` 看起來嚇人，拆解：

```
        fp                 是個變數
       *fp                 解參考後是函式
      (*fp)(int,int)       函式接 (int, int)
int   (*fp)(int,int)       函式回傳 int
```

> 函式指標宣告語法是 C 裡最醜陋的地方之一。**用 `typedef` 包起來會好很多**。

---

## 4.2 用 typedef 包起來

`include/gps/nmea_parser.h`：

```c
typedef void (*nmea_fix_cb_t)(const gps_fix_t *fix, void *user_data);
```

讀法：「`nmea_fix_cb_t` 是一個型別，它代表 *指向接受 (const gps_fix_t *, void *) 並回傳 void 的函式* 的指標」。

之後拿來宣告變數就清爽多了：

```c
nmea_fix_cb_t on_fix;   // 比 void (*on_fix)(const gps_fix_t *, void *) 友善
```

---

## 4.3 為什麼 parser 要用函式指標？

`nmea_parser_t` 不會直接 `printf`，因為它不知道呼叫端想做什麼：

- 應用程式可能想印到 UART
- 測試程式可能想記一個計數
- 嵌入式系統可能想透過 LoRa 發送出去

於是 parser 留一個「掛勾」：解到完整一筆就呼叫你給的函式。
這就是經典的 **callback / 觀察者模式**，靠函式指標實作。

`src/gps/nmea_parser.c` 的關鍵程式碼：

```c
nmea_status_t st = nmea_parser_parse_sentence(p->sentence_buf, &p->latest_fix);
if (st == NMEA_OK && p->on_fix != NULL) {
    p->on_fix(&p->latest_fix, p->user_data);   // <-- 透過指標呼叫使用者的函式
}
```

`p->on_fix` 是個指標 → 後面接 `(...)` 就是「跳過去執行那段機器碼」。

---

## 4.4 `void *user_data`：傳一包不透明的 context

呼叫端要怎麼把自己的狀態帶進回呼？傳一個 `void *`：

```c
typedef struct { int fix_count; } app_state_t;

static void on_new_fix(const gps_fix_t *fix, void *user_data)
{
    app_state_t *state = (app_state_t *)user_data;   // 還原型別
    state->fix_count++;
    ...
}

int main(void) {
    app_state_t state = {0};
    nmea_parser_init(&parser, on_new_fix, &state);   // 把自己的 state 傳進去
    ...
}
```

`void *` = 「指向某個東西，但 C 不知道是什麼」，是 C 表達泛型的方式。
**回呼端要負責 cast 回正確型別**。如果 cast 錯了，C 不會幫你抓 — 完全是你的責任。

---

## 4.5 一份 parser，兩種 callback

跑範例：

```bash
./build/examples/ex_04_function_pointer
```

```
-- handler A: log to stdout --
[demo] lat=48.1173 lon=11.5167 sats=0
[demo] lat=48.1173 lon=11.5167 sats=8
-- handler B: just count --
count = 2
```

同一份 parser 程式碼，第一次餵進 `log_handler`，第二次餵進 `counter_handler`，行為完全不同 — 這就是函式指標的威力。

---

## 4.6 STM32 上常見的函式指標：HAL 中斷回呼

熟 STM32 的人會記得這類函式：

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // 你自己定義這個函式，HAL 在接收完成時呼叫
}
```

STM32 HAL 內部其實就是 **(一張函式指標表) + (你 override 的弱符號)**。
這份教學在 host 上沒用真實 HAL，但 `nmea_parser_t::on_fix` 設計就是仿造這種模式 — **在 MCU 上你會把同一個概念用在 ISR → 應用層的傳遞**。

---

## 4.7 動手練習

1. 寫一個 `csv_writer` callback，每收到 fix 就寫一行 CSV。
2. 在 `nmea_parser_t` 加一個 `on_error` callback，當 `nmea_parser_parse_sentence` 回非 OK 時呼叫。
3. 把 `nmea_fix_cb_t` 的 `void *` 換成具體型別 `app_state_t *`，思考會犧牲什麼。
   提示：parser 就會「知道」應用層型別 → 失去通用性，變成耦合。

---

> 練習解答：[exercises-solutions/solutions-04.md](exercises-solutions/solutions-04.md)

## 下一章

→ [05 — 雙重指標 `**`](05-double-pointer.md)
