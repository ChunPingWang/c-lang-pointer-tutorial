# 第 09 章 — 參考解答

回到 [章節原文](../09-linked-list.md).

---

## 練習 1：迭代器型別

```c
/* track_log.h */
typedef struct {
    track_node_t *current;
} track_log_iter_t;

static inline track_log_iter_t track_log_iter_begin(const track_log_t *log) {
    return (track_log_iter_t){ .current = log->head };
}
static inline bool track_log_iter_valid(const track_log_iter_t *it) {
    return it->current != NULL;
}
static inline const gps_fix_t *track_log_iter_get(const track_log_iter_t *it) {
    return &it->current->fix;
}
static inline void track_log_iter_next(track_log_iter_t *it) {
    it->current = it->current->next;
}
```

用法：

```c
track_log_iter_t it = track_log_iter_begin(&log);
while (track_log_iter_valid(&it)) {
    const gps_fix_t *fix = track_log_iter_get(&it);
    printf("lat=%.4f\n", fix->latitude);
    track_log_iter_next(&it);
}
```

**為什麼包成 struct 而不是直接 `track_node_t *`？**
這樣將來可以加更多狀態（例如記錄走到第幾個、是否反向）而不必改 API 簽名。

**為什麼設成 `inline`？**
避免「迭代器 = 包一層函式呼叫 overhead」。簡單存取應該被 inline 掉，跟手寫迴圈一樣快。

---

## 練習 2：改成雙向串列

```c
typedef struct track_dnode {
    gps_fix_t           fix;
    struct track_dnode *next;
    struct track_dnode *prev;        /* ← 新增 */
} track_dnode_t;

bool track_log_append(track_log_t *log, const gps_fix_t *fix)
{
    track_dnode_t *node = malloc(sizeof(*node));
    if (!node) return false;
    node->fix  = *fix;
    node->next = NULL;
    node->prev = log->tail;          /* ← 連回前一個 */
    if (log->tail) log->tail->next = node;
    else           log->head = node;
    log->tail  = node;
    log->length++;
    return true;
}

void track_log_foreach_reverse(const track_log_t *log,
                               track_visitor_t visit, void *ud)
{
    size_t i = log->length;
    for (track_dnode_t *cur = log->tail; cur != NULL; cur = cur->prev) {
        visit(&cur->fix, --i, ud);
    }
}
```

**陷阱**：

- 移除節點時要同時更新 `prev->next` 與 `next->prev`，**兩處都要改**。
- 邊界：移除 head 時 `cur->prev` 是 NULL，要小心 NULL deref。
- 移除 tail 時要更新 `log->tail = cur->prev`。

通常會寫個 `link_after(prev, new)` / `unlink(node)` helper 集中處理。

---

## 練習 3：反轉整個串列

```c
void track_log_reverse(track_log_t *log)
{
    track_node_t *prev = NULL;
    track_node_t *cur  = log->head;
    log->tail = cur;                 /* 反轉後原本的 head 變 tail */
    while (cur != NULL) {
        track_node_t *next = cur->next;   /* 暫存下一步 */
        cur->next = prev;                  /* 翻轉指向 */
        prev = cur;
        cur  = next;
    }
    log->head = prev;                 /* prev 最後停在原本的 tail (新 head) */
}
```

視覺化：

```
原本:  head → A → B → C → NULL
                       ↑ tail

迭代 1: prev=NULL, cur=A:  A->next=NULL
迭代 2: prev=A, cur=B:     B->next=A
迭代 3: prev=B, cur=C:     C->next=B
結束:   prev=C (new head)

反轉:  head → C → B → A → NULL
                       ↑ tail
```

**重點**：用三指標 (prev/cur/next) 滾動更新，是 linked list 反轉的標準解。
這是面試常考題，動手寫一遍勝過讀十遍。

---

## 練習 4：為什麼 `remove_if` 結尾要更新 `tail`？

回顧程式碼：

```c
size_t track_log_remove_if(track_log_t *log, ...)
{
    size_t removed = 0;
    track_node_t **pp = &log->head;
    track_node_t  *prev_kept = NULL;

    while (*pp != NULL) {
        track_node_t *cur = *pp;
        if (pred(&cur->fix, ud)) {
            *pp = cur->next;
            free(cur);
            removed++;
        } else {
            prev_kept = cur;
            pp = &cur->next;
        }
    }
    log->tail = prev_kept;        /* ← 這行 */
    return removed;
}
```

**如果拿掉這行**：

情境：原本串列 `head → A → B → C`，`tail = C`。
若 `C` 被移除，迴圈結束時 `head → A → B`，但 `log->tail` 還指向已釋放的 `C`。

後續 `track_log_append`：

```c
if (log->tail == NULL) { ... } else { log->tail->next = node; }
```

→ 透過懸空指標 `C` 寫入它的 `next` 欄位 → use-after-free，多半 segfault 或踩到別的記憶體。

**這就是測試 `移除 head (特例)` 在驗證的事**：
不只要刪對，刪完之後 tail 仍然要正確指向「最後留下的節點」，後續 append 才不會壞。

> 教訓：**結構體的不變量 (invariant) 在每個 mutate 函式結束時都要重新成立**。
> 對 `track_log_t` 來說，不變量是 `tail` 永遠指向最後一個節點（空串列時為 NULL）。

---

## 關鍵概念回顧

- `**` 在 linked list 移除節點時極度好用 — 統一處理 head 與中間節點。
- 反轉、走訪、移除都是「三指標滾動」或「指向指標的指標」的應用。
- 嵌入式情境下要警覺 heap 碎片化，pool allocator 比 malloc 適合。
