# 第 01 章 — 參考解答

回到 [章節原文](../01-pointer-basics.md)。

---

## 練習 1：少一個 `&`

```c
int  satellites = 8;
int *p_sat      = satellites;    // 改: 拿掉 &
```

**預期結果**：編譯失敗或警告。

clang 上的訊息大致是：

```
warning: incompatible integer to pointer conversion initializing
         'int *' with an expression of type 'int' [-Wint-conversion]
    int *p_sat = satellites;
         ^      ~~~~~~~~~~
```

**為什麼**：`p_sat` 的型別是 `int *`（指標）、`satellites` 是 `int`（整數），兩者類別不同。
C 允許整數隱式轉成指標（多半是給特殊用途如 MMIO 用），但會出 warning。
若強行執行：`p_sat` 會指向位址 `0x00000008`，這是無效位址 → 解參考時 segfault。

> 教訓：**取地址才能讓變數與指標連起來**。

---

## 練習 2：把 `*p_sat = 12;` 改成 `p_sat = 12;`

```c
int  satellites = 8;
int *p_sat      = &satellites;
p_sat = 12;        // 改: 拿掉 *
```

**預期結果**：同樣是 `int → int *` 警告 + 執行期 crash。

執行流程：

1. `p_sat = 12` 把 `p_sat` 改成指向位址 `0x0C`。
2. 後面如果有 `*p_sat`，就會嘗試讀寫位址 `0x0C` → segfault。

`*p_sat = 12` vs `p_sat = 12` 完全不同：

| 寫法            | 意思                             |
| --------------- | -------------------------------- |
| `*p_sat = 12`   | 把 12 寫到 p_sat 所指的記憶體裡  |
| `p_sat = 12`    | 把 p_sat 自己改成「指向位址 12」 |

> 教訓：**`*` 在等號左邊跟在等號右邊的角色一樣 — 都是「解參考」動作**。

---

## 練習 3：未初始化的野指標

```c
int *p_sat;            // 未初始化
*p_sat = 7;            // 直接寫
```

**預期結果**：未定義行為 (UB)。多半 segfault，但實際上：

- macOS / Linux 在 Debug 模式可能立刻崩潰。
- Release 模式編譯器可能優化掉、或踩到其他堆疊內容。
- STM32 上更糟 — 可能寫到隨機 RAM、暫存器或無效位址，整個 MCU 復位。

加 `-Wall -Wuninitialized` 編譯時 clang 會提醒：

```
warning: variable 'p_sat' is uninitialized when used here [-Wuninitialized]
```

但編譯器**只是建議**，不會強迫你修。

> 教訓：**永遠在宣告指標時初始化 — 至少給 NULL**。

```c
int *p_sat = NULL;     // ✅ 好習慣
```

---

## 關鍵概念回顧

- `&` 取地址、`*` 解參考 — 是互逆的兩個運算。
- 指標的「型別」決定了它「解參考時拿到什麼大小的東西」。
- 不要省略 `&` 與 `*`，每個位置都有意義。
