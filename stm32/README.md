# STM32 對照版

> 本目錄存放可直接搬到 STM32CubeIDE 專案的程式碼骨架，**示範如何把 host 端驗證過的 `gps_core` 真正掛到 MCU 上**。

⚠️ 注意：這份骨架的解析核心是與 host 端**共用同一份程式碼**的 (`src/gps/*.c`)，但 HAL 與時序部分**未在實體板子上驗證** — 你需要自行燒錄測試。

---

## 為什麼這樣分？

整個 GPS pipeline 拆成兩塊：

```
┌─────────────────────────┐    ┌─────────────────────────┐
│  與 MCU 無關的純邏輯     │    │  與 MCU 強相關的部分      │
│  (host 上跑單元測試)     │    │  (只能在板子上跑)         │
├─────────────────────────┤    ├─────────────────────────┤
│ src/gps/ring_buffer.c   │    │ stm32/main_stm32.c      │
│ src/gps/nmea_parser.c   │    │ HAL_UART_RxCpltCallback │
│ include/gps/*.h         │    │ stm32xx_hal_msp.c       │
└─────────────────────────┘    └─────────────────────────┘
        ↑                              ↑
        └──────────── 共用 ─────────────┘
```

這就是「把純邏輯與 IO 邊界分開」的好處：

- 80% 程式碼可以在 host 上用 CMake + CTest 自動測試。
- 真正要上板的時候，只要把 HAL 那一層接好就行。

---

## 整合步驟

### 1. 用 STM32CubeMX 產生專案骨架

選你的 MCU (例如 STM32F411RE)，在 `Connectivity` 開啟一個 USART:

- 鮑率：9600 (大多數 GPS 模組預設) 或 38400
- Word length: 8 bit
- Stop bits: 1
- Parity: None
- 啟用 **USART global interrupt**

點 `Generate Code`。

### 2. 把 `gps_core` 程式碼拷進去

| 從本專案的                  | 拷到 CubeIDE 專案的            |
| --------------------------- | ------------------------------ |
| `include/gps/*.h`           | `Core/Inc/`                    |
| `src/gps/ring_buffer.c`     | `Core/Src/`                    |
| `src/gps/nmea_parser.c`     | `Core/Src/`                    |
| **不要** 拷 `src/hal/`      | 用 stm32/main_stm32.c 取代     |

### 3. 在 `main.c` 接上去

參考 `stm32/main_stm32.c` 的程式碼片段，把 `nmea_parser` 與 ring buffer 整合到 CubeMX 產生的 `main.c` 裡。

關鍵點：

- ISR 推 byte：`HAL_UART_RxCpltCallback` 把收到的 byte `ring_buffer_push` 進去。
- 主迴圈拉 byte：`while (ring_buffer_pop(...)) { nmea_parser_feed_byte(...); }`
- 注意 `ring_buffer_t::head/tail` 已經是 `volatile`，無需額外設定。

---

## 接腳建議 (以 STM32F411RE Nucleo + NEO-6M 模組為例)

| Nucleo  | NEO-6M | 說明                |
| ------- | ------ | ------------------- |
| 3V3     | VCC    | NEO-6M 是 3.3V 模組 |
| GND     | GND    |                     |
| PA10 (USART1_RX) | TX | GPS 模組 TX → MCU RX |
| PA9  (USART1_TX) | RX | (可不接, 不需發送)   |

---

## 為什麼 host 驗證 + MCU 整合的兩階段？

對學習者：

1. **指標 bug 在 host 抓比 MCU 抓快 100 倍**：host 有 valgrind/ASan、可以 printf、可以 gdb。
2. **每次只改一件事**：在 host 跑通邏輯 → 換掉 IO 層接 HAL → 在板上 debug HAL 設定。
3. **CI 自動化**：未來如果要持續整合，host 部分可以接 GitHub Actions / GitLab CI；MCU 部分通常只能手動跑。

這個工程模式叫做 **hexagonal architecture** 或 **ports and adapters**，在 IoT / 嵌入式很常見。

---

## 進階：用 `idle line detection + DMA` 取代逐 byte 中斷

正式專案上 GPS 9600 baud 還能用逐 byte ISR，但更高鮑率或要省 CPU 時，會改用「DMA + UART IDLE line interrupt」。
`gps_core` 的設計能直接相容 — 只要把整段 DMA buffer 在 IDLE callback 裡 `ring_buffer_push` 進去即可，parser 端完全不需改動。

這就是「把純邏輯與 IO 邊界分開」的價值。
