# 第 03 章 — 參考解答

回到 [章節原文](../03-struct-pointer.md)。

---

## 練習 1：`change_lat_by_value` vs `change_lat_by_pointer`

```c
static void change_lat_by_value(gps_fix_t fix)
{
    fix.latitude = 99.9;
    printf("[傳值] 函式內 lat=%.4f\n", fix.latitude);
}

static void change_lat_by_pointer(gps_fix_t *fix)
{
    fix->latitude = 99.9;
    printf("[傳指標] 函式內 lat=%.4f\n", fix->latitude);
}

int main(void)
{
    gps_fix_t fix = { .latitude = 25.0, ... };

    change_lat_by_value(fix);
    printf("呼叫後 lat=%.4f\n", fix.latitude);   // 25.0 (沒變)

    change_lat_by_pointer(&fix);
    printf("呼叫後 lat=%.4f\n", fix.latitude);   // 99.9 (被改了)
}
```

**結論一致**：傳值無法修改呼叫端，傳指標可以。
這條規則對 `int`、結構體、陣列退化指標等都成立。

---

## 練習 2：印出每個欄位的位址

```c
gps_fix_t fix;
printf("&fix              = %p\n", (void *)&fix);
printf("&fix.latitude     = %p (offset %ld)\n",
       (void *)&fix.latitude,
       (char *)&fix.latitude - (char *)&fix);
printf("&fix.longitude    = %p (offset %ld)\n",
       (void *)&fix.longitude,
       (char *)&fix.longitude - (char *)&fix);
printf("&fix.satellites   = %p (offset %ld)\n",
       (void *)&fix.satellites,
       (char *)&fix.satellites - (char *)&fix);
printf("&fix.valid        = %p (offset %ld)\n",
       (void *)&fix.valid,
       (char *)&fix.valid - (char *)&fix);
```

典型輸出（位址會因執行而異）：

```
&fix              = 0x16f...30
&fix.latitude     = 0x16f...30 (offset 0)
&fix.longitude    = 0x16f...38 (offset 8)
&fix.satellites   = 0x16f...58 (offset 40)
&fix.valid        = 0x16f...4c (offset ?)  -- 隨 padding 而定
```

**重點觀察**：

1. 第一個欄位的位址 = 結構體本身位址（offset 0）。
2. `double` 欄位之間相隔 8 bytes（`double` 大小）。
3. 把指標相減時必須先 cast 成 `char *`，因為其他型別相減會自動除以 sizeof。

也可以直接用 `offsetof()`：

```c
#include <stddef.h>
printf("offsetof(satellites) = %zu\n", offsetof(gps_fix_t, satellites));
```

---

## 練習 3：欄位順序重排對 sizeof 的影響

原版 `gps_fix_t` (約 56 bytes)：

```c
typedef struct {
    double   latitude;          // 8
    double   longitude;         // 8
    double   altitude_m;        // 8
    double   speed_knots;       // 8
    double   course_deg;        // 8
    gps_time_t time;            // 6 + 2 padding
    uint8_t  satellites;        // 1 + 3 padding
    gps_fix_quality_t fix_quality; // 4
    bool     valid;             // 1 + 7 padding 對齊到 8
} gps_fix_t;
```

**實驗**：交錯放小型欄位：

```c
typedef struct {
    uint8_t  satellites;        // 1
    double   latitude;          // 8 (前面有 7 bytes padding)
    bool     valid;             // 1
    double   longitude;         // 8 (前面有 7 bytes padding)
    ...
} gps_fix_bad_t;
```

結果：sizeof 會比 56 大很多（可能 80+）。

**為什麼**：`double` 必須對齊到 8 bytes，編譯器在前面塞 padding 達成對齊。

**結論**：

- 把**大型欄位放前面、小型欄位集中放後面**，padding 最少。
- 嵌入式環境（SRAM 寶貴）特別需要這種「結構體 packing」意識。
- 真的要極致壓縮可以用 `#pragma pack(1)`，但會犧牲存取效能（ARM 上甚至會 fault）。

> **不要為了「省一個 byte」就亂用 `#pragma pack`**，先理解對齊規則，再決定要不要犧牲效能換空間。

---

## 關鍵概念回顧

- 結構體幾乎一定傳指標（除非真的很小如 `gps_time_t`）。
- 欄位順序影響 sizeof — padding 真實存在。
- `&fix.field` 與 `(char *)&fix + offsetof(...)` 結果相同。
