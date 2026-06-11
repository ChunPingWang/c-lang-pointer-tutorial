# 第 07 章 — 參考解答 / 後續學習指引

回到 [章節原文](../07-pitfalls.md).

第 7 章主要是地雷清單與「完課後可以做什麼」的建議。下面是這些建議的進一步討論。

---

## 完課建議 1：把 `stm32/` 對照版骨架實際燒一塊板子

最小硬體配置：

- STM32 Nucleo-F411RE（或任何有 USART 的 STM32）— 約 NT$500。
- u-blox NEO-6M 或 NEO-M8N GPS 模組 — 約 NT$200–500。
- 4 條杜邦線。

整合步驟見 [`stm32/README.md`](../../stm32/README.md)。

**第一個障礙**通常不是程式碼，而是：

- 接腳接錯（NEO-6M 是 3.3V TTL，**不能直接接 5V** 否則會燒模組）。
- 鮑率不對（NEO-6M 預設 9600，NEO-M8N 預設可能是 38400）。
- 室內收不到訊號 — 必須靠近窗邊或到室外。

開始燒之前先把 `gps_pipeline_pump` 內加一個 `HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin)` 確認主迴圈有跑。

---

## 完課建議 2：擴充 parser 支援 `$GPVTG` 或 `$GPGSV`

### `$GPVTG`（地面速度向量）格式：

```
$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
       ^^^^^   ^^^^^   ^^^^^   ^^^^^
       真北方向 磁北方向 節     公里/小時
```

擴充 `gps_types.h`：

```c
typedef struct {
    ...
    double true_track_deg;
    double speed_kmh;
} gps_fix_t;
```

在 `nmea_parser.c` 加：

```c
static gps_sentence_id_t identify_sentence(const char *type)
{
    ...
    if (strcmp(type + 2, "VTG") == 0) return GPS_SENTENCE_VTG;
    ...
}

static nmea_status_t parse_vtg(char **fields, size_t n, gps_fix_t *out)
{
    if (n < 8) return NMEA_ERR_BAD_FIELD;
    out->true_track_deg = atof(fields[1]);
    out->speed_kmh      = atof(fields[7]);
    return NMEA_OK;
}
```

別忘了在 `tests/test_nmea_parser.c` 加一筆 VTG 測試，確認沒有破壞 RMC/GGA 既有行為。

### `$GPGSV`（衛星詳細資訊）難度較高

一次 GSV 訊息只能塞 4 顆衛星，所以一筆完整資料會跨多個 sentence：

```
$GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
$GPGSV,3,2,11,14,25,170,00,16,57,208,39,18,67,296,40,19,40,246,00*74
$GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00*4D
```

要在 parser 裡維護「正在組裝的多句訊息狀態」，超出本教學範圍但是個好練習。

---

## 完課建議 3：lock-free SPSC ring buffer

目前的 `ring_buffer_push/pop` 是兩個獨立函式，head/tail 各自只被一邊更新（ISR 寫 head、主迴圈寫 tail）→ 已經是 SPSC（單生產者單消費者）lock-free 模式了。

唯一需要警覺的：

```c
size_t next = (rb->head + 1) % rb->capacity;
if (next == rb->tail) { return false; }
rb->buffer[rb->head] = byte;     // (a)
rb->head = next;                  // (b)
```

(a) 必須先於 (b)：消費者看到 head 更新時，資料必須已經寫好。
**在 ARM Cortex-M 單核**上單純 `volatile` 通常夠（CPU 不會 reorder 簡單存取）。
**Cortex-A / 多核** 上要加 `__DMB()` 記憶體屏障：

```c
rb->buffer[rb->head] = byte;
__DMB();                          // 確保資料寫入先 visible
rb->head = next;
```

或用 C11 atomic：

```c
#include <stdatomic.h>
atomic_size_t head;
atomic_store_explicit(&rb->head, next, memory_order_release);
```

---

## 第 7 章地雷的延伸思考

| 章節地雷             | 在 GPS 專案中如何被避免           |
| -------------------- | --------------------------------- |
| 7.1 野指標           | 所有結構體初始化 `= {0}`          |
| 7.2 懸空指標         | parser 不回傳指標, 由呼叫端給 buffer |
| 7.3 sizeof 退化      | `ring_buffer_init` 收 capacity 參數 |
| 7.4 buffer overflow  | `strncpy` + 強制補 `\0`           |
| 7.5 字串字面值       | 公開 API 用 `const char *`        |
| 7.6 二進位封包       | NMEA 是文字協定, 天然避開         |
| 7.7 use-after-free   | host main.c 的 `free(heap_buf)` 在程式結束才呼叫 |
| 7.11 ISR 缺 volatile | `ring_buffer_t::head/tail` 都是 volatile |

回顧時可以對著這張表審視自己的程式碼。

---

## 完課回顧

如果你能：

- 看著 `src/gps/nmea_parser.c` 立刻指出哪些參數是 in、哪些是 out。
- 解釋為什麼 `ring_buffer_t::head/tail` 必須 volatile。
- 講出 `char **fields` 在記憶體裡的長相。
- 答得出 `sizeof(arr)` 在陣列 vs 在函式參數位置為什麼不一樣。

那你已經跨過 C 指標的最大門檻。

接著去寫真正的 MCU 程式 — 那是讓所有概念落地的地方。
