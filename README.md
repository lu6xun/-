# Grid Snake

使用 **C++17、raylib 和 CMake** 实现的经典网格贪吃蛇。包含完整的游戏流程、逐级加速和本地最高分，画面由几何图形与内置字体绘制，无外部素材依赖。

项目采用简单的模块划分，将游戏规则与窗口、输入、绘制和文件操作分离，便于阅读、维护和无窗口测试。

[快速开始](#快速开始) · [操作说明](#操作说明) · [设计与实现](#设计与实现) · [测试](#测试) · [已知限制](#已知限制)

## 功能

- **完整流程**：标题、游玩、暂停、失败与满盘胜利，支持重新开始和返回标题。
- **经典玩法**：24 × 20 网格，吃食物增长，碰墙或碰到自身结束。
- **逐级加速**：初始每 150 ms 移动一格，每吃 5 份食物减少 10 ms，最低 70 ms。
- **输入缓冲**：最多缓存两个转向，按计划方向校验，防止快速输入导致非法掉头。
- **本地记录**：最高分保存在用户数据目录，兼容缺失、损坏和读写失败。
- **可测试逻辑**：19 项 CTest 测试，不需要打开窗口或安装图形依赖即可验证核心规则。

## 快速开始

下载或克隆仓库后，在包含 `CMakeLists.txt` 的项目根目录执行以下命令。

### 环境要求

| 工具       | 要求                                                         |
| ---------- | ------------------------------------------------------------ |
| CMake      | 3.20 或更高版本                                              |
| C++ 编译器 | 支持 C++17 和 `std::filesystem`；Windows 首选 Visual Studio 2022 |
| raylib     | 固定为 5.5，由 CMake FetchContent 自动获取                   |
| 图形环境   | 桌面窗口环境及支持 OpenGL 3.3 的驱动                         |

首次完整配置需要联网。无需单独安装 raylib，离线构建方式见下文。

### Windows / Visual Studio 2022

安装 Visual Studio 2022 或 Build Tools 的“使用 C++ 的桌面开发”工作负载，包含 MSVC、Windows SDK 和 CMake 工具。

在 PowerShell 中执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
.\build\Release\grid_snake.exe
```

脚本会寻找 PATH 或 Visual Studio 附带的 CMake，配置 x64 Release、构建并运行测试。`ExecutionPolicy Bypass` 仅对当前 PowerShell 进程生效。构建调试版本可添加 `-Configuration Debug`。

若 CMake 和 CTest 已在 PATH 中，也可手动构建：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\grid_snake.exe
```

生成的 `build\GridSnake.sln` 可以直接用 Visual Studio 打开。MSVC 构建使用静态运行库，raylib 也以静态库链接。

<details>
<summary>Windows / MinGW-w64</summary>


推荐 MinGW-w64 GCC 12 或更新版本。将 `gcc`、`g++`、`mingw32-make` 和 CMake 加入 PATH：

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
.\build-mingw\grid_snake.exe
```

GCC 8 及更早版本不受支持；旧版 Windows 标准库缺少完整的 C++17 文件系统支持。不同生成器应使用不同构建目录。

</details>

<details>
<summary>Linux / macOS</summary>


需要支持 C++17 文件系统的编译器（GCC 9+ 或较新的 Clang / AppleClang）、CMake 及 Make 或 Ninja。

Linux 需要 raylib/GLFW 对应的图形开发包，参考 [raylib Linux 构建说明](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux)。macOS 需要 Xcode Command Line Tools，参考 [raylib macOS 构建说明](https://github.com/raysan5/raylib/wiki/Working-on-macOS)。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/grid_snake
```

Linux 和 macOS 尚未实际构建验证。

</details>

### 离线构建

在可联网的设备下载 [raylib 5.5 源码包](https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5)，解压到本机，例如 `D:\deps\raylib-5.5`。指定的目录应直接包含 `CMakeLists.txt` 和 `src/raylib.h`。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 -RaylibSource D:\deps\raylib-5.5
```

也可在 CMake 配置时传入 `-DFETCHCONTENT_SOURCE_DIR_RAYLIB=D:/deps/raylib-5.5`，随后按常规命令构建。

默认在线配置固定获取 raylib **5.5** 并验证 SHA-256；手动指定本地源码时也应使用该版本。压缩包校验值：

```text
aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
```

## 操作说明

| 输入           | 行为                             |
| -------------- | -------------------------------- |
| Enter          | 标题页开始；失败或胜利后返回标题 |
| 方向键 / WASD  | 转向                             |
| P / 空格       | 暂停或恢复                       |
| R              | 失败或胜利后重新开始             |
| Esc / 关闭窗口 | 退出                             |

初始蛇长为 3，蛇头位于地图中央并朝右。每份食物增加一格长度和 10 分；填满 480 格后获胜，最高正常得分为 **4770**。

蛇头、蛇身和食物使用不同颜色，蛇头的眼睛表示朝向。顶部显示本局分数、历史最高分、难度等级和移动间隔。失败后保留最后一个合法棋盘，并标记碰撞位置。

地图每格 24 像素，顶部状态栏高 88 像素，窗口客户区为 576 × 568。游戏界面使用英文，无需额外字体文件。

## 设计与实现

### 游戏循环与固定步进

主循环依次处理输入、更新逻辑、保存新纪录和绘制画面。目标渲染帧率为 60 FPS，蛇的移动由独立时间累积器控制。

`Game::update(dt)` 累积经过的时间，每够一个移动间隔就执行一步并扣除相应时间，剩余时间留到下一帧。每步重新检查游戏状态和难度，使速度变化从下一步生效。单帧最多计入 250 ms，避免长时间卡顿后突然连续补走；因此长卡顿时游戏时间可能落后于现实时间。

非游玩状态不累积时间。开始、重开、返回标题和暂停切换均清空计时器，状态切换所在帧也会丢弃旧的 `dt`。

### 转向缓冲

键盘事件通过 `GetKeyPressed()` 按顺序读取。转向队列最多保存两个方向，新输入与最后一个计划方向比较；队列为空时才与当前朝向比较。重复、反向和队列满时的新输入均被忽略。

例如朝右时快速输入“上、左”，会在两步中依次执行；输入“上、下”，第二个方向会被拒绝。每次实际移动最多消费一个转向。暂停期间拒绝方向输入，暂停切换时清空队列，避免遗留输入影响恢复后的移动。

### 碰撞与食物生成

蛇使用 `std::deque<Cell>` 保存，队首为头、队尾为尾。移动前先计算候选蛇头并判断是否吃到食物：

- **普通移动**：旧尾部同时离开，因此自身碰撞扫描排除旧尾格。
- **吃到食物**：尾部保留，碰撞扫描包含整个蛇身。

碰撞时记录候选格并保留原棋盘。合法移动插入新头，普通移动再移除队尾，增长时保留队尾并加分。

食物生成先标记占用格，再枚举空格并均匀抽取，复杂度为 O(地图格数 + 蛇长)。空格列表为空时返回 `std::nullopt` 并进入胜利状态，避免满盘时反复尝试随机坐标。

正常游戏使用随机种子，测试使用固定种子。同一标准库实现下随机序列可复现，不保证不同编译器产生完全相同的食物序列。

### 模块划分

```text
.
├── CMakeLists.txt
├── README.md
├── scripts/
│   └── build-windows.ps1       # Windows 配置、构建和测试
├── src/
│   ├── Types.h                # 坐标、方向和状态枚举
│   ├── Board.h / Board.cpp    # 地图边界与食物生成
│   ├── Game.h / Game.cpp      # 蛇、输入队列、计时和游戏规则
│   ├── Renderer.h / Renderer.cpp
│   ├── ScoreStorage.h / ScoreStorage.cpp
│   └── main.cpp               # 窗口、输入和主循环
└── tests/
    ├── GameTests.cpp
    └── ScoreStorageTests.cpp
```

`snake_core` 只依赖 C++ 标准库；`snake_storage` 独立处理文件读写；raylib 仅用于应用入口和绘制。使用标准库容器和 RAII 管理资源，无手动 `new/delete`。

## 最高分存储

最高分保存在用户数据目录，不写入源码目录：

| 平台    | 默认文件                                                     |
| ------- | ------------------------------------------------------------ |
| Windows | `%LOCALAPPDATA%\GridSnake\highscore.txt`                     |
| macOS   | `$HOME/Library/Application Support/GridSnake/highscore.txt`  |
| Linux   | `$XDG_DATA_HOME/GridSnake/highscore.txt`，未设置时使用 `$HOME/.local/share/GridSnake/highscore.txt` |

Windows 缺少 `LOCALAPPDATA` 时使用 `%USERPROFILE%\AppData\Local`，并通过 UTF-16 路径支持含中文的用户名。无法确定用户数据位置时仅在内存记录。

文件内容为一个非负整数，允许前后空白。缺失、空文件、负数、溢出、非法内容或读取失败均按 0 处理。新纪录出现时立即保存；保存失败会在界面提示，游戏继续运行，并在退出时重试一次。

## 测试

CTest 注册 **19 项测试**，包含 18 组游戏逻辑测试和 1 组存储测试。检查不依赖 `assert`，Debug 和 Release 均会执行。

| 范围       | 覆盖规则                                                     |
| ---------- | ------------------------------------------------------------ |
| 移动与输入 | 普通移动、增长加分、反向与重复输入、快速转向、队列容量和计划方向 |
| 碰撞与地图 | 四个边界、自身碰撞、进入即将离开的尾格、食物不重叠、满盘胜利 |
| 状态与计时 | 完整重置、暂停恢复、标题切换、固定步进、异常时间、难度递增及下限 |
| 随机与存储 | 固定种子、最高分读写、非法内容、溢出和读写失败               |

稀有局面由测试专用友元布置，再调用真实移动逻辑验证；存储测试使用独立临时目录，不修改玩家记录。

### 无窗口测试

关闭游戏目标后不会下载或链接 raylib：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 -Headless -Configuration Debug
```

Linux / macOS：

```bash
cmake -S . -B build-headless -DSNAKE_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure
```

运行单项测试（Visual Studio 构建）：

```powershell
ctest --test-dir build -C Release -R logic.departing_tail --output-on-failure
```

### 已验证环境

Windows x64，Visual Studio 2022 / MSVC 19.41，CMake 3.29.5：

- 无窗口 Debug 和完整 Release 构建成功，两种配置均通过 19/19 项测试。
- Windows 构建脚本已跑通配置、编译和测试流程，完整构建使用已校验的本地 raylib 5.5 源码。
- 已启动窗口并检查标题、游玩、得分变化、难度显示、P 键暂停及最高分写入。

Linux、macOS 和较新 MinGW 尚未实际构建验证；满盘胜利通过逻辑测试验证，未人工在窗口中完成。空格恢复、R 重开和退出按键仍需按下列清单逐项验收。

<details>
<summary>手动验收清单</summary>


- [ ] 标题显示操作说明，Enter 从中央、长度 3、朝右开始。
- [ ] 方向键与 WASD 均可转向，快速“上、左”分两步执行，直接反向无效。
- [ ] 吃食物增长并加 10 分，每 5 份食物加速一级。
- [ ] P 与空格均可暂停/恢复；暂停期间方向输入无效，恢复不连续补走。
- [ ] 撞墙或撞身显示结果与最终棋盘；R 重开，Enter 返回标题。
- [ ] 重开清空本局分数、速度和输入状态，保留最高分。
- [ ] 重启后仍能读取最高分；损坏的记录安全回退到 0。
- [ ] Esc 和关闭窗口按钮均可退出。

</details>

## 已知限制

- 窗口尺寸固定，尚未提供缩放、按键重映射、手柄、音效或续玩存档。
- 失去焦点时不会自动暂停，需按 P 或空格。
- 最高分是本地明文文件，没有防篡改、多进程协调或断电时的原子写入保证。
