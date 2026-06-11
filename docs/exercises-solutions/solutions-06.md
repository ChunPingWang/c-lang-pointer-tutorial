# 第 06 章 — 參考解答

回到 [章節原文](../06-const-volatile.md).

---

## 練習 1：拿掉 `volatile` 在 STM32 上的後果

修改：

```c
typedef struct {
    uint8_t       *buffer;
    size_t         capacity;
    size_t head;          // 拿掉 volatile
    size_t tail;          // 拿掉 volatile
} ring_buffer_t;
```

**host 上**：99% 看不出差別。host CPU 沒有 ISR 干擾，編譯器看到主迴圈內有 `ring_buffer_push/pop` 呼叫（函式邊界會強制 reload），不會把 `head/tail` 整個 cache 在暫存器裡。

**STM32 上潛在 bug**：

考慮這段主迴圈：

```c
while (ring_buffer_is_empty(&rx)) {
    /* 什麼都不做, 等 ISR 把 byte push 進來 */
}
```

`ring_buffer_is_empty` 內容是：

```c
return rb->head == rb->tail;
```

沒有 `volatile` 時，編譯器可能優化成「在主迴圈外讀一次 head/tail，迴圈內只比較那兩個常數」 → **死迴圈，看不到 ISR 的更新**。

加 `-O2 -DNDEBUG` 後特別容易中招。`volatile` 強迫每次都「真的去記憶體讀」，破壞這個錯誤的最佳化。

**深一層**：`volatile` **只擋編譯器最佳化**，並不保證原子性。
若 ISR 和主迴圈同時改 `head`（雙生產者場景），即使有 volatile 也會壞。
單生產者單消費者（SPSC，ring buffer 標準用法）+ volatile 是夠的。

---

## 練習 2：把 `const char *` 改成 `char *` 的編譯器反應

修改 `nmea_parser_parse_sentence` 簽名：

```c
nmea_status_t nmea_parser_parse_sentence(char *sentence, gps_fix_t *out_fix);
//                                       ^^^^ 拿掉 const
```

**呼叫端可能會出問題**：

```c
nmea_parser_parse_sentence("$GPRMC,...", &fix);
```

字串字面值的型別在現代 clang/gcc 下是 `char *`，但實際儲存在唯讀段。
有些編譯器會出警告：

```
warning: passing argument 1 of 'nmea_parser_parse_sentence'
         discards 'const' qualifier from pointer target type
```

且嚴格說，這條 API 失去了「我不會改你的字串」的契約 → 維護期內任何工程師都可能在 parser 裡偷加一行 `sentence[0] = '\0'`，呼叫端會被嚇到。

**結論**：能加 const 就加 const。**它是契約、不是裝飾**。

---

## 練習 3：`mmio_set_bit`

```c
static inline void mmio_set_bit(volatile uint32_t *reg, int bit)
{
    *reg |= (1u << bit);
}

static inline void mmio_clear_bit(volatile uint32_t *reg, int bit)
{
    *reg &= ~(1u << bit);
}

static inline bool mmio_read_bit(const volatile uint32_t *reg, int bit)
{
    return (*reg & (1u << bit)) != 0;
}
```

**細節**：

1. `volatile uint32_t *reg`：寫進去會真的觸發硬體。
2. `1u << bit`：用 unsigned 避免 `1 << 31` 是 UB 的問題。
3. `*reg |= (1u << bit)` 在機器層級是 **read-modify-write** — 不是原子操作。
   如果 ISR 也會碰同一個 register，要用 atomic 或關中斷再操作。

**用例**：

```c
#define GPIOA_ODR ((volatile uint32_t *)0x40020014)
mmio_set_bit(GPIOA_ODR, 5);     // 點亮 PA5 (Nucleo F4 內建 LED)
```

---

## 關鍵概念回顧

- `const` 是契約，不是裝飾。盡量加。
- `volatile` 告訴編譯器「別優化掉這個記憶體存取」，**不保證原子**。
- 中斷共享變數 → volatile（+ 可能還需要關中斷或 atomic）。
- MMIO → 永遠 volatile。
