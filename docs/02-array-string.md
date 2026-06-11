# 02 — 指標、陣列、字串

> 本章對應範例：[`examples/02_array_string/main.c`](../examples/02_array_string/main.c)
> 對應「真實」用法：`src/gps/nmea_parser.c` 裡的 `nmea_verify_checksum`、`split_fields`

---

## 2.1 陣列名 = 指向第一個元素的指標（幾乎）

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p     = arr;     // arr 在運算式裡退化成 &arr[0]
```

`p` 與 `arr` 都指向 `arr[0]`。所以下面四個寫法等價：

```c
arr[2]
*(arr + 2)
p[2]
*(p + 2)
```

> 「陣列退化為指標」是 C 一個非常重要也很容易踩雷的特性，第 7 章還會回來談 `sizeof` 在這件事上的差別。

---

## 2.2 字串就是 `\0` 結尾的 char 陣列

```c
const char *msg = "$GPRMC";
```

實際在記憶體中的樣子：

```
位址        內容
0x...0   '$'
0x...1   'G'
0x...2   'P'
0x...3   'R'
0x...4   'M'
0x...5   'C'
0x...6   '\0'    <-- 字串結束的標記
```

`msg` 是 `const char *`，指向 `'$'`。
判斷字串結束就是看到 `\0`：

```c
while (*msg != '\0') {
    putchar(*msg);
    msg++;
}
```

---

## 2.3 指標算術：`+1` 不是真的加 1

```c
int   arr[5];
int  *p = arr;
p + 1;                  // 位址其實往前 sizeof(int)=4 bytes
char *q = "abc";
q + 1;                  // 位址往前 sizeof(char)=1 byte
```

C 編譯器知道每種型別大小，會自動乘上去。
這就是「為什麼字串掃描可以一直 `p++`」 — 因為 `char` 大小是 1，每次正好走一格。

`src/gps/nmea_parser.c` 裡的 checksum 計算：

```c
uint8_t nmea_compute_checksum(const char *body, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum ^= (uint8_t)body[i];        // 或寫成 *(body + i)
    }
    return sum;
}
```

`body[i]` 是糖，編譯器轉成 `*(body + i)`。

---

## 2.4 用兩個指標標記範圍：找 `*` 的位置

NMEA 句子格式：

```
$GPRMC,123519,A,4807.038,N,...,W*6A
                              ^-- 從這裡開始是 checksum
```

我們要計算 `$` 之後、`*` 之前的內容做 XOR 校驗碼。
`src/gps/nmea_parser.c` 的做法：

```c
const char *star = strchr(sentence, '*');     // 找到 '*' 的位置
size_t  body_len = (size_t)(star - (sentence + 1));   // 兩個指標相減 = 距離
```

**兩個指向同一段記憶體的指標可以相減，結果是「中間隔了幾個元素」**。
這就是為什麼上面的算式能算出長度。

跑一下範例：

```bash
./build/examples/ex_02_array_string
```

```
陣列名 sentence 退化為指標時: 0x16d5565c3
sentence + 1 指向 'G'
sentence[3] 等價於 *(sentence + 3) = 'R'
欄位數 (逗號數+1) = 12
checksum 起始於相對偏移 65 處
```

---

## 2.5 const char *、char * const、const char * const

當你看到 `const`，先唸右到左：

```c
const char *p1;        // p1 是 pointer to const char  --> 不能透過 p1 改 *p1
char const *p2;        // 一樣，只是寫法不同
char *const p3;        // p3 是 const pointer to char  --> p3 自己不能變
const char *const p4;  // 兩者都不能改
```

`strchr` 的原型：

```c
char *strchr(const char *s, int c);
```

`const char *s` 表示：「我保證不會透過這個指標改你的字串」。
這是一種「我給的指標是唯讀」的契約，後面第 6 章會深入。

---

## 2.6 字串字面值是常數

```c
char *p = "hello";
p[0] = 'H';          // 未定義行為！可能 segfault
```

`"hello"` 是程式裡的常數區段，**不能寫**。正確寫法：

```c
const char *p = "hello";    // 表明唯讀，編譯器會擋你寫
// 或
char buf[] = "hello";       // 把字串「拷貝」到可寫的陣列裡
buf[0] = 'H';                // OK
```

`src/gps/nmea_parser.c` 裡 `nmea_parser_parse_sentence` 就把句子複製一份：

```c
char working[NMEA_MAX_SENTENCE_LEN];
strncpy(working, sentence + 1, sizeof(working) - 1);
working[sizeof(working) - 1] = '\0';
```

為什麼？因為下一步 `split_fields` 會把逗號 **改寫成 `\0`** 來切欄位 — 直接動原本的字串會炸（特別是字串字面值或唯讀資料）。

---

## 2.7 動手練習

1. 在 `examples/02_array_string/main.c` 印出 `sentence + 5` 指向的字元。
2. 計算 `sentence` 裡 'A' 出現幾次（用指標走訪而非索引）。
3. 試試把 `const char sentence[] = "..."` 改成 `const char *sentence = "..."`，看 `sizeof(sentence)` 變什麼。

> 提示：陣列宣告下 `sizeof(sentence)` 是整個陣列的位元組數；指標下則是指標本身大小（通常 8 bytes）。第 7 章會再展開這個地雷。

---

> 練習解答：[exercises-solutions/solutions-02.md](exercises-solutions/solutions-02.md)

## 下一章

→ [03 — 結構體與指標](03-struct-pointer.md)
