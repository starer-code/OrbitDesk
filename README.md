# OrbitDesk

> Windows 桌面效率工具 — 集成 8 大功能模块，AI 驱动，Catppuccin Mocha 暗色主题

基于 **Qt 6 (C++)** 构建的桌面效率工具，集成系统监控、任务管理、快捷笔记、番茄钟、AI 对话、工作流启动六大核心功能，支持 AI 流式输出、Markdown 渲染、多会话并发、历史趋势图等高级特性。

---

## 功能特性

### 系统监控
- 实时监控 CPU、内存、磁盘使用率
- 网络上传/下载速度统计
- **历史趋势图**：QPainter 自绘最近 1 小时 CPU/内存折线图
- 显示主机名、操作系统版本、运行时间

### 任务管理
- 任务增删改查，支持优先级（低/中/高/紧急）
- 分类管理、截止日期设置
- **到期高亮**：已过期红色、24 小时内黄色
- **专注时长统计**：关联番茄钟，显示每个任务的累计专注时间

### 快捷笔记
- 笔记创建、编辑、删除、置顶
- 关键词搜索
- 标签分类

### 番茄钟
- 专注/休息状态切换，环形进度条倒计时
- **自定义时长**：可调整专注和休息时长（默认 25+5 分钟）
- **关联任务**：选择任务后启动专注，自动记录到任务下
- 每日专注统计

### AI 对话
- **流式输出**：SSE 逐字显示，实时渲染
- **Markdown 渲染**：标题、粗体、代码块、列表、链接
- **多会话并发**：独立消息历史，切换会话不丢失上下文
- **上下文管理**：6000 token 自动裁剪，防止长对话崩溃
- 气泡式聊天界面，用户/AI 消息左右布局

### 工作流启动
- 卡片式布局，一键启动外部程序
- **右键菜单**：编辑、上移、下移、删除
- **图标显示**：自动从 exe 提取原生图标
- **拖拽排序**：调整工作流顺序
- 支持 URL 和本地程序

### 其他特性
- Catppuccin Mocha 暗色主题（QSS 全局样式）
- 无边框窗口，自定义标题栏拖拽移动
- 系统托盘支持，关闭窗口最小化到托盘
- 单实例检测（QSharedMemory + QLocalSocket）
- SQLite 数据持久化

---

## 环境变量

| 变量名 | 说明 | 必填 |
|---|---|---|
| `ORBITDESK_API_KEY` | AI API 密钥 | ✅ |
| `ORBITDESK_API_URL` | AI API 地址 | ❌（默认 MiMo API） |

设置方法（Windows）：
```powershell
# 临时设置（当前终端）
set ORBITDESK_API_KEY=your_api_key

# 永久设置（系统环境变量）
setx ORBITDESK_API_KEY "your_api_key"
setx ORBITDESK_API_URL "https://your-api-url/v1/chat/completions"
```

---

## 技术栈

| 类别 | 技术 |
|---|---|
| 编程语言 | C++ (Qt 6.9.2) |
| 构建系统 | CMake 3.19+ |
| Qt 模块 | Core, Widgets, Sql, Network |
| 数据库 | SQLite |
| 主题 | Catppuccin Mocha (QSS) |
| 平台 | Windows 10/11 |
| 图标 | SVG (Qt 资源) |
| 系统 API | Win32 API (iphlpapi, kernel32) |

---

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/starer-code/OrbitDesk.git
cd OrbitDesk
```

### 2. 打开项目

- **Qt Creator**：文件 → 打开文件或项目 → 选择 `CMakeLists.txt`
- **命令行**：
  ```bash
  mkdir build && cd build
  cmake .. -G "MinGW Makefiles"
  cmake --build .
  ```

### 3. 设置环境变量（AI 功能必需）

```powershell
# Windows PowerShell
$env:ORBITDESK_API_KEY="your_api_key"

# 或永久设置
setx ORBITDESK_API_KEY "your_api_key"
```

> ⚠️ 不设置 API Key 也能运行，但 AI 对话功能不可用

### 4. 运行

- Qt Creator：点击 ▶ 运行按钮
- 命令行：`./build/OrbitDesk.exe`
- 双击：`build/OrbitDesk.exe`

---

## 构建与运行

### 前置条件
- Qt 6.5+（推荐 6.9.2）
- MinGW 或 MSVC 编译器
- CMake 3.19+

### 构建步骤

```bash
# 方式一：Qt Creator
# 打开 CMakeLists.txt，选择 Kit，Ctrl+B 构建

# 方式二：命令行
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### 运行

