#include "test_helpers.h"
#include "gps/ring_buffer.h"

int main(void)
{
    printf("Testing ring_buffer...\n");

    uint8_t storage[8];
    ring_buffer_t rb;
    ring_buffer_init(&rb, storage, sizeof(storage));

    TEST_CASE("init -> empty, not full");
    EXPECT(ring_buffer_is_empty(&rb));
    EXPECT(!ring_buffer_is_full(&rb));
    EXPECT_EQ_INT(ring_buffer_count(&rb), 0);

    TEST_CASE("push / pop single byte");
    EXPECT(ring_buffer_push(&rb, 0xAB));
    EXPECT_EQ_INT(ring_buffer_count(&rb), 1);
    uint8_t out = 0;
    EXPECT(ring_buffer_pop(&rb, &out));
    EXPECT_EQ_INT(out, 0xAB);
    EXPECT(ring_buffer_is_empty(&rb));

    TEST_CASE("fill to capacity-1 then full");
    for (int i = 0; i < 7; ++i) {
        EXPECT(ring_buffer_push(&rb, (uint8_t)i));
    }
    EXPECT(ring_buffer_is_full(&rb));
    EXPECT(!ring_buffer_push(&rb, 0x99));

    TEST_CASE("FIFO order preserved");
    for (int i = 0; i < 7; ++i) {
        uint8_t v;
        EXPECT(ring_buffer_pop(&rb, &v));
        EXPECT_EQ_INT(v, i);
    }

    TEST_CASE("wrap-around");
    ring_buffer_reset(&rb);
    for (int i = 0; i < 5; ++i) ring_buffer_push(&rb, (uint8_t)i);
    uint8_t tmp;
    for (int i = 0; i < 3; ++i) ring_buffer_pop(&rb, &tmp);
    for (int i = 5; i < 9; ++i) ring_buffer_push(&rb, (uint8_t)i);
    int seen = 0;
    while (ring_buffer_pop(&rb, &tmp)) {
        EXPECT_EQ_INT(tmp, 3 + seen);
        seen++;
    }
    EXPECT_EQ_INT(seen, 6);

    TEST_REPORT();
}
