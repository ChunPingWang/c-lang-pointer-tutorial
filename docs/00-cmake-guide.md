# 00 — CMake 建置系統詳解

> 在進入指標學習之前，你會需要會「建置」這個專案才能跑測試與範例。
> 這一章把整個專案的 `CMakeLists.txt` 一行一行拆給你看，並解釋為什麼要這樣分層。

---

## 0.1 為什麼用 CMake？不能直接 `gcc *.c` 嗎？

可以，但隨著檔案變多會痛苦：

- 「我只改了 `ring_buffer.c`，要重編所有東西嗎？」 → Make / Ninja 會幫你算依賴。
- 「我想在 macOS、Linux、Windows 都能編。」 → CMake 是 build system 的 build system，會幫你產出對應平台的檔案。
- 「我想分 host 版本和 STM32 版本。」 → CMake 可以用 toolchain file 切換。

簡單規則：**3 個以上 .c 檔，就值得用 CMake**。

---

## 0.2 三層 CMakeLists.txt

我們的專案有 3 個 `CMakeLists.txt`：

```
CMakeLists.txt              <-- 頂層 (project, library, demo)
├── tests/CMakeLists.txt    <-- 測試
└── examples/CMakeLists.txt <-- 章節範例
```

頂層用 `add_subdirectory(tests)` 把下層拉進來。這樣可以讓「測試是測試、範例是範例」各管各的。

---

## 0.3 頂層 `CMakeLists.txt` 逐行解說

打開根目錄的 `CMakeLists.txt`，對照下面說明。

### (1) 宣告 CMake 版本

```cmake
cmake_minimum_required(VERSION 3.16)
```

宣告「我這個專案需要 CMake 3.16 以上的功能」。3.16 是個保守且廣泛被支援的版本（Ubuntu 20.04 內建）。

### (2) 宣告專案

```cmake
project(c_pointer_tutorial
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "C pointer tutorial driven by an STM32 + GPS NMEA pipeline")
```

- `project(name ...)` 必填，CMake 內部會用 `${PROJECT_NAME}` 等變數。
- `LANGUAGES C` 告訴 CMake「不要找 C++ 編譯器」，節省 configure 時間並避免在沒裝 `g++` 的環境出錯。

### (3) C 語言標準

```cmake
set(CMAKE_C_STANDARD          11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS        OFF)
```

- 強制使用 C11（指標相關的章節有些範例用到了 designated initializer 等 C99/C11 才有的語法）。
- `OFF` 表示用 `-std=c11` 而不是 `-std=gnu11`，避免依賴 GCC 擴充語法。

### (4) 預設 build type

```cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
```

如果使用者沒指定 `-DCMAKE_BUILD_TYPE=Release`，預設用 Debug。
教學階段 Debug 比 Release 好，因為符號資訊完整、能配合 gdb 看指標。

### (5) Warning flags

```cmake
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wno-sign-conversion
)
```

寫指標時編譯器警告非常珍貴：未初始化、型別不對、隱式轉換⋯⋯都會幫你抓出來。
`-Wno-sign-conversion` 是因為 `size_t` 與 `int` 互轉太常見，全開太吵。

### (6) include path

```cmake
include_directories(${CMAKE_SOURCE_DIR}/include)
```

之後所有 target 都能寫 `#include "gps/ring_buffer.h"`。
（在更大的專案會建議改用 `target_include_directories()` 而不是全域 `include_directories()`，但教學專案這樣寫比較直覺。）

### (7) 把核心程式包成靜態函式庫

```cmake
add_library(gps_core STATIC
    src/gps/ring_buffer.c
    src/gps/nmea_parser.c
    src/hal/uart_mock.c
)
target_link_libraries(gps_core PUBLIC m)
```

`gps_core` 是一個 **靜態函式庫** (`.a`)，把所有解析邏輯打包起來。
這樣 `gps_demo`、每個 `ex_XX`、每個測試都可以連結同一份程式碼，**不會重複編譯**。

`PUBLIC m` 表示「我依賴 libm（數學函式庫，提供 `modf`），而且任何連結我的 target 也會自動連到 libm」。

