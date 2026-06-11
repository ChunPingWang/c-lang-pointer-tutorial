# 09 — 連結串列 (Linked List)

> 本章對應範例：[`examples/09_linked_list/main.c`](../examples/09_linked_list/main.c)
> 對應「真實」用法：`src/gps/track_log.c` — 一個記錄 GPS 軌跡的單向串列

---

## 9.1 為什麼需要連結串列？

第 8 章的動態陣列 (`fix_array_t`) 已經能裝任意筆 fix，為什麼還要 linked list？

| 性質                       | 動態陣列 (vector)               | 單向連結串列 (singly linked)    |
| -------------------------- | ------------------------------- | ------------------------------- |
| 隨機存取 `arr[i]`          | ✅ O(1)                         | ❌ O(n) (要從 head 走)          |
| append 到尾端              | 均攤 O(1)，偶爾 realloc 搬家    | O(1)（有 tail 指標）            |
| 插入到中間                 | O(n)（後面要全部搬）             | O(1)（只要改幾個指標）          |
| 刪除中間節點               | O(n)（後面要全部搬）             | O(1)（只要改幾個指標）          |
| 記憶體 overhead            | 0（連續陣列）                   | 每節點多 `sizeof(void*)`        |
| Cache 友善度               | ★★★（連續記憶體）               | ★（節點隨機散布在 heap）         |
| 對 `realloc` 失敗的容忍度  | ❌（成長時可能失敗）             | ✅（每節點獨立 malloc）         |

GPS track log 的取捨：

- **不需要隨機存取**：通常從頭走到尾就好。
- **常常要過濾 / 移除無效點**：在串列中間刪除是 O(1)。
- **不知道總筆數**：linked list 不必預先估容量。

→ 用 linked list 合理。

---

## 9.2 節點與串列的資料結構

`include/gps/track_log.h`：

```c
typedef struct track_node {
    gps_fix_t          fix;
    struct track_node *next;        /* ← 指向下一個節點 */
} track_node_t;

typedef struct {
    track_node_t *head;
    track_node_t *tail;             /* 為了 O(1) append */
    size_t        length;
} track_log_t;
```

**為什麼節點裡要寫 `struct track_node`，不能直接寫 `track_node_t`？**
因為這個結構體還沒 `typedef` 完成，C 編譯器還不認得 `track_node_t`，必須用 `struct track_node` 引用自己。
這是 C 的「自我參照結構體」標準寫法。

視覺化：

```
head ──→ [fix0|next] ──→ [fix1|next] ──→ [fix2|next] ──→ NULL
                                              ↑
                                            tail
```

---

## 9.3 Append：O(1) 加到尾端

```c
bool track_log_append(track_log_t *log, const gps_fix_t *fix)
{
    track_node_t *node = malloc(sizeof(*node));
    if (node == NULL) return false;
    node->fix  = *fix;            /* struct 複製 */
    node->next = NULL;

    if (log->tail == NULL) {
        log->head = node;          /* 空串列, 新節點就是 head */
    } else {
        log->tail->next = node;    /* 原本的 tail 接上去 */
    }
    log->tail = node;
    log->length++;
    return true;
}
```

**沒有 tail 指標的話**：每次 append 都要從 head 走到尾巴 → O(n)。
**多維護一個 tail 指標**：append 變 O(1)，代價是刪除時要記得更新 tail。

---

## 9.4 走訪 (foreach)

```c
for (track_node_t *cur = log->head; cur != NULL; cur = cur->next) {
    /* 對 cur->fix 做事 */
}
```

這是 C 寫 linked list 走訪的標準慣用語。讀法：

- 起始：`cur = log->head`
- 條件：`cur != NULL`（走到尾端）
- 推進：`cur = cur->next`

`track_log_foreach` 就把這個迴圈包起來，再透過 callback 把每筆送出去 — 是第 4 章函式指標的應用。

---

## 9.5 Remove：經典「指向指標的指標」技法

`src/gps/track_log.c` 裡這段是 C 程式設計的小經典：

```c
track_node_t **pp = &log->head;
while (*pp != NULL) {
    track_node_t *cur = *pp;
    if (pred(&cur->fix, user_data)) {
        *pp = cur->next;       /* 把上一個的 next 跳過 cur */
        free(cur);
    } else {
        pp = &cur->next;       /* 移到下一個 next 指標的位址 */
    }
}
```

`pp` 是 `track_node_t **`，永遠指向「上一個節點的 `next` 欄位」（或 `head` 自己）。

視覺化「移除中間節點」：

