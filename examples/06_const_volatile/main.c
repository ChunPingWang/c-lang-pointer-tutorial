#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/*
 * 嵌入式情境裡四種「const + 指標」的位置:
 *
 *   const T *p     -- 不能透過 p 改變 *p (常見於唯讀資料)
 *   T const *p     -- 同上 (語意一樣, 寫法不同)
 *   T *const p     -- p 自己不能改, 但 *p 可改 (常用於 MMIO 暫存器位址)
 *   const T *const -- 兩者都不能改
 */

static void hexdump(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int main(void)
{
    /* 假設這是一段從 GPS 拿到的封包資料 (唯讀使用) */
    uint8_t packet[] = { 0x24, 0x47, 0x50, 0x52, 0x4D, 0x43 };  /* "$GPRMC" */
    hexdump(packet, sizeof(packet));

    /* 模擬 STM32 暫存器位址 (此處只是示意, 真正硬體請小心 volatile)。
     * 在 host 上我們用一個普通變數來示範語法, 真正 MCU 程式碼會像:
     *     #define GPIOA_ODR (*(volatile uint32_t *)0x40020014)
     */
    volatile uint32_t fake_register = 0x12345678;
    volatile uint32_t * const reg_ptr = &fake_register;

    printf("讀暫存器: 0x%08X\n", *reg_ptr);
    *reg_ptr = 0xCAFEBABE;
    printf("寫入後再讀: 0x%08X\n", *reg_ptr);

    /* reg_ptr = NULL;  <-- 這行會編譯失敗, 因為 reg_ptr 是 T *const */

    return 0;
}
