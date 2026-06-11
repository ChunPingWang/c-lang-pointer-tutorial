# 第 02 章 — 參考解答

回到 [章節原文](../02-array-string.md)。

---

## 練習 1：印出 `sentence + 5` 指向的字元

```c
printf("sentence + 5 指向 '%c'\n", *(sentence + 5));
```

對於 `"$GPRMC,..."` 來說：

```
index:  0   1   2   3   4   5
char:  '$' 'G' 'P' 'R' 'M' 'C'
```

所以 `*(sentence + 5)` 是 `'C'`。

也可以寫 `sentence[5]`，兩者等價。

---

## 練習 2：用指標走訪計算 'A' 出現幾次

```c
static size_t count_a(const char *s)
{
    size_t n = 0;
    while (*s) {                       // 走到 '\0' 為止
        if (*s == 'A') n++;
        s++;                            // 指標自己往前一格
    }
    return n;
}

int main(void)
{
    const char sentence[] =
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
    printf("A 出現 %zu 次\n", count_a(sentence));
    return 0;
}
```

預期輸出：`A 出現 2 次`（一個是 valid 旗標的 `A`，一個是 `*6A` 結尾）。

**重點**：`s++` 移動的是「指標自己」、不是字串本身。每次跳 1 byte，因為 `char` 大小是 1。

也可以用「索引」寫法：

```c
for (size_t i = 0; s[i] != '\0'; ++i) {
    if (s[i] == 'A') n++;
}
```

兩種寫法產生的機器碼幾乎一樣，但「指標走訪」風格在 C 程式碼裡是更常見的慣用法。

---

## 練習 3：`const char sentence[]` vs `const char *sentence`

```c
const char sentence_arr[] = "$GPRMC,...";       // 陣列
const char *sentence_ptr  = "$GPRMC,...";       // 指標

printf("陣列 sizeof = %zu\n", sizeof(sentence_arr));  // 例如 69 (含 \0)
printf("指標 sizeof = %zu\n", sizeof(sentence_ptr));  // 8 (在 64-bit 上)
```

**為什麼**：

| 寫法           | sentence 是什麼          | sizeof 結果                     |
| -------------- | ------------------------ | ------------------------------- |
| `char arr[]`   | 真正的陣列、儲存在 stack | 整個陣列的位元組數 (含 `\0`)    |
| `char *ptr`    | 指標、指向 .rodata       | 指標自己的大小 (32-bit 4, 64-bit 8) |

這也是「陣列退化為指標」的反面：**陣列在「自己原本的作用域內」還沒退化**，所以 sizeof 能拿到真實大小；一旦傳進函式（成為參數），就退化為指標，sizeof 就只剩指標大小。

```c
void f(char arr[100]) {
    printf("%zu\n", sizeof(arr));   // 8，不是 100！
}
```

> 這個地雷在第 7 章還會再強調 — 是 C 最容易踩的一個。

---

## 關鍵概念回顧

- `sentence[i]` 是 `*(sentence + i)` 的糖。
- 指標可以用 `++` 移動，每次跳的距離是 `sizeof(指向型別)`。
- 陣列名「幾乎」是指標，但 `sizeof` 是它倆最大的不同點。
