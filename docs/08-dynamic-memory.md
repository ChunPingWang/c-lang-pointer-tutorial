# 08 — 動態記憶體 (`malloc` / `realloc` / `free`)

> 本章對應範例：[`examples/08_dynamic_memory/main.c`](../examples/08_dynamic_memory/main.c)
> 對應「真實」用法：`src/app/main.c` 裡的 `read_whole_file()`

---

## 8.1 為什麼需要動態配置？

到目前為止我們的指標都指向 **stack** 上或 **全域** 變數：

```c
uint8_t storage[256];               // stack 陣列, 大小編譯期決定
ring_buffer_t rx;                   // stack 結構
```

但有些情況編譯期不知道大小：

- 讀檔 — 檔案多大要執行時才知道（`main.c` 的 `read_whole_file`）。
- 收集任意筆 GPS fix — 跑多久就收多少筆。
- 字串拼接、JSON 序列化 — 結果長度動態。

這時要向 **heap** 借記憶體：`malloc` / `calloc` / `realloc`，用完 `free`。

---

## 8.2 三個基本函式

```c
void *malloc(size_t bytes);             // 配置 bytes 個 byte, 內容未初始化
void *calloc(size_t n, size_t each);    // 配置 n*each bytes, 內容全部歸 0
void *realloc(void *p, size_t new_bytes); // 重新調整 p 的大小, 可能搬家
void  free(void *p);                    // 釋放
```

回傳 `void *` 是「我不知道你拿來放什麼」的泛型寫法 — 接收端習慣會 cast：

```c
uint8_t *buf = (uint8_t *)malloc(256);
```

> 在純 C 裡 cast `malloc` 結果不是必要（`void *` 自動轉），但對 C++ 兼容性與閱讀清晰度有幫助，看個人風格。

---

## 8.3 配置失敗會回 NULL

```c
uint8_t *buf = malloc(SIZE);
if (buf == NULL) {
    /* 記憶體不夠, 必須處理 */
    return -1;
}
```

`malloc` 失敗在 host 上很罕見（除非配置超大），但**在 STM32 上 SRAM 有限，很常發生**。
公開 API 一定要檢查 NULL。

---

## 8.4 看 `read_whole_file` 怎麼做 (`src/app/main.c`)

```c
static uint8_t *read_whole_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        perror(path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }

    uint8_t *buf = malloc((size_t)sz);
    if (buf == NULL) { fclose(f); return NULL; }   // ← 失敗檢查 + 關檔

    size_t read_bytes = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if (read_bytes != (size_t)sz) {
        free(buf);                                   // ← 失敗路徑要 free
        return NULL;
    }

    *out_size = read_bytes;
    return buf;
}
```

注意三件事：

1. **每個錯誤路徑都要清理已配置的資源**（檔案要關、buffer 要 free）。
   忘記一個 → leak。
2. **失敗時回 NULL，呼叫端必須檢查**。
3. **成功時把所有權交給呼叫端**：呼叫端必須在用完後 `free` — 這就是 C 的「擁有權契約」，靠註解或慣例傳達，編譯器不會幫你檢查。

---

## 8.5 擁有權 (ownership) 概念

C 沒有自動垃圾回收 → 每塊 `malloc` 出來的記憶體**永遠**只有一個「擁有者」負責 `free`。

```
malloc()
   ↓
擁有者 A (e.g. read_whole_file 函式內)
   ↓ return
擁有者 B (e.g. main 函式)
   ↓ free()
釋放
```

如果同一塊被兩次 free → **double free**（UB）。
如果都不 free → **memory leak**。
如果擁有者已 free 卻還在用指標 → **use-after-free**。

**規則**：API 文件必須講清楚「誰擁有這個指標、誰負責 free」。
例如 `read_whole_file` 應該在註解寫「呼叫端負責 `free` 回傳值」。

---

## 8.6 `realloc` 與動態成長

跑範例：

```bash
./build/examples/ex_08_dynamic_memory
```

輸出大致：

```
初始: items=(nil), size=0, capacity=0
push #1: items=0x6000001a4080, size=1, capacity=4
push #2: items=0x6000001a4080, size=2, capacity=4
push #3: items=0x6000001a4080, size=3, capacity=4
push #4: items=0x6000001a4080, size=4, capacity=4
push #5: items=0x6000022f4000 (realloc 搬家了!), size=5, capacity=8
...
push #9: items=0x6000037d2000 (realloc 搬家了!), size=9, capacity=16
```

關鍵程式片段：

