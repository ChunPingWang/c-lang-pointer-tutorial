# 第 08 章 — 參考解答

回到 [章節原文](../08-dynamic-memory.md).

---

## 練習 1：`fa_pop` + 縮回容量

```c
static bool fa_pop(fix_array_t *fa, gps_fix_t *out)
{
    if (fa->size == 0) return false;
    fa->size--;
    if (out) *out = fa->items[fa->size];

    /* 當使用量降到 1/4 capacity 以下時, 縮回到一半。
     * 為什麼是 1/4 而不是 1/2? 避免 push/pop 交界處抖動: 若你在剛縮一半的邊緣再 push 一次,
     * 又要立刻 grow, 反覆 realloc 損失效能。 */
    if (fa->capacity >= 8 && fa->size < fa->capacity / 4) {
        size_t new_cap = fa->capacity / 2;
        gps_fix_t *shrunk = realloc(fa->items, new_cap * sizeof(gps_fix_t));
        if (shrunk != NULL) {            /* 縮失敗就保持原本大小, 不視為錯誤 */
            fa->items    = shrunk;
            fa->capacity = new_cap;
        }
    }
    return true;
}
```

**關鍵點**：

- 不要 `size == capacity / 2` 就縮 → 邊界抖動。
- 縮失敗不算錯：原指標仍可用，只是沒省到記憶體。

---

## 練習 2：1.5 倍 vs 2 倍成長

C++ `std::vector` 在某些 STL 實作（GCC libstdc++）用 2 倍，MSVC 用 1.5 倍。
1.5 倍的論點是：歷史上配置的空間總和可能小於目前需求 → 較可能就地擴充而非搬家。

實驗：

```c
/* 推 1000 筆, 計數有幾次「searchedmissing != 原 items 指標」(即 realloc 搬家) */
size_t reallocs_2x  = 0;
size_t reallocs_15x = 0;

/* 模擬 2 倍 */
fix_array_t fa;
fa_init(&fa);
gps_fix_t f = {0};
for (int i = 0; i < 1000; ++i) {
    gps_fix_t *prev = fa.items;
    fa_push(&fa, &f);
    if (prev != fa.items) reallocs_2x++;
}

/* 改 fa_push 內成長公式為 capacity + capacity / 2, 重做一次 */
```

理論上：

| 成長率 | 推 N 筆所需 realloc 次數     |
| ------ | ---------------------------- |
| 2.0×   | ~ log₂(N) ≈ 10 次 (N=1000)   |
| 1.5×   | ~ log₁.₅(N) ≈ 18 次          |

但 1.5× 留下的「碎片」較可能在後續成長時被重用，整體記憶體浪費較少。
trade-off：**realloc 次數多 vs 記憶體 footprint 小**。

教學重點：成長係數是工程選擇，不是定理。

---

## 練習 3：故意 leak + ASan 偵測

修改 `fa_destroy`，把 `free(fa->items)` 註解掉。

編譯時加 `-fsanitize=address -g`：

```bash
gcc -fsanitize=address -g -I include \
    examples/08_dynamic_memory/main.c \
    src/gps/*.c src/hal/*.c -lm -o leak_demo

./leak_demo
```

ASan 在程式結束時報告：

```
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1024 byte(s) in 1 object(s) allocated from:
    #0 0x... in realloc (libasan)
    #1 0x... in fa_push examples/08_dynamic_memory/main.c:30
    #2 0x... in main examples/08_dynamic_memory/main.c:70

SUMMARY: AddressSanitizer: 1024 byte(s) leaked in 1 allocation(s).
```

**重點**：ASan 能精準告訴你「哪個 malloc 沒對應的 free」、「哪個 free 後又用了」。
寫 C 程式建議**永遠**先用 ASan 跑過一次。

GCC / clang 都支援 `-fsanitize=address`。
在 CMake 裡可以這樣加（自家專案的 ASan target）：

```cmake
add_executable(leak_demo ...)
target_compile_options(leak_demo PRIVATE -fsanitize=address -g)
target_link_options(leak_demo PRIVATE -fsanitize=address)
```

---

## 關鍵概念回顧

- `malloc / free` 是 C 唯一的動態記憶體機制，所有權必須由人腦追蹤。
- `realloc(NULL, n)` ≡ `malloc(n)`、`realloc(p, 0)` 實作定義但**不要依賴**。
- `free(NULL)` 是合法 no-op。
- 嵌入式優先用 pool 而非 `malloc`。