> 對指標學習者來說：靜態函式庫的概念跟「把 `ring_buffer_init` 放進共用工具箱」差不多。記得，連結器最後會把所有 `.o` 串成一支可執行檔。

### (8) 主程式

```cmake
add_executable(gps_demo src/app/main.c)
target_link_libraries(gps_demo PRIVATE gps_core)
```

`gps_demo` 是個可執行檔，依賴 `gps_core`。`PRIVATE` 表示「這只是 gps_demo 自己的依賴，別人連結 gps_demo 不需要間接依賴 gps_core」。

### (9) 啟用測試與子目錄

```cmake
enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
```

- `enable_testing()` 必須在頂層呼叫，CTest 才會生成測試清單。
- `add_subdirectory(...)` 進入子目錄找對應的 `CMakeLists.txt`。

---

## 0.4 `tests/CMakeLists.txt`

```cmake
function(add_unit_test name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE gps_core)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(test_ring_buffer)
add_unit_test(test_nmea_parser)
add_unit_test(test_pointer_basics)
```

定義一個 `add_unit_test()` 函式，把「編譯 + 註冊測試」三行縮成一行。
每呼叫一次就會：

1. `add_executable` — 把 `tests/test_xxx.c` 編譯成執行檔。
2. `target_link_libraries` — 連到 `gps_core`，這樣測試裡可以 `#include "gps/..."`。
3. `add_test` — 註冊到 CTest，之後 `ctest` 會自動跑。

CTest 規則很單純：**程式 return 0 算過，return 非 0 算失敗**。

---

## 0.5 `examples/CMakeLists.txt`

跟測試類似，每個範例對應一個資料夾，目標名稱加上 `ex_` 前綴避免和未來的測試重名。

```cmake
function(add_example dir)
    set(target "ex_${dir}")
    add_executable(${target} ${dir}/main.c)
    target_link_libraries(${target} PRIVATE gps_core)
endfunction()

add_example(01_basics)
add_example(02_array_string)
...
```

---

## 0.6 完整建置流程

```bash
# 1. configure: 讀 CMakeLists.txt，產生 Makefile 到 build/
cmake -S . -B build

# 2. build: 跑 Makefile，編譯所有 target
cmake --build build

# 3. 跑測試
ctest --test-dir build --output-on-failure

# 4. 跑 demo / 範例
./build/gps_demo
./build/examples/ex_01_basics
```

### 想清乾淨重來？

```bash
rm -rf build
```

不要在原始碼資料夾裡放 `.o` 檔，**永遠用 out-of-source build**（所有產物都丟進 `build/`）。

### 想換成 Release 編譯（最佳化過）？

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

### 想用 Ninja 而不是 Make（更快）？

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

---

## 0.7 常見錯誤

| 錯誤訊息                                       | 通常意思                              |
| ---------------------------------------------- | ------------------------------------- |
| `CMake Error: source directory does not exist` | `-S` 路徑打錯了                       |
| `No CMAKE_C_COMPILER could be found`           | 沒裝 C 編譯器（macOS: 跑 xcode-select）|
| `undefined reference to 'modf'`                | 忘了 `target_link_libraries(... m)`   |
| `fatal error: 'gps/...' file not found`        | `include_directories` 沒設好          |

---

## 0.8 為什麼 CMake 與「指標」有關？

乍看無關，但其實有兩個重要連結：

1. **連結器** 最後會把所有 `.o` 裡的「函式符號」和「全域變數符號」對在一起。
   每個函式呼叫底層就是「跳到那個位址」 — 也就是函式指標的概念。
2. **共用靜態函式庫** 表示同一份 `ring_buffer_push` 程式碼，在 `gps_demo`、`test_ring_buffer`、`ex_01_basics` 裡是 **同一段機器碼**。
   你之後會在 第 4 章 看到，函式名稱在 C 裡其實就是「指向那段機器碼的指標」。

---

## 下一步

→ [01 — 指標的本質：地址與解參考](01-pointer-basics.md)
