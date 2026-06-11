#include "test_helpers.h"
#include "gps/track_log.h"

static gps_fix_t make_fix(double lat, double lon, bool valid)
{
    gps_fix_t f = {0};
    f.latitude  = lat;
    f.longitude = lon;
    f.valid     = valid;
    return f;
}

typedef struct { size_t count; double last_lat; } visit_ctx_t;

static void visit_cb(const gps_fix_t *fix, size_t index, void *user_data)
{
    visit_ctx_t *ctx = (visit_ctx_t *)user_data;
    ctx->count++;
    ctx->last_lat = fix->latitude;
    (void)index;
}

static bool is_invalid(const gps_fix_t *fix, void *user_data)
{
    (void)user_data;
    return !fix->valid;
}

int main(void)
{
    printf("Testing track_log...\n");

    track_log_t log;
    track_log_init(&log);

    TEST_CASE("init 後 length=0");
    EXPECT_EQ_INT(track_log_length(&log), 0);

    TEST_CASE("append 3 筆");
    EXPECT(track_log_append(&log, &(gps_fix_t){.latitude = 1.0, .valid = true}));
    EXPECT(track_log_append(&log, &(gps_fix_t){.latitude = 2.0, .valid = false}));
    EXPECT(track_log_append(&log, &(gps_fix_t){.latitude = 3.0, .valid = true}));
    EXPECT_EQ_INT(track_log_length(&log), 3);

    TEST_CASE("foreach 走訪順序");
    visit_ctx_t ctx = {0};
    track_log_foreach(&log, visit_cb, &ctx);
    EXPECT_EQ_INT(ctx.count, 3);
    EXPECT_NEAR(ctx.last_lat, 3.0, 0.001);

    TEST_CASE("remove_if 移除 invalid");
    size_t removed = track_log_remove_if(&log, is_invalid, NULL);
    EXPECT_EQ_INT(removed, 1);
    EXPECT_EQ_INT(track_log_length(&log), 2);

    TEST_CASE("移除後走訪正確");
    ctx.count = 0;
    track_log_foreach(&log, visit_cb, &ctx);
    EXPECT_EQ_INT(ctx.count, 2);

    TEST_CASE("移除 head (特例)");
    track_log_t log2;
    track_log_init(&log2);
    track_log_append(&log2, &(gps_fix_t){.latitude = 10.0, .valid = false});
    track_log_append(&log2, &(gps_fix_t){.latitude = 20.0, .valid = true});
    track_log_remove_if(&log2, is_invalid, NULL);
    EXPECT_EQ_INT(track_log_length(&log2), 1);
    /* 移除 head 後 tail 不能變壞: 再 append 應該還能正確接上 */
    track_log_append(&log2, &(gps_fix_t){.latitude = 30.0, .valid = true});
    EXPECT_EQ_INT(track_log_length(&log2), 2);
    track_log_destroy(&log2);

    TEST_CASE("移除全部");
    gps_fix_t bad = make_fix(0, 0, false);
    track_log_t log3;
    track_log_init(&log3);
    track_log_append(&log3, &bad);
    track_log_append(&log3, &bad);
    removed = track_log_remove_if(&log3, is_invalid, NULL);
    EXPECT_EQ_INT(removed, 2);
    EXPECT_EQ_INT(track_log_length(&log3), 0);
    EXPECT(log3.head == NULL);
    EXPECT(log3.tail == NULL);
    track_log_destroy(&log3);

    TEST_CASE("destroy 後 length=0, head/tail=NULL");
    track_log_destroy(&log);
    EXPECT_EQ_INT(track_log_length(&log), 0);
    EXPECT(log.head == NULL);
    EXPECT(log.tail == NULL);

    TEST_REPORT();
}
