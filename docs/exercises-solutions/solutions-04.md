# 第 04 章 — 參考解答

回到 [章節原文](../04-function-pointer.md)。

---

## 練習 1：CSV writer callback

```c
#include <stdio.h>
#include "gps/nmea_parser.h"

static void csv_writer(const gps_fix_t *fix, void *user_data)
{
    FILE *fp = (FILE *)user_data;
    fprintf(fp, "%02u:%02u:%02u,%.6f,%.6f,%.1f,%u,%d\n",
            fix->time.hour, fix->time.minute, fix->time.second,
            fix->latitude, fix->longitude, fix->altitude_m,
            fix->satellites, fix->valid ? 1 : 0);
}

int main(void)
{
    FILE *fp = fopen("fixes.csv", "w");
    if (fp == NULL) return 1;

    fprintf(fp, "time,lat,lon,alt_m,sats,valid\n");

    nmea_parser_t p;
    nmea_parser_init(&p, csv_writer, fp);

    const char *stream =
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n"
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
    for (const char *c = stream; *c; ++c) nmea_parser_feed_byte(&p, *c);

    fclose(fp);
    return 0;
}
```

**重點**：

- `user_data` 直接收 `FILE *`，省去包成 struct。
- `FILE *` 本身就是個指標（不透明指標 — 你看不到內部，但 stdio 知道）。
- 用 callback 而非 hard-code，所以同一個 parser 可以同時餵給 logger、CSV、UI⋯⋯。

---

## 練習 2：加上 `on_error` callback

修改 `include/gps/nmea_parser.h`：

```c
typedef void (*nmea_error_cb_t)(nmea_status_t status,
                                const char *sentence,
                                void *user_data);

typedef struct {
    char           sentence_buf[NMEA_MAX_SENTENCE_LEN];
    size_t         length;
    bool           collecting;
    gps_fix_t      latest_fix;
    nmea_fix_cb_t  on_fix;
    nmea_error_cb_t on_error;          // ← 新加
    void          *user_data;
} nmea_parser_t;

void nmea_parser_init(nmea_parser_t *p,
                      nmea_fix_cb_t fix_cb,
                      nmea_error_cb_t err_cb,    // ← 新加
                      void *user_data);
```

修改 `src/gps/nmea_parser.c`：

```c
nmea_status_t st = nmea_parser_parse_sentence(p->sentence_buf, &p->latest_fix);
if (st == NMEA_OK) {
    if (p->on_fix) p->on_fix(&p->latest_fix, p->user_data);
} else {
    if (p->on_error) p->on_error(st, p->sentence_buf, p->user_data);  // ← 新加
}
```

呼叫端：

```c
static void log_error(nmea_status_t st, const char *sentence, void *ud)
{
    (void)ud;
    fprintf(stderr, "[parser] failed (%d): %.40s...\n", st, sentence);
}

nmea_parser_init(&p, on_fix, log_error, NULL);
```

**思考**：把 `on_fix` 與 `on_error` 兩個 callback 放同一個 struct 是「事件分派」模式，能擴充到 `on_warning`、`on_stats` 等。

---

## 練習 3：把 `void *` 換成具體型別 `app_state_t *`

修改：

```c
typedef void (*nmea_fix_cb_t)(const gps_fix_t *fix, app_state_t *state);
```

**會犧牲什麼**：

1. **耦合**：parser 現在「知道」應用層型別 `app_state_t`。如果應用層改名或改結構，parser 也要跟著改。
2. **不可重用**：另一個專案要重用 `nmea_parser` 時，就必須沿用 `app_state_t` — 違反「parser 應該與應用層解耦」的原則。
3. **不能同時有兩種 callback 需求**：CSV writer 想傳 `FILE *`、計數器想傳 `int *`、Lua 綁定想傳自己的 context⋯⋯，全部塞同一個型別不可能。

**結論**：`void *` 是 C 表達泛型最便宜的方式，**正是為了讓函式庫不依賴應用層型別**。代價是型別檢查要靠程式設計師自己。

> 在 C++ / Rust 裡會用模板 / generics 達到「型別安全的泛型」，但在 C 裡 `void *` 是慣例。

---

## 關鍵概念回顧

- 函式名是位址，可以存進變數、傳給函式。
- `void *user_data` 是 C 表達「我不在意這是什麼」的方式。
- 用 typedef 命名函式指標型別，可讀性大幅提升。
