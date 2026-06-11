#ifndef TRACK_LOG_H
#define TRACK_LOG_H

#include <stddef.h>
#include <stdbool.h>

#include "gps/gps_types.h"

/*
 * GPS 軌跡記錄 — 用單向鏈結串列實作。
 *
 * 為什麼是 linked list 而不是動態陣列?
 *   - 走航線時不知道會記多少筆 → 不必預先 reserve。
 *   - 各節點獨立配置, 不會有 realloc 搬家成本。
 *   - 但代價: 每節點多 sizeof(void*) bytes overhead, cache 不友善, 不能 O(1) 隨機存取。
 *
 * 配置策略: 每個 track_node_t 由 track_log_append 在 heap 配置, 由 track_log_destroy
 * 統一釋放。所有權: track_log_t 擁有所有節點。
 */

typedef struct track_node {
    gps_fix_t          fix;
    struct track_node *next;
} track_node_t;

typedef struct {
    track_node_t *head;
    track_node_t *tail;     /* 為了 O(1) append */
    size_t        length;
} track_log_t;

void   track_log_init(track_log_t *log);
bool   track_log_append(track_log_t *log, const gps_fix_t *fix);
size_t track_log_length(const track_log_t *log);
void   track_log_destroy(track_log_t *log);

/* 走訪: 由呼叫端對每個 node 呼叫一次 visitor。
 * 例: 印出整個 log:
 *     track_log_foreach(log, print_fix, NULL); */
typedef void (*track_visitor_t)(const gps_fix_t *fix, size_t index, void *user_data);
void track_log_foreach(const track_log_t *log, track_visitor_t visit, void *user_data);

/* 從串列中移除符合條件的節點 (例如 invalid 的 fix)。
 * predicate 回傳 true 表示「移除這個」。回傳被移除的節點數。 */
typedef bool (*track_predicate_t)(const gps_fix_t *fix, void *user_data);
size_t track_log_remove_if(track_log_t *log, track_predicate_t pred, void *user_data);

#endif