```
原本:    head → A → B → C → NULL
          pp ╾┘ (指向 head)
                pp ╾┘ (指向 A->next)

要刪 B:  pp 指向 A->next, *pp == B
         *pp = B->next   →   A->next = C
         free(B)
                       ┌──→  C
         head → A ─────┘
```

**好處**：不需要為「刪 head」與「刪中間節點」寫兩套程式碼。
傳統寫法會這樣：

```c
if (cur == log->head) {
    log->head = cur->next;
} else {
    prev->next = cur->next;
}
free(cur);
```

兩套邏輯 → 容易漏一個分支。`**pp` 統一處理。

---

## 9.6 Destroy：釋放所有節點

```c
void track_log_destroy(track_log_t *log)
{
    track_node_t *cur = log->head;
    while (cur != NULL) {
        track_node_t *next = cur->next;   /* 先存 next */
        free(cur);                         /* 再 free cur */
        cur = next;
    }
    log->head = log->tail = NULL;
    log->length = 0;
}
```

**為什麼要先存 `next`？**
`free(cur)` 之後 `cur->next` 是未定義行為 — 那塊記憶體已經還給 allocator，內容可能被覆寫。
**順序**：read → store next → free → 用 next → loop。

---

## 9.7 跑範例

```bash
./build/examples/ex_09_linked_list
```

輸出：

```
收集到的 fix (4 筆):
  [0] lat=48.1173 lon=11.5167 sats=0 valid=yes
  [1] lat=48.1173 lon=11.5167 sats=8 valid=yes
  [2] lat=25.0605 lon=121.4431 sats=8 valid=yes
  [3] lat=25.0605 lon=121.4431 sats=10 valid=yes

remove_if(not_valid) 移除了 0 個節點, 現在 length=4
destroy 完成
```

把 parser 接上 `track_log` 後，整條 GPS pipeline 就完整了：

```
UART bytes → ring_buffer → nmea_parser → on_fix callback → track_log linked list
```

---

## 9.8 雙向連結串列 (doubly linked list)

加一個 `prev` 指標就成了雙向：

```c
typedef struct track_dnode {
    gps_fix_t  fix;
    struct track_dnode *next;
    struct track_dnode *prev;
} track_dnode_t;
```

**多了什麼能力**：

- 反向走訪 (從 tail 走到 head)。
- 給定一個節點，O(1) 移除自己（不需要從 head 找前一個）。

**多了什麼成本**：

- 每節點多一個指標欄位（host 上 8 bytes，MCU 上 4 bytes）。
- 插入 / 移除時要更新更多指標 → 程式碼更複雜，更容易寫錯。

Linux kernel 的 `struct list_head` 就是雙向（+ 環狀），效能與彈性兼顧但用法不直觀。

---

## 9.9 環狀連結串列 (circular linked list)

把 tail 的 next 接回 head：

```
head → [A] → [B] → [C] ──┐
        ↑________________│
```

用途：

- Round-robin 排程（永遠有「下一個」）。
- 環狀 buffer 的 logical 表達（但實務上還是用陣列實作較快，見 `ring_buffer.c`）。

---

## 9.10 何時不要用 linked list？

- **需要頻繁隨機存取**：用陣列。
- **資料量很大且常走訪**：陣列的 cache locality 通常壓倒一切。Linus Torvalds 有名的對 linked list 的批評就是針對這點。
- **嵌入式且記憶體緊**：每節點的 overhead + heap 碎片化很傷。

對 GPS track log 來說：
- 資料量通常 < 1000 點
- 主要操作是 append + filter + foreach
- 不需要 random access

→ linked list 還算合適。但如果是「即時 60Hz 採樣連續 24 小時」(=518k 點)，動態陣列會更好。

---

## 9.11 動手練習

1. 加一個 `track_log_iter_t` 迭代器型別，包成「物件導向」風格：
   ```c
   track_log_iter_t it = track_log_iter_begin(&log);
   while (track_log_iter_valid(&it)) {
       const gps_fix_t *fix = track_log_iter_get(&it);
       ...
       track_log_iter_next(&it);
   }
   ```
2. 把 `track_log_t` 改成雙向串列，新增 `track_log_foreach_reverse`。
3. 寫一個 `track_log_reverse(track_log_t *log)`，把整個串列反轉（不重新配置任何節點，只調整 `next` 指標）。
4. 為什麼 `track_log_remove_if` 在迴圈尾要把 `log->tail` 設成 `prev_kept`？把這行拿掉，會怎樣壞？

---

> 練習解答：[exercises-solutions/solutions-09.md](exercises-solutions/solutions-09.md)

## 回到目錄

→ [README.md](../README.md)
