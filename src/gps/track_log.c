#include "gps/track_log.h"

#include <stdlib.h>

void track_log_init(track_log_t *log)
{
    log->head   = NULL;
    log->tail   = NULL;
    log->length = 0;
}

bool track_log_append(track_log_t *log, const gps_fix_t *fix)
{
    track_node_t *node = (track_node_t *)malloc(sizeof(*node));
    if (node == NULL) {
        return false;
    }
    node->fix  = *fix;          /* 結構體複製 (深拷貝, 因為 gps_fix_t 沒指標欄位) */
    node->next = NULL;

    if (log->tail == NULL) {
        log->head = node;       /* 空串列 */
    } else {
        log->tail->next = node;
    }
    log->tail = node;
    log->length++;
    return true;
}

size_t track_log_length(const track_log_t *log)
{
    return log->length;
}

void track_log_destroy(track_log_t *log)
{
    track_node_t *cur = log->head;
    while (cur != NULL) {
        track_node_t *next = cur->next;     /* 先存下 next, 否則 free 完就找不到 */
        free(cur);
        cur = next;
    }
    log->head = log->tail = NULL;
    log->length = 0;
}

void track_log_foreach(const track_log_t *log, track_visitor_t visit, void *user_data)
{
    size_t i = 0;
    for (track_node_t *cur = log->head; cur != NULL; cur = cur->next) {
        visit(&cur->fix, i++, user_data);
    }
}

size_t track_log_remove_if(track_log_t *log, track_predicate_t pred, void *user_data)
{
    /*
     * 經典「指向指標的指標」技法:
     *   pp 永遠指向「目前要檢查的那個節點指標所在位置」 (head 或某個 next 欄位)。
     *   若不移除, pp 移到 (*pp)->next 的位址;
     *   若移除,  *pp = (*pp)->next, 然後 free 舊節點。
     *
     * 好處: 不需要為「移除 head」寫特例。
     */
    size_t removed = 0;
    track_node_t **pp = &log->head;
    track_node_t  *prev_kept = NULL;

    while (*pp != NULL) {
        track_node_t *cur = *pp;
        if (pred(&cur->fix, user_data)) {
            *pp = cur->next;
            free(cur);
            log->length--;
            removed++;
        } else {
            prev_kept = cur;
            pp = &cur->next;
        }
    }
    log->tail = prev_kept;
    return removed;
}
