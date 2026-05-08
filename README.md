# OrbitDesk

Windows 桌面效率工具，基于 Qt 6 + C++ 构建，集成六大功能模块。

## 功能模块

- **番茄钟** - 番茄工作法计时器，带环形进度条，自动记录专注日志
- **任务管理** - 任务的增删改查，支持优先级、分类、截止日期
- **快捷笔记** - 笔记的增删改查，支持置顶和搜索
- **AI 对话** - 气泡式聊天界面，支持上下文对话
- **系统监控** - 实时监控 CPU、内存等系统资源
- **工作流启动** - 快捷启动外部程序，卡片式管理

## 其他特性

- 深色主题 (Catppuccin Mocha 配色)
- 无边框窗口，支持拖拽移动
- 系统托盘，关闭窗口时最小化到托盘
- 数据持久化 (SQLite)

## 技术栈

- **语言**: C++
- **框架**: Qt 6.5+
- **构建**: CMake
- **Qt 模块**: Core, Widgets, Sql, Network
- **平台**: Windows

## 构建

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## 运行

```bash
./build/OrbitDesk.exe
```

## 项目结构

```
OrbitDesk/
├── main.cpp                 # 程序入口
├── mainwindow.*             # 主窗口
├── database_manager.*       # 数据库管理
├── task_manager.*           # 任务管理
├── pomodoro_manager.*       # 番茄钟
├── circular_progress.*      # 环形进度条控件
├── ai_chat_manager.*        # AI 对话
├── system_monitor.*         # 系统监控
├── monitor_page.*           # 监控页面
├── note_manager.*           # 笔记管理
├── workflow_manager.*       # 工作流管理
├── dark.qss                 # 深色主题样式
├── CMakeLists.txt           # 构建配置
└── resources/               # 图标资源
```
