# 3D Dino 專案編譯說明

## 系統需求
- CMake 3.12 或更新版本
- C++20 支援的編譯器：
  - Windows: Visual Studio 2019/2022 或 MinGW
  - Linux: GCC 8+ 或 Clang 10+
  - macOS: Xcode 12+ 或 Clang 10+
- OpenGL 3.3 或更新版本

## 編譯方式

### 方法一：使用VSCode （推薦）
如果有安裝VSCode + CMake Tools擴充功能：
1. 開啟專案資料夾
2. Ctrl+Shift+P → "CMake: Configure"
3. Ctrl+Shift+P → "CMake: Build"
4. F5執行


### 方法＝：使用命令列

#### Windows (PowerShell 或命令提示字元)
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

執行程式：
```bash
cd Release
./hello_window.exe
```

#### Linux/macOS (終端機)
```bash
mkdir build
cd build
cmake ..
make -j4
```

執行程式：
```bash
./hello_window
```

## 專案結構
```
computer_graphic/
├── src/
│   └── main.cpp          # 主程式
├── models/
│   └── dino.obj          # 3D模型檔案
├── dependencies/
│   ├── glfw/            # GLFW庫
│   └── glad/            # GLAD庫
├── CMakeLists.txt       # CMake配置檔
└── README.md           # 本說明檔
```

## 功能說明
- 滑鼠左鍵拖拽：旋轉物件
- 滾輪：縮放
- WASD鍵：移動視角
- ESC鍵：關閉程式