# C 語言指標教學 — 用 STM32 + GPS 案例貫穿學習

> 一個給初學者的 C 指標教程。所有指標概念都用同一個專案案例：**從 STM32 透過 UART 收 GPS NMEA 字串，解析成定位結構**。

---

## 為什麼選 STM32 + GPS？

學指標最大的痛點是：教科書範例（`int a = 10; int *p = &a;`）太抽象，學完還是不知道指標「實際上」在哪裡用得到。
這份教材改用一個真實的嵌入式情境：

```
[GPS 模組] --UART--> [STM32 ring buffer] ---> [NMEA parser] ---> [定位結構 gps_fix_t]
                          ^^^^^^^^^^^^         ^^^^^^^^^^^^         ^^^^^^^^^^^^
                          uint8_t * + volatile char ** 字串拆解      struct * 回傳
```

光是這一條 pipeline 你就會碰到：

| 指標概念        | GPS pipeline 中的對應                         |
| --------------- | --------------------------------------------- |
| `&` / `*`       | 把區域變數的位址傳給 init 函式                |
| 陣列 ↔ 指標     | NMEA 字串 (`char buf[96]`) 拆欄位             |
| 結構體指標 `->` | `gps_fix_t *out` 給 parser 寫入結果            |
| 函式指標        | parser 完成時回呼 `on_fix(...)`               |
| 雙重指標 `**`   | `split_fields(char *line, char **out_fields)` |
| `const`         | 唯讀字串、唯讀結構參數                        |
| `volatile`      | UART ISR 改動的 ring buffer 索引              |
| `malloc`/`free` | host 端 `read_whole_file` 讀整檔到 heap       |
| Linked list     | `track_log_t` 儲存任意筆數的 GPS 軌跡         |

---

## 教學章節順序（建議從上往下讀）

| #   | 文件                                                | 重點                                             |
| --- | --------------------------------------------------- | ------------------------------------------------ |
| 00  | [CMake 建置系統詳解](docs/00-cmake-guide.md)        | 看懂 CMakeLists.txt 每一行，知道為什麼要分層     |
| 01  | [指標的本質：地址與解參考](docs/01-pointer-basics.md) | `&` 與 `*` 從零開始                             |
| 02  | [指標、陣列、字串](docs/02-array-string.md)         | NMEA 字串怎麼用指標走訪                          |
| 03  | [結構體與指標](docs/03-struct-pointer.md)           | `gps_fix_t *` 是怎麼傳出去的                    |
| 04  | [函式指標與回呼](docs/04-function-pointer.md)       | parser 解到一筆資料時，怎麼通知上層             |
| 05  | [雙重指標 `**`](docs/05-double-pointer.md)          | 拆 NMEA 欄位陣列：`char *fields[]`              |
| 06  | [`const` 與 `volatile`](docs/06-const-volatile.md)  | 為什麼 MCU 暫存器一定要 `volatile`              |
| 07  | [常見地雷](docs/07-pitfalls.md)                     | 野指標、懸空指標、`sizeof` 陷阱、字串字面值     |
| 08  | [動態記憶體](docs/08-dynamic-memory.md)             | `malloc`/`realloc`/`free`、擁有權、動態成長陣列 |
| 09  | [連結串列](docs/09-linked-list.md)                  | GPS track log 案例、`**` 在移除節點時的威力     |

---

## 專案結構

```
c-lang-pointer-tutorial/
├── CMakeLists.txt              -- 頂層建置設定
├── README.md                   -- 你正在看的這份
├── include/                    -- 對外標頭
│   ├── gps/                    --   ring_buffer.h, nmea_parser.h, gps_types.h
│   └── hal/                    --   uart_mock.h (host 上模擬 STM32 HAL)
├── src/
│   ├── gps/                    -- 解析核心 (host + STM32 共用)
│   ├── hal/                    -- HAL 模擬 (僅 host)
│   └── app/main.c              -- demo 程式入口
├── tests/                      -- CTest 單元測試
├── examples/                   -- 各章節的小範例 (可獨立執行)
├── stm32/                      -- STM32 對照版骨架 + 整合說明
├── docs/                       -- 七個指標章節
└── scripts/                    -- 輔助工具
```

---

## 一分鐘上手

### 必要工具

| 工具      | macOS                                 | Linux              | Windows                |
| --------- | ------------------------------------- | ------------------ | ---------------------- |
| C 編譯器  | `xcode-select --install`              | `apt install gcc`  | MSYS2 + MinGW          |
| CMake     | `brew install cmake`                  | `apt install cmake`| 官方安裝包             |
| Make/Ninja| 內建                                  | 內建               | 透過 MSYS2 / Ninja     |

### 建置與測試

```bash
# 從專案根目錄
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

預期看到 3 個測試全部通過：

```
1/3 Test #1: test_ring_buffer .................   Passed
2/3 Test #2: test_nmea_parser .................   Passed
3/3 Test #3: test_pointer_basics ..............   Passed
100% tests passed, 0 tests failed out of 3
```

### 跑主程式（模擬完整 GPS pipeline）

```bash
./build/gps_demo                          # 用內建的 demo 字串
./build/gps_demo data/sample_nmea.txt     # 從檔案讀 (示範 FILE* + malloc)
```

```
=== GPS pipeline demo (host build) ===
[fix #1] time=12:35:19  lat=48.117300  lon=11.516667  ...
[fix #2] time=12:35:19  lat=48.117300  lon=11.516667  sats=8  alt=545.4m  ...
=== total fixes parsed: 2 ===
```

### 跑某一章的小範例

```bash
./build/examples/ex_01_basics
./build/examples/ex_02_array_string
# ... 03 ~ 06
```

### 輔助工具：手算 NMEA checksum

```bash
./build/nmea_checksum "GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W"
# 輸出: *6A
```

### 練習解答

每章末尾的「動手練習」皆有參考解答，見 [`docs/exercises-solutions/`](docs/exercises-solutions/)。

---

## 在 STM32 真機上跑？

`src/gps/` 底下的程式碼 **完全不依賴任何 HAL**，可以直接複製到你的 STM32CubeIDE 專案的 `Core/Src/`。
唯一需要替換的是 `src/hal/uart_mock.c` — 在真機上你會改成 `HAL_UART_RxCpltCallback` 把 byte 推進 ring_buffer。

詳見 [`stm32/README.md`](stm32/README.md)。

---

## 學習建議

1. **先建置成功**：跟著「一分鐘上手」跑一遍，看到測試全綠。
2. **章節 → 範例 → 核心程式碼**：每讀完一章，跑對應的 `ex_XX`，再回頭看 `src/gps/` 裡相同概念的「真實」用法。
3. **動手改一行壞一行**：把 `nmea_parser.c` 裡的 `*` 拿掉一個、把 `&` 改成傳值，看編譯器怎麼罵你 — 那是最有效的學法。
4. **看完第 7 章「常見地雷」再去寫自己的程式**。

祝學習愉快。