```bash
# 命令行运行
./build/OrbitDesk.exe

# 或直接双击 build/OrbitDesk.exe
```

---

## 项目结构

```
OrbitDesk/
├── main.cpp                      # 程序入口，单实例检测，主题加载
├── mainwindow.cpp/h/ui           # 主窗口，所有 UI 逻辑
├── database_manager.cpp/h        # SQLite 单例，8 张表管理
│
├── system_monitor.cpp/h          # CPU/内存/磁盘/网络监控（Windows API）
├── monitor_page.cpp/h            # 监控仪表盘 UI
├── history_chart.cpp/h           # 历史趋势图（QPainter 自绘）
│
├── task_manager.cpp/h            # 任务增删改查
├── note_manager.cpp/h            # 笔记增删改查
├── pomodoro_manager.cpp/h        # 番茄钟状态机（4 状态）
├── circular_progress.cpp/h       # 环形进度条控件（QPainter 自绘）
│
├── ai_chat_manager.cpp/h         # AI 对话 HTTP 客户端（SSE 流式）
├── chat_session_manager.cpp/h    # 多会话管理
│
├── workflow_manager.cpp/h        # 工作流快捷启动
│
├── dark.qss                      # Catppuccin Mocha 主题样式
├── CMakeLists.txt                # CMake 构建配置
├── app.rc                        # Windows 资源文件（图标）
│
├── resources/
│   └── icons/
│       ├── app.png               # 应用图标
│       ├── app.ico               # Windows exe 图标
│       ├── monitor.svg           # 系统监控导航图标
│       ├── task.svg              # 任务管理导航图标
│       ├── note.svg              # 快捷笔记导航图标
│       ├── pomodoro.svg          # 番茄钟导航图标
│       ├── chat.svg              # AI 对话导航图标
│       └── launch.svg            # 工作流启动导航图标
│
├── CLAUDE.md                     # 项目文档（AI 开发规范）
├── 项目开发总结.md                # 开发总结文档
├── OrbitDesk功能优化与扩展规划.md  # 功能规划文档
└── TEST_CASES.md                 # 测试用例
```

---

## 数据库设计

| 表名 | 用途 | 关键字段 |
|---|---|---|
| `tasks` | 任务管理 | title, priority, category, due_date, is_done |
| `notes` | 快捷笔记 | title, content, tags, is_pinned |
| `pomodoro_logs` | 番茄钟日志 | duration_min, task_id (FK), started_at |
| `chat_sessions` | AI 会话 | title, created_at, updated_at |
| `chat_history` | 聊天记录 | role, content, session_id |
| `monitor_stats` | 监控数据 | cpu_percent, memory_percent, recorded_at |
| `user_settings` | 用户配置 | key, value |
| `workflows` | 工作流 | name, command, icon_path, sort_order |

数据库路径：`QStandardPaths::AppDataLocation/orbitdesk.db`

---

## 架构设计

```
main.cpp (入口 + 单实例检测)
  │
  └── MainWindow (控制器，持有所有 Manager)
        │
        ├── MonitorPage
        │     ├── SystemMonitor (Windows API)
        │     ├── CircularProgress × 3 (仪表盘)
        │     └── HistoryChart (趋势图)
        │
        ├── TaskManager → DatabaseManager
        ├── PomodoroManager → QTimer + 信号
        ├── AiChatManager → QNetworkAccessManager (SSE)
        ├── ChatSessionManager → DatabaseManager
        ├── NoteManager → DatabaseManager
        ├── WorkflowManager → DatabaseManager + QProcess
        ├── QSystemTrayIcon + QMenu
        └── QLocalServer (IPC)
```

**设计模式**：
- 单例模式：DatabaseManager
- 观察者模式：Qt 信号槽
- 状态机：PomodoroManager（Idle/Focusing/Paused/Resting）
- 事件过滤器：eventFilter() 统一处理拖拽、表格选中、工作流点击

---

## 已知问题

- 窗口最大化后恢复位置可能有偏差（已基本修复）
- 部分 Windows 11 版本下无边框窗口拖拽有延迟

---

## 开发日志

### 2026.06.07 更新
- AI 流式输出（SSE 逐字显示）
- AI Markdown 渲染
- 多会话并发（独立消息历史）
- AI 上下文窗口管理（6000 token 自动裁剪）
- 番茄钟关联任务 + 自定义时长
- 系统监控历史趋势图
- 工作流右键菜单（编辑/排序/图标）
- 任务到期高亮 + 优先级中文化
- 侧边栏导航图标
- 修复 8 个 Bug

---

## 许可证

MIT License

---

## 作者

[starer-code](https://github.com/starer-code)
