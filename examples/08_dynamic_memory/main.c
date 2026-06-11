#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gps/gps_types.h"

/*
 * 動態成長的 fix 陣列 — 對應第 8 章「動態記憶體」的教學案例。
 *
 *   - capacity: 已配置的容量
 *   - size:     目前有效的元素數
 *   - items:    指向 heap 上的連續陣列 (可被 realloc 換掉)
 *
 * 注意 fix_array_t 自己很小 (只有 16~24 bytes), 但 items 指向的記憶體可能很大。
 */
typedef struct {
    gps_fix_t *items;
    size_t     size;
    size_t     capacity;
} fix_array_t;

static void fa_init(fix_array_t *fa)
{
    fa->items    = NULL;
    fa->size     = 0;
    fa->capacity = 0;
}

static bool fa_push(fix_array_t *fa, const gps_fix_t *fix)
{
    if (fa->size == fa->capacity) {
        size_t new_cap = (fa->capacity == 0) ? 4 : fa->capacity * 2;
        gps_fix_t *grown = realloc(fa->items, new_cap * sizeof(gps_fix_t));
        if (grown == NULL) {
            /* 注意: realloc 失敗時, 原本 fa->items 仍然有效 (不會被 free) */
            return false;
        }
        fa->items    = grown;
        fa->capacity = new_cap;
    }
    fa->items[fa->size++] = *fix;       /* struct copy */
    return true;
}

static void fa_destroy(fix_array_t *fa)
{
    free(fa->items);            /* free(NULL) 是合法 no-op, 不用先檢查 */
    fa->items    = NULL;
    fa->size     = 0;
    fa->capacity = 0;
}

int main(void)
{
    fix_array_t fa;
    fa_init(&fa);

    printf("初始: items=%p, size=%zu, capacity=%zu\n",
           (void *)fa.items, fa.size, fa.capacity);

    /* 推 10 筆 fix, 觀察 capacity 怎麼倍增 */
    for (int i = 0; i < 10; ++i) {
        gps_fix_t f = {
            .latitude   = 25.0 + i * 0.001,
            .longitude  = 121.5 + i * 0.001,
            .satellites = (uint8_t)(8 + i),
            .valid      = true,
        };
        gps_fix_t *prev_items = fa.items;
        fa_push(&fa, &f);
        printf("push #%d: items=%p%s, size=%zu, capacity=%zu\n",
               i + 1, (void *)fa.items,
               (prev_items != fa.items) ? " (realloc 搬家了!)" : "",
               fa.size, fa.capacity);
    }

    /* 走訪整個陣列 */
    printf("\n--- 走訪 ---\n");
    for (size_t i = 0; i < fa.size; ++i) {
        printf("[%zu] lat=%.4f lon=%.4f sats=%u\n",
               i, fa.items[i].latitude, fa.items[i].longitude,
               fa.items[i].satellites);
    }

    fa_destroy(&fa);
    printf("\n釋放完成。再次嘗試 destroy() 也是安全的 (free(NULL) no-op):\n");
    fa_destroy(&fa);
    printf("OK\n");

    return 0;
}
