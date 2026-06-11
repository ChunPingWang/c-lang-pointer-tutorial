/*
 * STM32 整合範例 (示意, 未在實機驗證)
 * --------------------------------------------------------
 * 假設 STM32CubeMX 已產生:
 *   - USART1 (RX 中斷已啟用)
 *   - SysTick 預設 1ms
 *
 * 你需要在 CubeMX 產生的 main.c 對應位置插入下面程式碼。
 * 標 [PASTE] 的區段請依註解貼入。
 */

#include "main.h"            /* CubeMX 產生 */
#include "usart.h"           /* CubeMX 產生 */
#include "gps/ring_buffer.h"
#include "gps/nmea_parser.h"

/* --- 全域狀態 ---------------------------------------------------------- */

#define RX_BUF_SIZE 256

static uint8_t       g_rx_storage[RX_BUF_SIZE];
static ring_buffer_t g_rx_buffer;
static nmea_parser_t g_parser;
static uint8_t       g_isr_byte;        /* HAL 收 1 byte 用的暫存 */

extern UART_HandleTypeDef huart1;        /* CubeMX 在 usart.c 宣告 */

/* --- 應用層 callback --------------------------------------------------- */

static void app_on_fix(const gps_fix_t *fix, void *user_data)
{
    (void)user_data;
    /* 真實專案: 透過 ITM/printf 或顯示器送出 */
    /* printf 在 STM32 上需要 retarget _write 到 ITM 或 UART */
    if (fix->valid) {
        /* 例: 把 latlon 透過第二個 USART 送出, 或更新 LCD */
    }
}

/* --- HAL 中斷回呼: 每收到 1 byte 進來 ---------------------------------- */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* 在 ISR 內快速把 byte 推進 ring buffer, 不做解析 */
        ring_buffer_push(&g_rx_buffer, g_isr_byte);

        /* 重新 arm 下一個 byte 的接收 */
        HAL_UART_Receive_IT(&huart1, &g_isr_byte, 1);
    }
}

/* --- 應用初始化 -------------------------------------------------------- */

static void gps_pipeline_init(void)
{
    ring_buffer_init(&g_rx_buffer, g_rx_storage, sizeof(g_rx_storage));
    nmea_parser_init(&g_parser, app_on_fix, NULL);
    HAL_UART_Receive_IT(&huart1, &g_isr_byte, 1);   /* 啟動第一個 RX */
}

/* --- 主迴圈: 從 ring buffer 拉 byte, 餵 parser -------------------------- */

static void gps_pipeline_pump(void)
{
    uint8_t byte;
    /* 把 ring buffer 內目前所有 byte 都處理完 */
    while (ring_buffer_pop(&g_rx_buffer, &byte)) {
        nmea_parser_feed_byte(&g_parser, (char)byte);
    }
}

/* --- 以下是在 CubeMX 產生的 main() 裡的整合點 -------------------------- */

int main_stm32_example(void)
{
    /* CubeMX 會生成 HAL_Init / SystemClock_Config / MX_USART1_UART_Init() */
    /* HAL_Init(); */
    /* SystemClock_Config(); */
    /* MX_GPIO_Init(); */
    /* MX_USART1_UART_Init(); */

    /* [PASTE] 在 USER CODE BEGIN 2 區段插入: */
    gps_pipeline_init();

    /* [PASTE] 在 while(1) 迴圈中插入: */
    while (1) {
        gps_pipeline_pump();
        /* 其他任務、或 HAL_Delay / WFI 進入低功耗 */
    }
}
