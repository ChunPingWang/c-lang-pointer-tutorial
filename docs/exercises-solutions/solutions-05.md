# 第 05 章 — 參考解答

回到 [章節原文](../05-double-pointer.md).

---

## 練習 1：`print_fields(char **fields, size_t n)`

```c
static void print_fields(char **fields, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        printf("  fields[%zu] = \"%s\"\n", i, fields[i]);
    }
}
```

呼叫：

```c
char sentence[] = "GPRMC,123519,A,...";
char *fields[16] = {0};
size_t n = split_inplace(sentence, fields, 16);
print_fields(fields, n);
```

**重點**：

- `fields[i]` 透過第一層 `*` 拿到一個 `char *`，再透過 `%s` 印字串內容。
- 完整解開：`fields[i]` ≡ `*(fields + i)`，這就是雙層指標的本質。

---

## 練習 2：`find_field`

```c
#include <string.h>

static int find_field(char **fields, size_t n, const char *target)
{
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(fields[i], target) == 0) {
            return (int)i;
        }
    }
    return -1;
}
```

`strcmp` 接兩個 `const char *`，內部會逐 byte 比對直到 `\0` 或不同。

```c
int idx = find_field(fields, n, "GPRMC");
if (idx >= 0) printf("找到 GPRMC 在第 %d 欄\n", idx);
```

---

## 練習 3：拿掉 `*cursor` 檢查的後果

原版：

```c
while (*cursor && n < max_fields) {
    if (*cursor == ',') { ... }
    cursor++;
}
```

如果改成：

```c
while (n < max_fields) {       // 沒檢查 *cursor
    if (*cursor == ',') { ... }
    cursor++;
}
```

**後果**：

- 當字串掃到 `\0` 後，`cursor` 繼續往後跑、超出 `sentence` 的合法範圍。
- 接下來讀到的是 stack 上其他變數的內容 — 行為未定義。
- 很可能某個 byte 剛好是 `','`，就會把那個地址當「新欄位起點」存進 `fields[]` → 後續 `printf("%s", fields[i])` 印出垃圾或 segfault。

**這就是缺少邊界檢查的典型 bug**：症狀不一定立即發生，但執行環境一變就壞。

> 教訓：**走訪字串時永遠記得 `\0` 是邊界**。
> 走訪陣列時永遠記得長度是邊界。

---

## 關鍵概念回顧

- `char **fields` = 「指向一陣列指標的指標」。
- `argv` 與 `fields` 結構完全一樣。
- 超過 2 層的指標多半代表設計可以重構。