```c
size_t new_cap = (fa->capacity == 0) ? 4 : fa->capacity * 2;
gps_fix_t *grown = realloc(fa->items, new_cap * sizeof(gps_fix_t));
if (grown == NULL) {
    return false;        // 原本 fa->items 還在, 沒搞丟
}
fa->items    = grown;
fa->capacity = new_cap;
```

`realloc` 的兩種行為：

1. **就地擴充**：若舊位址後面剛好有空間，直接擴大，回同樣的指標。
2. **搬家**：找新地方配置，拷貝舊內容，free 舊地方，回新指標。

**重要陷阱**：

```c
fa->items = realloc(fa->items, new_cap);   // ❌ 若 realloc 失敗, fa->items 變 NULL, 原資料 leak!
```

正確寫法：用暫時變數接：

```c
gps_fix_t *grown = realloc(fa->items, new_cap);
if (grown == NULL) return false;
fa->items = grown;
```

---

## 8.7 倍增成長策略

範例每次 capacity 翻倍：4 → 8 → 16 → 32...

**為什麼是倍增不是 +1？**

- 加 1：每次 push 都要 realloc + 拷貝 → O(n²) 總成本。
- 倍增：均攤下來每次 push 仍是 O(1)（少數 push 很貴但次數呈對數）。

C++ 的 `std::vector`、Rust 的 `Vec` 都用倍增（或乘 1.5）策略。

---

## 8.8 `free(NULL)` 是合法的

```c
free(NULL);     // OK, 什麼都不做
```

所以解構函式可以一律呼叫 `free`，不必先檢查：

```c
static void fa_destroy(fix_array_t *fa)
{
    free(fa->items);
    fa->items = NULL;       // 防止後續誤用
}
```

並且 **`free` 完設 NULL** 是最便宜的 use-after-free 防線。

---

## 8.9 stack vs heap 一張表

| 性質           | stack                    | heap                       |
| -------------- | ------------------------ | -------------------------- |
| 配置方式       | 宣告區域變數             | `malloc` / `calloc`        |
| 釋放方式       | 函式 return 時自動       | 必須 `free`                |
| 速度           | 極快 (僅移動 SP 暫存器)  | 慢 (allocator 查表)        |
| 大小限制       | 受 stack size 限制       | 受 heap / RAM 限制         |
| 生命週期       | 函式作用域內              | 任意 (從 malloc 到 free)   |
| 碎片化         | 不會                     | 會 (尤其長時間運行)        |
| 嵌入式偏好     | ★ 預設首選               | 視情況, 通常避免           |

**嵌入式黃金規則**：能用 stack/static 就用，不得已才 heap，且大多在啟動時一次配好。
這也是為什麼 `gps_core` 完全不依賴 `malloc` — host 的 `main.c` 才用。

---

## 8.10 嵌入式專用：pool allocator (記憶體池)

如果你要在 STM32 上配置很多「相同大小的物件」（例如 64 個 `gps_fix_t`），別用 `malloc`，用 pool：

```c
#define POOL_SIZE 64
static gps_fix_t  pool_storage[POOL_SIZE];
static bool       pool_used[POOL_SIZE];

gps_fix_t *pool_alloc(void) {
    for (int i = 0; i < POOL_SIZE; ++i) {
        if (!pool_used[i]) { pool_used[i] = true; return &pool_storage[i]; }
    }
    return NULL;
}

void pool_free(gps_fix_t *p) {
    int idx = (int)(p - pool_storage);
    if (idx >= 0 && idx < POOL_SIZE) pool_used[idx] = false;
}
```

好處：

- 無碎片化。
- O(1) 分配（用 free list 還可以更快）。
- 大小固定 → 編譯期可推算最壞情況記憶體用量。

FreeRTOS、ChibiOS 等 RTOS 都內建類似機制。

---

## 8.11 動手練習

1. 寫一個 `fa_pop(fix_array_t *fa, gps_fix_t *out)`，把最後一筆取出來；考慮要不要在 `size < capacity / 4` 時 `realloc` 縮回。
2. 把 `fa_push` 改成「成長 1.5 倍」而非 2 倍，量測 push 1000 筆的 realloc 次數差異。
3. 故意把 `fa_destroy` 裡的 `free` 拿掉，用 `valgrind` 或 ASan 跑（編譯加 `-fsanitize=address`），看 leak report 長什麼樣。

---

> 練習解答：[exercises-solutions/solutions-08.md](exercises-solutions/solutions-08.md)

## 下一章

→ [09 — 連結串列](09-linked-list.md)
