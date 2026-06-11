# 01 — 指標的本質：地址與解參考

> 本章對應範例：[`examples/01_basics/main.c`](../examples/01_basics/main.c)
> 對應的「真實」用法：`src/gps/ring_buffer.c` 裡的 `ring_buffer_init`

---

## 1.1 變數 = 一塊有名字的記憶體

C 裡每個變數都實際存在於記憶體某個位址上：

```c
int satellites = 8;
```

可以想成：

```
位址          內容
0x16b0ae628 ┐
            │ 0x00000008      <-- satellites 這個變數住在這裡
0x16b0ae62C ┘
```

> 「位址」就是門牌號碼，「內容」是房子裡的東西。
> `int` 在 64-bit 系統通常占 4 bytes，所以 satellites 佔了 4 個連續位址。

---

## 1.2 `&` 取地址、`*` 解參考

```c
int  satellites = 8;
int *p_sat      = &satellites;
```

- `&satellites` 唸做「取 satellites 的地址」，結果是 `int *` 型別。
- `p_sat` 是個指標變數，自己也住在某個位址，但它「內容」是另一個變數的位址。

```
0x16b0ae620  [ 0x16b0ae628 ]   <-- p_sat (內容是另一個地址)
0x16b0ae628  [ 0x00000008 ]   <-- satellites
```

要透過 `p_sat` 拿回 `satellites` 的值，用 `*`：

```c
printf("%d\n", *p_sat);  // 印出 8
```

`*p_sat` 唸做「p_sat 所指的那個東西」。

### 兩個 `*` 的不同情境

```c
int *p = &x;   // (1) 宣告時的 *：表示「p 是 pointer to int」
*p = 42;       // (2) 運算式裡的 *：解參考，把右邊的值寫進 p 指的位址
```

初學者最常混淆的就是這兩個。記住：**宣告時的 `*` 是型別的一部分，運算式裡的 `*` 是動作**。

---

## 1.3 跑一遍範例

```bash
./build/examples/ex_01_basics
```

你會看到類似輸出：

```
satellites 變數位於記憶體位址: 0x16b0ae628
p_sat 指標自己的位址:         0x16b0ae620
p_sat 內容 (它指向哪裡):       0x16b0ae628    <-- 與 satellites 的位址相同
*p_sat 解參考後拿到的值:        8
透過指標改寫後, satellites = 12
```

最後一行最關鍵：透過指標寫入，**會真的改到原本的變數**。
這就是為什麼 C 函式想「對外修改參數」時，必須傳指標。

---

## 1.4 對到 GPS 專案：`ring_buffer_init`

看 `src/gps/ring_buffer.c` 第一段：

```c
void ring_buffer_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity)
{
    rb->buffer   = storage;
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
}
```

如果 `ring_buffer_init` 不收指標，而是收一個 `ring_buffer_t` 值，那麼：

```c
void bad_init(ring_buffer_t rb, ...) {
    rb.head = 0;   // 只是改到函式內的「副本」
}
```

呼叫端的 `rb` 不會被改到。**這就是為什麼幾乎所有「初始化函式」都收指標。**

呼叫端怎麼用？看 `src/app/main.c`：

```c
ring_buffer_t  rx;
ring_buffer_init(&rx, storage, sizeof(storage));
//               ^^^ 取 rx 的位址傳進去
```

完美對應到本章的 `&` / `*` 概念。

---

## 1.5 NULL 是什麼？

C 規範保證：值為 `0` 的指標是「不指向任何有效物件」的指標，慣例上寫成 `NULL`：

```c
int *p = NULL;
if (p != NULL) {
    *p = 10;       // 只有當 p 有效時才解參考
}
```

**用 NULL 表示「還沒指向任何東西」是好習慣**。不檢查就解參考是 C 程式 segfault 的第一名原因。

`src/gps/nmea_parser.c` 開頭就在做這件事：

```c
nmea_status_t nmea_parser_parse_sentence(const char *sentence, gps_fix_t *out_fix)
{
    if (sentence == NULL || out_fix == NULL) return NMEA_ERR_BAD_FIELD;
    ...
}
```

---

## 1.6 動手練習

打開 `examples/01_basics/main.c`，試試這些改動，觀察編譯器訊息或執行結果：

1. 把 `int *p_sat = &satellites;` 改成 `int *p_sat = satellites;`（少一個 `&`）
   → 編譯警告或錯誤，因為型別不對。
2. 把 `*p_sat = 12;` 改成 `p_sat = 12;`
   → 可能可以編譯但執行時 crash，因為你把 `p_sat` 指向了 `0x0C` 這個非法位址。
3. 把 `int *p_sat;`（先不初始化）然後直接 `*p_sat = 7;`
   → 經典「野指標」錯誤，行為未定義（UB），通常 segfault。

---

> 練習解答：[exercises-solutions/solutions-01.md](exercises-solutions/solutions-01.md)

## 下一章

→ [02 — 指標、陣列、字串](02-array-string.md)
