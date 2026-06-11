# 05 — 雙重指標 `**`

> 本章對應範例：[`examples/05_double_pointer/main.c`](../examples/05_double_pointer/main.c)
> 對應「真實」用法：`src/gps/nmea_parser.c` 裡的 `split_fields(char *sentence, char **fields, ...)`

---

## 5.1 從「為什麼需要 `**`」開始想

`*` 是一層位址：「我指向某個東西」。
`**` 是兩層位址：「我指向 *一個指標*」。

兩個典型用途：

1. **指標陣列**：`char *fields[10]` 退化後型別是 `char **`。
2. **想修改指標本身**：函式想把呼叫端的指標改指向別處，就要傳 `T **`。

---

## 5.2 NMEA 欄位拆解：經典 `char **` 用法

一筆 NMEA：

```
GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W
```

我們想拆成 12 個字串：`["GPRMC", "123519", "A", ...]`。
做法：**就地把逗號改成 `\0`，再把每段起點存到一個陣列**。

`src/gps/nmea_parser.c`：

```c
static size_t split_fields(char *sentence, char **fields, size_t max_fields)
{
    size_t n = 0;
    char  *cursor = sentence;
    fields[n++] = cursor;                       // 第 0 欄起點
    while (*cursor != '\0' && *cursor != '*' && n < max_fields) {
        if (*cursor == ',') {
            *cursor = '\0';                     // 把分隔符變成字串結束
            fields[n++] = cursor + 1;           // 下一欄起點
        }
        cursor++;
    }
    ...
    return n;
}
```

關鍵在 `char **fields`：

- `fields` 自己是個 `char **`，指向一個 `char *` 陣列的開頭。
- `fields[i]` 是其中一個 `char *`，指向某個欄位字串的開頭。
- `fields[i][j]` 是該欄位的第 j 個字元。

---

## 5.3 視覺化

呼叫前：

```c
char sentence[] = "GPRMC,123519,A,4807.038,...";
char *fields[16] = {0};
split_fields(sentence, fields, 16);
```

呼叫後 `sentence` 被改成：

```
GPRMC\0123519\0A\04807.038\0N\0...
^      ^       ^  ^         ^
|      |       |  |         |
fields[0..4] 各自指向這些位置
```

`fields` 的內容：

```
fields[0] -> 0x...00  ("GPRMC")
fields[1] -> 0x...06  ("123519")
fields[2] -> 0x...0D  ("A")
...
```

跑範例：

```bash
./build/examples/ex_05_double_pointer
```

```
拆出 12 個欄位:
  fields[0] = "GPRMC"  (位址 0x16f68a5b7)
  fields[1] = "123519"  (位址 0x16f68a5bd)
  fields[2] = "A"  (位址 0x16f68a5c4)
  ...
fields 自己是 char**, sizeof(fields[0]) = 8 (一個指標的大小)
```

---

## 5.4 「函式想改呼叫端指標」的情境

另一個 `**` 的典型用途：

```c
void take_ownership(int **pp)
{
    static int storage[] = {10, 20, 30};
    *pp = storage;          // 把呼叫端的指標指向 storage
}

int *p = NULL;
take_ownership(&p);         // 傳「p 的位址」，即 int **
// 現在 p 指向 storage[0]
```

如果只傳 `int *p`，函式只能改 `*p`，不能改 `p` 自己指向哪。

> 同樣的概念：第 1 章用 `&satellites` 改 `satellites`，這裡用 `&p` 改 `p`。
> **要改誰，就傳誰的位址** — 規則完全一致，只是層數多一層。

---

## 5.5 為什麼 `main` 的 `argv` 是 `char *argv[]`？

```c
int main(int argc, char *argv[]);   // 或寫 int main(int, char **)
```

執行 `./gps_demo foo bar`：

- `argc = 3`
- `argv[0]` 指向字串 `"./gps_demo"`
- `argv[1]` 指向 `"foo"`
- `argv[2]` 指向 `"bar"`

跟 `fields` 完全是同一個結構：**一個指標陣列，每個元素指向一個字串**。

---

## 5.6 三重指標、四重指標？

理論上可以一直堆 `***`，但實務上 **超過兩層就要警覺**。
如果你需要 `int ***`，多半代表設計可以重構成「結構體 + 指標」。

`char **` 是少數會頻繁出現的合理用法（指標陣列、argv、out parameter 改指標）。

---

## 5.7 動手練習

1. 在 `examples/05_double_pointer/main.c` 加一個函式 `print_fields(char **fields, size_t n)`，把每欄印出來。
2. 寫一個 `find_field(char **fields, size_t n, const char *target)`，回傳目標字串在第幾欄。
3. 修改 `split_inplace`，讓它在遇到 `\0` 之前不停 — 思考為什麼原版要檢查 `*cursor`。

---

> 練習解答：[exercises-solutions/solutions-05.md](exercises-solutions/solutions-05.md)

## 下一章

→ [06 — `const` 與 `volatile`](06-const-volatile.md)
