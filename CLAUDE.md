# OrbitDesk

Qt 6 桌面效率工具，Catppuccin Mocha 暗色主题。

## Build

Qt Creator 中 Ctrl+B，或命令行：
```bash
cd build && cmake .. -G "MinGW Makefiles" && mingw32-make
```

## 文件结构

| 文件 | 职责 |
|---|---|
| main.cpp | 入口，主题加载，单实例检测（QLocalSocket + QSharedMemory） |
| mainwindow.cpp/h/ui | 主窗口，无边框，自定义标题栏，侧边栏导航，页面切换 |
| database_manager.cpp/h | SQLite 数据库（Meyer's 单例） |
| task_manager.cpp/h | 任务增删改查 |
| pomodoro_manager.cpp/h | 番茄钟状态机（Idle/Focusing/Paused/Resting） |
| circular_progress.cpp/h | 环形进度条（QPainter 自绘） |
| ai_chat_manager.cpp/h | AI 对话（MiMo API，OpenAI 兼容协议） |
| chat_session_manager.cpp/h | 多会话管理 |
| system_monitor.cpp/h | CPU/内存/磁盘/网络监控（Windows API） |
| monitor_page.cpp/h | 监控仪表盘 UI |
| note_manager.cpp/h | 快捷笔记 |
| workflow_manager.cpp/h | 工作流快捷启动 |
| dark.qss | Catppuccin Mocha 主题样式 |

## 数据库表

- tasks: id, title, description, priority, category, due_date, is_done
- notes: id, title, content, tags, is_pinned
- pomodoro_logs: id, duration_min, task_id, started_at, completed
- chat_sessions: id, title, created_at, updated_at
- chat_history: id, role, content, session_id
- monitor_stats: id, cpu_percent, memory_percent, memory_used_mb, disk_percent, net_up_kbps, net_down_kbps
- user_settings: key, value
- workflows: id, name, description, command, icon_path, sort_order

数据库路径：`QStandardPaths::AppDataLocation/orbitdesk.db`

## AI API

- 模型：`mimo-v2.5-pro`（全小写，大小写敏感）
- 协议：OpenAI 兼容
- 强制 HTTP/1.1：`Http2AllowedAttribute = false`（解决 SSL 认证问题）
- 环境变量：`ORBITDESK_API_KEY`，`ORBITDESK_API_URL`

## 编码规范

- 信号槽：Qt5 函数指针语法 `&Class::method`
- Manager 类：单例或普通 QObject，所有 DB 操作通过 `DatabaseManager::instance()`
- 样式：dark.qss 全局加载，组件样式用 `setObjectName` + QSS 选择器
- 单线程架构：无显式多线程，扩展时注意 DatabaseManager 线程安全

## 已知 Bug

- 窗口最大化后无法恢复原始状态（mainwindow.cpp:34 标注 bug2）

## 已修复（2026-06-05）

- AI JSON 解析防崩溃（ai_chat_manager.cpp）
- CMakeLists.txt 重复配置清理
- 建表逻辑统一到 DatabaseManager
- 删除死代码（loadChatHistory、note_manager::initDatabase）
- 番茄钟 pause/resume 状态记录修复
- 样式硬编码迁移到 dark.qss
- 工作流启动失败增加错误反馈

## 待开发

- 角色卡系统（智能体炼化 + 记忆系统）
- 聊天记录导入（复刻朋友聊天风格，方案见 memory/chat_clone_feature.md）

## 求职相关

- 应聘岗位：Qt 开发工程师（实习）
- 公司：电力行业 Qt 应用开发
- 对接人：赵德基（招聘 boss）
- 2026-06-04 技术面完成，等待下一轮面试通知
- 面试准备文档：`D:\projects\Myqt\新建文件夹\` 下的 .md 文件
