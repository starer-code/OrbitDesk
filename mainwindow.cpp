#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QButtonGroup>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSinglePointEvent>
#include <QModelIndex>
#include <QSqlQuery>
#include <QSqlError>
#include "database_manager.h"
#include <QScrollBar>
#include <QJsonObject>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QListWidget>
#include <QDialog>
#include <QMessageBox>
#include <QIcon>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSpinBox>
#include <QComboBox>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QTimer>
#include <QTextDocument>
#include <QTextCursor>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    menuBar()->hide();
    statusBar()->hide();
    setWindowFlag(Qt::FramelessWindowHint);
    m_normalGeometry = geometry();

    m_monitorPage = new MonitorPage(this);
    ui->monitorWrapperLayout->addWidget(m_monitorPage);
    m_monitorPage->startMonitoring();

    // 标题栏按钮
    connect(ui->btnMin, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->btnMax, &QPushButton::clicked, this, [this]() {
        if (isMaximized() || isFullScreen()) {
            showNormal();
            setGeometry(m_normalGeometry);
        } else {
            m_normalGeometry = geometry();
            showMaximized();
        }
    });
    connect(ui->btnClose, &QPushButton::clicked, this, [this]() {
        hide();
    });

    m_trayIcon = new QSystemTrayIcon(this);
    QIcon trayIcon(":/resources/icons/app.png");
    if (trayIcon.isNull()) {
        trayIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip("OrbitDesk");

    // 创建托盘菜单
    m_trayMenu = new QMenu(this);
    QAction *showAction = m_trayMenu->addAction("显示主窗口");
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction("退出");

    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
    });
    connect(quitAction, &QAction::triggered, this, [this]() {
        m_trayIcon->hide();
        qApp->quit();
    });

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            activateWindow();
        }
    });

    m_trayIcon->show();

    // 导航按钮
    connect(ui->btnNavMonitor, &QPushButton::clicked, this, &MainWindow::showMonitorPage);
    connect(ui->btnNavTask, &QPushButton::clicked, this, &MainWindow::showTaskPage);
    connect(ui->btnNavNote, &QPushButton::clicked, this, &MainWindow::showNotePage);
    connect(ui->btnNavPomodoro, &QPushButton::clicked, this, &MainWindow::showPomodoroPage);
    connect(ui->btnNavChat, &QPushButton::clicked, this, &MainWindow::showChatPage);
    connect(ui->btnNavLauncher, &QPushButton::clicked, this, &MainWindow::showLauncherPage);
    ui->btnNavMonitor->setCheckable(true);
    ui->btnNavTask->setCheckable(true);
    ui->btnNavNote->setCheckable(true);
    ui->btnNavPomodoro->setCheckable(true);
    ui->btnNavChat->setCheckable(true);
    ui->btnNavLauncher->setCheckable(true);

    // 侧边栏图标
    ui->btnNavMonitor->setIcon(QIcon(":/resources/icons/monitor.svg"));
    ui->btnNavTask->setIcon(QIcon(":/resources/icons/task.svg"));
    ui->btnNavNote->setIcon(QIcon(":/resources/icons/note.svg"));
    ui->btnNavPomodoro->setIcon(QIcon(":/resources/icons/pomodoro.svg"));
    ui->btnNavChat->setIcon(QIcon(":/resources/icons/chat.svg"));
    ui->btnNavLauncher->setIcon(QIcon(":/resources/icons/launch.svg"));

    // 图标大小
    QSize iconSize(22, 22);
    ui->btnNavMonitor->setIconSize(iconSize);
    ui->btnNavTask->setIconSize(iconSize);
    ui->btnNavNote->setIconSize(iconSize);
    ui->btnNavPomodoro->setIconSize(iconSize);
    ui->btnNavChat->setIconSize(iconSize);
    ui->btnNavLauncher->setIconSize(iconSize);

    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->btnNavMonitor, 0);
    navGroup->addButton(ui->btnNavTask, 1);
    navGroup->addButton(ui->btnNavNote, 2);
    navGroup->addButton(ui->btnNavPomodoro, 3);
    navGroup->addButton(ui->btnNavChat, 4);
    navGroup->addButton(ui->btnNavLauncher, 5);
    navGroup->setExclusive(true);

    ui->btnNavMonitor->setChecked(true);

    m_taskManager = new TaskManager(this);

    ui->taskTable->setColumnCount(6);
    ui->taskTable->setHorizontalHeaderLabels({"标题", "优先级", "分类", "截止日期", "专注时长", "状态"});
    ui->taskTable->setAlternatingRowColors(true);
    ui->taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->taskTable->verticalHeader()->hide();
    ui->taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->taskPriorityCombo->clear();
    ui->taskPriorityCombo->addItem("低", "low");
    ui->taskPriorityCombo->addItem("中", "medium");
    ui->taskPriorityCombo->addItem("高", "high");
    ui->taskPriorityCombo->addItem("紧急", "urgent");
    ui->taskPriorityCombo->setCurrentIndex(1);  // 默认"中"
    connect(ui->btnAddTask, &QPushButton::clicked, this, &MainWindow::addTask);
    connect(ui->btnDeleteTask, &QPushButton::clicked, this, &MainWindow::deleteTask);
    connect(ui->btnClearTask, &QPushButton::clicked, this, &MainWindow::clearInput);
    connect(ui->taskTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        int taskId = ui->taskTable->item(row, 0)->data(Qt::UserRole).toInt();
        m_taskManager->toggleDone(taskId);
        refreshTaskTable();
        refreshPomTaskCombo();
    });
    refreshTaskTable();
    //表单空白处取消选中
    qApp->installEventFilter(this);

    // 任务页面布局
    ui->gridLayout->setColumnStretch(0, 0);
    ui->gridLayout->setColumnStretch(1, 1);
    ui->gridLayout->setVerticalSpacing(8);
    ui->gridLayout->setHorizontalSpacing(10);

    ui->taskTitleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskPriorityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskDueDateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskCategoryLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskTitleLabel->setMaximumWidth(60);
    ui->taskPriorityLabel->setMaximumWidth(60);
    ui->taskDueDateLabel->setMaximumWidth(70);
    ui->taskCategoryLabel->setMaximumWidth(50);

    ui->taskTitleInput->setMinimumWidth(200);
    ui->taskTitleInput->setMaximumWidth(550);
    ui->taskCategoryInput->setMinimumWidth(120);
    ui->taskCategoryInput->setMaximumWidth(250);
    ui->taskPriorityCombo->setMaximumWidth(150);
    ui->taskDueDate->setDateTime(QDateTime::currentDateTime());

    ui->taskMainLayout->setAlignment(Qt::AlignHCenter);
    ui->taskButtonLayout->setSpacing(10);
    ui->taskButtonLayout->setContentsMargins(0, 10, 0, 10);

    ui->taskTable->setMinimumHeight(200);
    ui->taskTable->setMaximumHeight(16777215);
    ui->taskTable->verticalHeader()->setDefaultSectionSize(35);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->taskTable->setColumnWidth(1, 80);
    ui->taskTable->setColumnWidth(2, 80);
    ui->taskTable->setColumnWidth(3, 100);
    ui->taskTable->setColumnWidth(4, 80);
    ui->taskTable->setColumnWidth(5, 100);
    ui->contentLayout->setStretch(0, 1);
    ui->contentLayout->setStretch(1, 4);
    ui->taskContentWrapper->setMinimumWidth(550);
    ui->taskContentWrapper->setMaximumWidth(800);

    ui->taskFormCard->setObjectName("taskFormCard");
    ui->taskTableCard->setObjectName("taskTableCard");
    ui->taskListLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4;");
    // ===== 番茄钟 =====
    m_pomodoroManager = new PomodoroManager(this);
    connect(m_pomodoroManager, &PomodoroManager::stateChanged,
            this, &MainWindow::onPomodoroStateChange);
    connect(m_pomodoroManager, &PomodoroManager::tick,
            this, &MainWindow::onPomodoroTick);
    connect(m_pomodoroManager, &PomodoroManager::focusCompleted,
            this, &MainWindow::onPomodoroFocusCompleted);
    connect(m_pomodoroManager, &PomodoroManager::restCompleted,
            this, &MainWindow::onPomodoroRestCompleted);
    connect(ui->pomBtnStart, &QPushButton::clicked, this, [this]() {
        int taskId = m_pomTaskCombo->currentData().toInt();
        m_pomodoroManager->setCurrentTask(taskId);
        m_pomodoroManager->setFocusDuration(m_pomFocusSpin->value());
        m_pomodoroManager->setRestDuration(m_pomRestSpin->value());
        m_pomodoroManager->startFocus();
    });
    connect(ui->pomBtnPause, &QPushButton::clicked, this, [this]() {
        if (m_pomodoroManager->state() == Paused) {
            m_pomodoroManager->resume();
        } else {
            m_pomodoroManager->pause();
        }
    });
    connect(ui->pomBtnReset, &QPushButton::clicked, this, [this]() {
        m_pomodoroManager->reset();
    });

    ui->pomLogTable->setColumnCount(2);
    ui->pomLogTable->setHorizontalHeaderLabels({"开始时间", "专注时长(分钟)"});
    ui->pomLogTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->pomLogTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->pomLogTable->verticalHeader()->hide();

    ui->pomTimeLabel->hide();
    m_circularProgress = new CircularProgress(ui->pomWrapper);
    QVBoxLayout *pomLayout = qobject_cast<QVBoxLayout*>(ui->pomWrapper->layout());
    int stateLabelIndex = pomLayout->indexOf(ui->pomStateLabel);
    pomLayout->insertWidget(stateLabelIndex + 1, m_circularProgress);
    m_circularProgress->setMinimumSize(200, 200);
    m_circularProgress->setMaximumSize(280, 280);
    m_circularProgress->setTimeText("25:00");
    pomLayout->setSpacing(5);
    pomLayout->setContentsMargins(20, 15, 20, 15);
    ui->pomLogTable->setMaximumHeight(100);
    ui->pomLogTable->setMinimumHeight(80);
    ui->pomLogTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->pomLogCard->setMaximumHeight(150);
    ui->pomWrapper->setMinimumWidth(400);
    ui->pomWrapper->setMaximumWidth(550);

    ui->pomStateLabel->setText("准备开始");

    // ===== 番茄钟：时长设置 =====
    QWidget *pomSettingsWidget = new QWidget();
    pomSettingsWidget->setObjectName("pomSettingsWidget");
    QHBoxLayout *pomSettingsLayout = new QHBoxLayout(pomSettingsWidget);
    pomSettingsLayout->setContentsMargins(0, 0, 0, 0);
    pomSettingsLayout->setSpacing(10);

    QLabel *focusLabel = new QLabel("专注:");
    focusLabel->setObjectName("pomLabel");
    m_pomFocusSpin = new QSpinBox();
    m_pomFocusSpin->setObjectName("pomFocusSpin");
    m_pomFocusSpin->setRange(1, 120);
    m_pomFocusSpin->setValue(25);
    m_pomFocusSpin->setSuffix(" 分钟");
    m_pomFocusSpin->setMaximumWidth(100);

    QLabel *restLabel = new QLabel("休息:");
    restLabel->setObjectName("pomLabel");
    m_pomRestSpin = new QSpinBox();
    m_pomRestSpin->setObjectName("pomRestSpin");
    m_pomRestSpin->setRange(1, 60);
    m_pomRestSpin->setValue(5);
    m_pomRestSpin->setSuffix(" 分钟");
    m_pomRestSpin->setMaximumWidth(100);

    pomSettingsLayout->addStretch();
    pomSettingsLayout->addWidget(focusLabel);
    pomSettingsLayout->addWidget(m_pomFocusSpin);
    pomSettingsLayout->addWidget(restLabel);
    pomSettingsLayout->addWidget(m_pomRestSpin);
    pomSettingsLayout->addStretch();

    // 插入到状态标签之前
    pomLayout->insertWidget(stateLabelIndex, pomSettingsWidget);

    // 连接 SpinBox 信号，实时更新计时器配置
    connect(m_pomFocusSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        m_pomodoroManager->setFocusDuration(val);
        if (m_pomodoroManager->state() == Idle) {
            m_circularProgress->setTimeText(QString("%1:00").arg(val, 2, 10, QChar('0')));
        }
        savePomSettings();
    });
    connect(m_pomRestSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        m_pomodoroManager->setRestDuration(val);
        savePomSettings();
    });

    // 加载保存的设置
    loadPomSettings();

    // ===== 番茄钟：关联任务 =====
    QWidget *pomTaskWidget = new QWidget();
    pomTaskWidget->setObjectName("pomTaskWidget");
    QHBoxLayout *pomTaskLayout = new QHBoxLayout(pomTaskWidget);
    pomTaskLayout->setContentsMargins(0, 0, 0, 0);
    pomTaskLayout->setSpacing(8);

    QLabel *taskLabel = new QLabel("关联任务:");
    taskLabel->setObjectName("pomLabel");
    m_pomTaskCombo = new QComboBox();
    m_pomTaskCombo->setObjectName("pomTaskCombo");
    m_pomTaskCombo->addItem("无", -1);
    m_pomTaskCombo->setMinimumWidth(200);
    m_pomTaskCombo->setMaximumWidth(300);

    pomTaskLayout->addStretch();
    pomTaskLayout->addWidget(taskLabel);
    pomTaskLayout->addWidget(m_pomTaskCombo, 1);
    pomTaskLayout->addStretch();

    // 插入到时长设置之后
    int taskWidgetIndex = pomLayout->indexOf(pomSettingsWidget) + 1;
    pomLayout->insertWidget(taskWidgetIndex, pomTaskWidget);

    refreshPomTaskCombo();

    // ===== AI 对话 =====
    m_aiChatManager = new AiChatManager(this);
    m_chatSessionManager = new ChatSessionManager(this);

    QWidget *chatContentWidget = new QWidget();
    m_chatContentLayout = new QVBoxLayout(chatContentWidget);
    m_chatContentLayout->setAlignment(Qt::AlignTop);
    m_chatContentLayout->setSpacing(10);
    m_chatContentLayout->setContentsMargins(10, 10, 10, 10);
    ui->chatScrollArea->setWidget(chatContentWidget);
    ui->chatScrollArea->setWidgetResizable(true);

    // 对话区域自适应宽度
    ui->chatWrapper->setMinimumWidth(500);
    ui->chatWrapper->setMaximumWidth(16777215);
    ui->chatScrollArea->setMinimumWidth(400);

    // 创建会话列表面板
    QWidget *chatSessionPanel = new QWidget();
    chatSessionPanel->setObjectName("chatSessionPanel");
    chatSessionPanel->setMinimumWidth(180);
    chatSessionPanel->setMaximumWidth(240);

    QVBoxLayout *sessionPanelLayout = new QVBoxLayout(chatSessionPanel);
    sessionPanelLayout->setContentsMargins(8, 8, 8, 8);
    sessionPanelLayout->setSpacing(6);

    QHBoxLayout *sessionHeaderLayout = new QHBoxLayout();
    QLabel *sessionLabel = new QLabel("对话列表");
    sessionLabel->setObjectName("chatSessionLabel");
    QPushButton *newSessionBtn = new QPushButton("+");
    newSessionBtn->setObjectName("chatNewSessionBtn");
    sessionHeaderLayout->addWidget(sessionLabel);
    sessionHeaderLayout->addWidget(newSessionBtn);
    sessionPanelLayout->addLayout(sessionHeaderLayout);

    QListWidget *chatSessionList = new QListWidget();
    chatSessionList->setObjectName("chatSessionList");
    chatSessionList->setContextMenuPolicy(Qt::CustomContextMenu);
    sessionPanelLayout->addWidget(chatSessionList);

    ui->chatOuterLayout->insertWidget(0, chatSessionPanel);

    // 连接信号
    connect(newSessionBtn, &QPushButton::clicked, this, &MainWindow::createChatSession);
    connect(chatSessionList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        QListWidget *list = findChild<QListWidget*>("chatSessionList");
        QListWidgetItem *item = list->item(row);
        if (!item) return;
        int sessionId = item->data(Qt::UserRole).toInt();
        switchChatSession(sessionId);
    });
    connect(chatSessionList, &QListWidget::customContextMenuRequested,
            this, [this, chatSessionList](const QPoint &pos) {
        QListWidgetItem *item = chatSessionList->itemAt(pos);
        if (!item) return;
        int sessionId = item->data(Qt::UserRole).toInt();

        QMenu menu(this);
        QAction *renameAction = menu.addAction("重命名");
        QAction *deleteAction = menu.addAction("删除对话");

        QAction *selected = menu.exec(chatSessionList->mapToGlobal(pos));
        if (selected == renameAction) {
            renameChatSession(sessionId);
        } else if (selected == deleteAction) {
            deleteChatSession(sessionId);
        }
    });

    connect(m_aiChatManager, &AiChatManager::replyReceived,
            this, &MainWindow::onAiReplyReceived);
    connect(m_aiChatManager, &AiChatManager::replyChunkReceived,
            this, &MainWindow::onAiReplyChunkReceived);
    connect(m_aiChatManager, &AiChatManager::errorOccurred,
            this, &MainWindow::onAiErrorOccurred);
    connect(m_aiChatManager, &AiChatManager::replyStarted,
            this, &MainWindow::onAiReplyStarted);
    connect(ui->chatSendBtn, &QPushButton::clicked, this, [this]() {
        QString text = ui->chatInput->toPlainText().trimmed();
        if (text.isEmpty() || m_currentSessionId < 0) return;

        addChatBubble(text, true);
        saveChatMessage("user", text);
        m_aiChatManager->sendMessage(text, m_currentSessionId);
        ui->chatInput->clear();
    });

    // Enter 发送，Shift+Enter 换行
    // Enter 发送，Shift+Enter 换行
    ui->chatInput->installEventFilter(this);

    // ===== 快捷笔记 =====
    m_noteManager = new NoteManager(this);
    m_currentNoteId = -1;
    ui->noteListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->noteMainLayout->setAlignment(Qt::AlignHCenter);
    ui->noteWrapper->setMinimumWidth(600);
    ui->noteWrapper->setMaximumWidth(900);

    connect(ui->noteAddBtn, &QPushButton::clicked, this, &MainWindow::addNote);
    connect(ui->noteSaveBtn, &QPushButton::clicked, this, &MainWindow::saveNote);
    connect(ui->noteDeleteBtn, &QPushButton::clicked, this, &MainWindow::deleteNote);
    connect(ui->notePinBtn, &QPushButton::clicked, this, &MainWindow::pinNote);
    connect(ui->noteSearchBtn, &QPushButton::clicked, this, &MainWindow::searchNotes);
    connect(ui->noteSearchInput, &QLineEdit::returnPressed, this, &MainWindow::searchNotes);

    connect(ui->noteListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onNoteSelected);
    refreshNoteList();

    // ===== 工作流快捷启动 =====
    m_workflowManager = new WorkflowManager(this);
    ui->launcherMainLayout->setAlignment(Qt::AlignHCenter);
    ui->launcherWrapper->setMinimumWidth(600);
    ui->launcherWrapper->setMaximumWidth(1000);

    connect(ui->workflowAddBtn, &QPushButton::clicked, this, &MainWindow::addWorkflow);
    connect(ui->workflowBrowseBtn, &QPushButton::clicked, this, &MainWindow::browseWorkflowExe);
    refreshWorkflowGrid();

    connect(ui->chatClearBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentSessionId < 0) return;
        QSqlDatabase db = DatabaseManager::instance().database();
        if (db.isOpen()) {
            QSqlQuery query(db);
            query.prepare("DELETE FROM chat_history WHERE session_id = ?");
            query.addBindValue(QString::number(m_currentSessionId));
            query.exec();
        }
        m_aiChatManager->clearHistory();
        QLayoutItem *item;
        while ((item = m_chatContentLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
    });

    // 加载会话列表并选中最近的会话
    refreshSessionList();
    QListWidget *sessionList = findChild<QListWidget*>("chatSessionList");
    if (sessionList->count() > 0) {
        sessionList->setCurrentRow(0);
    } else {
        createChatSession();
    }

    // 启动本地服务器，供第二个实例通知恢复窗口
    startLocalServer();
}



MainWindow::~MainWindow()
{
    if (m_localServer) {
        m_localServer->close();
    }
    delete ui;
}

void MainWindow::startLocalServer()
{
    m_localServer = new QLocalServer(this);
    QLocalServer::removeServer("OrbitDesk_SingleInstance_IPC");
    m_localServer->listen("OrbitDesk_SingleInstance_IPC");
    connect(m_localServer, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket *client = m_localServer->nextPendingConnection();
        if (client) {
            client->waitForReadyRead(500);
            QByteArray data = client->readAll();
            if (data == "show") {
                showNormal();
                activateWindow();
                raise();
            }
            client->disconnectFromServer();
            client->deleteLater();
        }
    });
}

void MainWindow::showMonitorPage()
{
    ui->mainContent->setCurrentWidget(ui->pageMonitor);
}

void MainWindow::showTaskPage()
{
    ui->mainContent->setCurrentWidget(ui->pageTask);
}

void MainWindow::showNotePage()
{
    ui->mainContent->setCurrentWidget(ui->pageNote);
}

void MainWindow::showPomodoroPage()
{
    ui->mainContent->setCurrentWidget(ui->pagePomodoro);
}

void MainWindow::showChatPage()
{
    ui->mainContent->setCurrentWidget(ui->pageChat);
}

void MainWindow::showLauncherPage()
{
    ui->mainContent->setCurrentWidget(ui->pageLauncher);
}
void MainWindow::addTask()
{
    QString title = ui->taskTitleInput->text().trimmed();
    if (title.isEmpty()) {
        qDebug() << "任务标题不能为空";
        return;
    }

    QString priority = ui->taskPriorityCombo->currentData().toString();
    QDateTime dueDate = ui->taskDueDate->dateTime();
    QString category = ui->taskCategoryInput->text().trimmed();

    if (m_taskManager->addTask(title, priority, dueDate, category)) {
        clearInput();
        refreshTaskTable();
        refreshPomTaskCombo();
        qDebug() << "任务添加成功";
    }
}

void MainWindow::deleteTask()
{
    int row = ui->taskTable->currentRow();
    if (row < 0) {
        qDebug() << "请先选择一个任务";
        return;
    }

    int taskId = ui->taskTable->item(row, 0)->data(Qt::UserRole).toInt();
    if (m_taskManager->deleteTask(taskId)) {
        refreshTaskTable();
        refreshPomTaskCombo();
        qDebug() << "任务删除成功";
    }
}

void MainWindow::clearInput()
{
    ui->taskTitleInput->clear();
    ui->taskCategoryInput->clear();
    ui->taskPriorityCombo->setCurrentIndex(1);  // 默认"中"
    ui->taskDueDate->setDateTime(QDateTime::currentDateTime());
}

void MainWindow::refreshTaskTable()
{
    QList<Task> tasks = m_taskManager->getAllTasks();
    ui->taskTable->setRowCount(tasks.size());

    for (int i = 0; i < tasks.size(); i++) {
        const Task &task = tasks[i];

        // 标题
        QTableWidgetItem *titleItem = new QTableWidgetItem(task.title);
        titleItem->setData(Qt::UserRole, task.id);
        titleItem->setTextAlignment(Qt::AlignCenter);
        if (task.isDone) {
            titleItem->setForeground(QColor("#6c7086"));
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
        }
        ui->taskTable->setItem(i, 0, titleItem);

        // 优先级（中文化显示）
        QString priorityText;
        QColor priorityColor;
        if (task.priority == "urgent") {
            priorityText = "紧急";
            priorityColor = QColor("#f38ba8");
        } else if (task.priority == "high") {
            priorityText = "高";
            priorityColor = QColor("#fab387");
        } else if (task.priority == "medium") {
            priorityText = "中";
            priorityColor = QColor("#f9e2af");
        } else {
            priorityText = "低";
            priorityColor = QColor("#a6e3a1");
        }
        QTableWidgetItem *priorityItem = new QTableWidgetItem(priorityText);
        priorityItem->setTextAlignment(Qt::AlignCenter);
        priorityItem->setForeground(priorityColor);
        ui->taskTable->setItem(i, 1, priorityItem);

        // 分类
        QTableWidgetItem *categoryItem = new QTableWidgetItem(task.category);
        categoryItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTable->setItem(i, 2, categoryItem);

        // 截止日期（到期高亮）
        QTableWidgetItem *dateItem = new QTableWidgetItem(task.dueDate.toString("MM-dd HH:mm"));
        dateItem->setTextAlignment(Qt::AlignCenter);
        if (!task.isDone) {
            QDateTime now = QDateTime::currentDateTime();
            qint64 secsToDue = now.secsTo(task.dueDate);
            if (secsToDue < 0) {
                // 已过期：红色背景
                dateItem->setBackground(QColor("#f38ba8"));
                dateItem->setForeground(QColor("#1e1e2e"));
            } else if (secsToDue < 86400) {
                // 24小时内到期：黄色背景
                dateItem->setBackground(QColor("#f9e2af"));
                dateItem->setForeground(QColor("#1e1e2e"));
            }
        }
        ui->taskTable->setItem(i, 3, dateItem);

        // 专注时长
        int focusMin = m_taskManager->getFocusMinutes(task.id);
        QString focusText = focusMin > 0 ? QString("%1分钟").arg(focusMin) : "-";
        QTableWidgetItem *focusItem = new QTableWidgetItem(focusText);
        focusItem->setTextAlignment(Qt::AlignCenter);
        if (focusMin > 0) {
            focusItem->setForeground(QColor("#89b4fa"));
        }
        ui->taskTable->setItem(i, 4, focusItem);

        // 状态
        QTableWidgetItem *statusItem = new QTableWidgetItem(task.isDone ? "已完成" : "待完成");
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTable->setItem(i, 5, statusItem);
    }
}
//表格空白处无法取消选中
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 窗口拖动 - 在标题栏按下鼠标
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // 检查是否点击在标题栏区域
            QPoint titleBarPos = ui->titleBar->mapFromGlobal(mouseEvent->globalPosition().toPoint());
            if (ui->titleBar->rect().contains(titleBarPos)) {
                // 排除标题栏上的按钮
                QWidget *clickedWidget = ui->titleBar->childAt(titleBarPos);
                if (clickedWidget != ui->btnMin && clickedWidget != ui->btnMax && clickedWidget != ui->btnClose) {
                    if (!isMaximized() && !isFullScreen()) {
                        m_isDragging = true;
                        m_dragPos = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
                    }
                    return true;
                }
            }
        }

        QPoint tablePos = ui->taskTable->mapFromGlobal(mouseEvent->globalPosition().toPoint());
        bool clickOnTable = ui->taskTable->rect().contains(tablePos);

        if (!clickOnTable) {
            ui->taskTable->clearSelection();
        }
    }
    // 窗口拖动 - 鼠标移动
    else if (event->type() == QEvent::MouseMove && m_isDragging) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        move(mouseEvent->globalPosition().toPoint() - m_dragPos);
        return true;
    }
    // 鼠标释放 - 拖拽结束 or 工作流点击
    else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        // 拖拽结束
        if (m_isDragging) {
            m_isDragging = false;
            return true;
        }
        // 工作流卡片 - 鼠标释放才执行
        QVariant workflowIdVariant = watched->property("workflowId");
        if (workflowIdVariant.isValid() && mouseEvent->button() == Qt::LeftButton) {
            int workflowId = workflowIdVariant.toInt();
            launchWorkflow(workflowId);
            return true;
        }
    }
    // AI 对话输入框：Enter 发送，Shift+Enter 换行
    if (watched == ui->chatInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false; // Shift+Enter 换行
            } else {
                ui->chatSendBtn->click();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onPomodoroStateChange(PomodoroState state)
{
    bool isRunning = (state == Focusing || state == Resting || state == Paused);
    m_pomFocusSpin->setEnabled(!isRunning);
    m_pomRestSpin->setEnabled(!isRunning);
    m_pomTaskCombo->setEnabled(!isRunning);

    switch (state) {
    case Idle:
        ui->pomStateLabel->setText("准备开始");
        ui->pomStateLabel->setStyleSheet(
            "QLabel { color: #a6adc8; font-size: 18px; font-weight: bold; }");
        ui->pomBtnStart->setEnabled(true);
        ui->pomBtnPause->setText("暂停");
        m_circularProgress->setProgressColor(QColor("#cba6f7"));
        m_circularProgress->setProgress(0.0);
        m_circularProgress->setTimeText(QString("%1:00").arg(m_pomFocusSpin->value(), 2, 10, QChar('0')));
        break;
    case Focusing:
        ui->pomStateLabel->setText("专注中...");
        ui->pomStateLabel->setStyleSheet(
            "QLabel { color: #f38ba8; font-size: 22px; font-weight: bold; }");
        ui->pomBtnStart->setEnabled(false);
        ui->pomBtnPause->setText("暂停");
        m_circularProgress->setProgressColor(QColor("#f38ba8"));
        break;
    case Paused:
        ui->pomStateLabel->setText("已暂停");
        ui->pomStateLabel->setStyleSheet(
            "QLabel { color: #f9e2af; font-size: 18px; font-weight: bold; }");
        ui->pomBtnPause->setText("继续");
        m_circularProgress->setProgressColor(QColor("#f9e2af"));
        break;
    case Resting:
        ui->pomStateLabel->setText("休息中...");
        ui->pomStateLabel->setStyleSheet(
            "QLabel { color: #a6e3a1; font-size: 22px; font-weight: bold; }");
        ui->pomBtnStart->setEnabled(false);
        ui->pomBtnPause->setText("暂停");
        m_circularProgress->setProgressColor(QColor("#a6e3a1"));
        break;
    }
}


void MainWindow::onPomodoroTick(int remainingSeconds)
{
    int min = remainingSeconds / 60;
    int sec = remainingSeconds % 60;
    QString timeText = QString("%1:%2")
                           .arg(min, 2, 10, QChar('0'))
                           .arg(sec, 2, 10, QChar('0'));

    m_circularProgress->setTimeText(timeText);

    int total = m_pomodoroManager->totalSeconds();
    if (total > 0) {
        double progress = static_cast<double>(remainingSeconds) / total;
        m_circularProgress->setProgress(progress);
    }
}


void MainWindow::onPomodoroFocusCompleted()
{
    int taskId = m_pomodoroManager->currentTaskId();
    QSqlDatabase db = DatabaseManager::instance().database();
    if (db.isOpen()) {
        QSqlQuery query(db);
        query.prepare(
            "INSERT INTO pomodoro_logs (duration_min, task_id, started_at, completed) "
            "VALUES (?, ?, ?, 1)"
            );
        query.addBindValue(m_pomodoroManager->totalSeconds() / 60);
        query.addBindValue(taskId > 0 ? taskId : QVariant());
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        query.exec();
    }

    // 更新统计
    refreshPomStats();
    refreshPomLogTable();
    refreshTaskTable();

    ui->pomStateLabel->setText("专注完成！点击开始进入休息");
    ui->pomBtnStart->setEnabled(true);
    m_pomodoroManager->startRest();
}

void MainWindow::onPomodoroRestCompleted()
{
    ui->pomStateLabel->setText("休息结束！准备下一轮");
    ui->pomBtnStart->setEnabled(true);
}

void MainWindow::refreshPomStats()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    query.prepare(
        "SELECT COUNT(*), COALESCE(SUM(duration_min), 0) "
        "FROM pomodoro_logs "
        "WHERE DATE(started_at) = ? AND completed = 1"
        );
    query.addBindValue(today);
    query.exec();

    int count = 0;
    int totalMin = 0;
    if (query.next()) {
        count = query.value(0).toInt();
        totalMin = query.value(1).toInt();
    }

    ui->pomStatsLabel->setText(
        QString("今日专注：%1 次，共 %2 分钟").arg(count).arg(totalMin)
        );
}

void MainWindow::refreshPomLogTable()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(db);
    query.prepare(
        "SELECT started_at, duration_min FROM pomodoro_logs "
        "WHERE DATE(started_at) = ? AND completed = 1 "
        "ORDER BY started_at DESC"
        );
    query.addBindValue(today);
    query.exec();

    QList<QStringList> rows;
    while (query.next()) {
        rows.append({
            query.value(0).toString(),
            query.value(1).toString()
        });
    }

    ui->pomLogTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); i++) {
        QTableWidgetItem *timeItem = new QTableWidgetItem(rows[i][0]);
        timeItem->setTextAlignment(Qt::AlignCenter);
        ui->pomLogTable->setItem(i, 0, timeItem);

        QTableWidgetItem *durItem = new QTableWidgetItem(rows[i][1]);
        durItem->setTextAlignment(Qt::AlignCenter);
        ui->pomLogTable->setItem(i, 1, durItem);
    }
}

void MainWindow::refreshPomTaskCombo()
{
    m_pomTaskCombo->clear();
    m_pomTaskCombo->addItem("无", -1);

    QList<Task> tasks = m_taskManager->getAllTasks();
    for (const Task &task : tasks) {
        if (!task.isDone) {
            m_pomTaskCombo->addItem(task.title, task.id);
        }
    }
}

void MainWindow::loadPomSettings()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("SELECT value FROM user_settings WHERE key = ?");
    query.addBindValue("pomodoro_focus_minutes");
    if (query.exec() && query.next()) {
        int val = query.value(0).toInt();
        if (val > 0) {
            m_pomFocusSpin->setValue(val);
            m_pomodoroManager->setFocusDuration(val);
        }
    }

    query.prepare("SELECT value FROM user_settings WHERE key = ?");
    query.addBindValue("pomodoro_rest_minutes");
    if (query.exec() && query.next()) {
        int val = query.value(0).toInt();
        if (val > 0) {
            m_pomRestSpin->setValue(val);
            m_pomodoroManager->setRestDuration(val);
        }
    }
}

void MainWindow::savePomSettings()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO user_settings (key, value) VALUES (?, ?)");
    query.addBindValue("pomodoro_focus_minutes");
    query.addBindValue(QString::number(m_pomFocusSpin->value()));
    query.exec();

    query.prepare("INSERT OR REPLACE INTO user_settings (key, value) VALUES (?, ?)");
    query.addBindValue("pomodoro_rest_minutes");
    query.addBindValue(QString::number(m_pomRestSpin->value()));
    query.exec();
}

QString MainWindow::markdownToHtml(const QString &markdown)
{
    QString html = markdown;

    // 代码块 ```
    html.replace(QRegularExpression("```([\\s\\S]*?)```"),
                 "<pre style='background-color:#1e1e2e; color:#a6e3a1; padding:10px; border-radius:6px; font-family:Consolas,monospace; font-size:13px;'>\\1</pre>");

    // 行内代码 `
    html.replace(QRegularExpression("`([^`]+)`"),
                 "<code style='background-color:#45475a; color:#f38ba8; padding:2px 6px; border-radius:4px; font-family:Consolas,monospace;'>\\1</code>");

    // 标题 ### / ## / #
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption),
                 "<h4 style='color:#cba6f7; margin:8px 0 4px 0;'>\\1</h4>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption),
                 "<h3 style='color:#cba6f7; margin:10px 0 5px 0;'>\\1</h3>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption),
                 "<h2 style='color:#cba6f7; margin:12px 0 6px 0;'>\\1</h2>");

    // 粗体 **text**
    html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
    // 斜体 *text*
    html.replace(QRegularExpression("\\*(.+?)\\*"), "<i>\\1</i>");

    // 无序列表 - item
    html.replace(QRegularExpression("^- (.+)$", QRegularExpression::MultilineOption),
                 "<span style='color:#cba6f7;'>•</span> \\1<br>");
    // 有序列表 1. item
    html.replace(QRegularExpression("^(\\d+)\\. (.+)$", QRegularExpression::MultilineOption),
                 "<span style='color:#cba6f7;'>\\1.</span> \\2<br>");

    // 链接 [text](url)
    html.replace(QRegularExpression("\\[(.+?)\\]\\((.+?)\\)"),
                 "<a href='\\2' style='color:#89b4fa;'>\\1</a>");

    // 换行
    html.replace("\n", "<br>");

    return html;
}

void MainWindow::addChatBubble(const QString &text, bool isUser)
{
    QWidget *bubble = new QWidget();
    QHBoxLayout *bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(0, 0, 0, 0);

    if (isUser) {
        QLabel *label = new QLabel(text);
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setContentsMargins(12, 8, 12, 8);
        label->setStyleSheet(
            "QLabel {"
            "  background-color: #cba6f7;"
            "  color: #1e1e2e;"
            "  border-radius: 12px;"
            "  padding: 8px 12px;"
            "  font-size: 14px;"
            "}"
            );
        bubbleLayout->addStretch();
        bubbleLayout->addWidget(label);
    } else {
        QLabel *label = new QLabel();
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        label->setOpenExternalLinks(true);
        label->setContentsMargins(12, 8, 12, 8);

        QString html = markdownToHtml(text);
        label->setText(html);

        label->setStyleSheet(
            "QLabel {"
            "  background-color: #313244;"
            "  color: #cdd6f4;"
            "  border-radius: 12px;"
            "  padding: 12px 16px;"
            "  font-size: 14px;"
            "  line-height: 1.6;"
            "}"
        );

        bubbleLayout->addStretch();
        bubbleLayout->addWidget(label, 1);
    }

    m_chatContentLayout->addWidget(bubble);
    QScrollBar *scrollBar = ui->chatScrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::removeThinkingBubble()
{
    // 从后往前找"正在思考..."气泡并移除
    for (int i = m_chatContentLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = m_chatContentLayout->itemAt(i);
        if (item && item->widget()) {
            QLabel *label = item->widget()->findChild<QLabel*>();
            if (label && label->text().contains("正在思考")) {
                m_chatContentLayout->removeItem(item);
                item->widget()->deleteLater();
                delete item;
                return;
            }
        }
    }
}

void MainWindow::onAiReplyReceived(const QString &reply, int sessionId)
{
    m_pendingSessions.remove(sessionId);

    // 始终保存到正确的会话
    saveChatMessageToSession(sessionId, "assistant", reply);

    // 只有当前会话匹配才显示最终气泡
    if (m_currentSessionId == sessionId) {
        // 移除流式气泡，替换为最终气泡
        removeStreamingBubble(sessionId);
        addChatBubble(reply, false);
    } else {
        // 不在当前会话，清理流式标签
        m_streamingLabels.remove(sessionId);
    }
}


void MainWindow::onAiErrorOccurred(const QString &error, int sessionId)
{
    m_pendingSessions.remove(sessionId);

    if (m_currentSessionId == sessionId) {
        removeStreamingBubble(sessionId);
        addChatBubble("请求失败：" + error, false);
    } else {
        m_streamingLabels.remove(sessionId);
    }
}

void MainWindow::onAiReplyChunkReceived(const QString &chunk, int sessionId)
{
    if (m_currentSessionId != sessionId) return;

    QLabel *label = m_streamingLabels.value(sessionId);
    if (!label) return;

    // 追加内容到流式气泡
    QString currentText = label->text();
    if (currentText == "思考中...") {
        currentText.clear();  // 第一次收到内容时清空"思考中..."
    }
    currentText += chunk;
    label->setText(markdownToHtml(currentText));

    // 自动滚动到底部
    QScrollBar *scrollBar = ui->chatScrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

QLabel* MainWindow::createStreamingBubble()
{
    QWidget *bubble = new QWidget();
    QHBoxLayout *bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *label = new QLabel("思考中...");
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    label->setOpenExternalLinks(true);
    label->setContentsMargins(12, 8, 12, 8);
    label->setStyleSheet(
        "QLabel {"
        "  background-color: #313244;"
        "  color: #cdd6f4;"
        "  border-radius: 12px;"
        "  padding: 12px 16px;"
        "  font-size: 14px;"
        "  line-height: 1.6;"
        "}"
    );

    bubbleLayout->addStretch();
    bubbleLayout->addWidget(label, 1);

    m_chatContentLayout->addWidget(bubble);
    QScrollBar *scrollBar = ui->chatScrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    return label;
}

void MainWindow::removeStreamingBubble(int sessionId)
{
    QLabel *label = m_streamingLabels.take(sessionId);
    if (!label) return;

    // 找到气泡容器并移除
    QWidget *bubble = label->parentWidget();
    if (bubble) {
        int idx = m_chatContentLayout->indexOf(bubble);
        if (idx >= 0) {
            QLayoutItem *item = m_chatContentLayout->takeAt(idx);
            bubble->deleteLater();
            delete item;
        }
    }
}

void MainWindow::onAiReplyStarted(int sessionId)
{
    m_pendingSessions.insert(sessionId);

    if (m_currentSessionId == sessionId) {
        // 创建一个空的流式气泡，后续逐步填充内容
        QLabel *streamLabel = createStreamingBubble();
        m_streamingLabels[sessionId] = streamLabel;
    }
}

void MainWindow::saveChatMessage(const QString &role, const QString &content)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO chat_history (role, content, session_id) "
        "VALUES (?, ?, ?)"
        );
    query.addBindValue(role);
    query.addBindValue(content);
    query.addBindValue(QString::number(m_currentSessionId));
    query.exec();

    m_chatSessionManager->updateSessionTimestamp(m_currentSessionId);

    // 首条用户消息自动作为会话标题
    if (role == "user") {
        ChatSession session = m_chatSessionManager->getSession(m_currentSessionId);
        if (session.title == "新对话") {
            QString autoTitle = content.left(30);
            if (content.length() > 30) autoTitle += "...";
            m_chatSessionManager->renameSession(m_currentSessionId, autoTitle);
            refreshSessionList();
        }
    }
}

void MainWindow::saveChatMessageToSession(int sessionId, const QString &role, const QString &content)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO chat_history (role, content, session_id) "
        "VALUES (?, ?, ?)"
        );
    query.addBindValue(role);
    query.addBindValue(content);
    query.addBindValue(QString::number(sessionId));
    query.exec();

    m_chatSessionManager->updateSessionTimestamp(sessionId);
}

void MainWindow::addNote()
{
    m_currentNoteId = -1;
    ui->noteTitleInput->clear();
    ui->noteTagsInput->clear();
    ui->noteContentEdit->clear();
    ui->noteTitleInput->setFocus();
}

void MainWindow::saveNote()
{
    QString title = ui->noteTitleInput->text().trimmed();
    QString content = ui->noteContentEdit->toPlainText().trimmed();
    QString tags = ui->noteTagsInput->text().trimmed();

    if (title.isEmpty() && content.isEmpty()) {
        return;
    }

    if (title.isEmpty()) {
        title = "无标题笔记";
    }

    if (m_currentNoteId == -1) {
        // 新增笔记
        if (m_noteManager->addNote(title, content, tags)) {
            refreshNoteList();
            // 选中最后添加的笔记
            ui->noteListWidget->setCurrentRow(ui->noteListWidget->count() - 1);
        }
    } else {
        // 更新笔记
        if (m_noteManager->updateNote(m_currentNoteId, title, content, tags)) {
            refreshNoteList();
        }
    }
}

void MainWindow::deleteNote()
{
    if (m_currentNoteId == -1) return;

    if (m_noteManager->deleteNote(m_currentNoteId)) {
        m_currentNoteId = -1;
        ui->noteTitleInput->clear();
        ui->noteTagsInput->clear();
        ui->noteContentEdit->clear();
        refreshNoteList();
    }
}

void MainWindow::pinNote()
{
    if (m_currentNoteId == -1) return;

    if (m_noteManager->togglePin(m_currentNoteId)) {
        refreshNoteList();
    }
}

void MainWindow::searchNotes()
{
    QString keyword = ui->noteSearchInput->text().trimmed();
    if (keyword.isEmpty()) {
        refreshNoteList();
        return;
    }

    QList<Note> notes = m_noteManager->searchNotes(keyword);
    ui->noteListWidget->clear();

    for (const Note &note : notes) {
        QString display = note.title;
        if (note.isPinned) {
            display = "📌 " + display;
        }
        QListWidgetItem *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, note.id);
        ui->noteListWidget->addItem(item);
    }
}

void MainWindow::refreshNoteList()
{
    QList<Note> notes = m_noteManager->getAllNotes();
    ui->noteListWidget->clear();

    for (const Note &note : notes) {
        QString display = note.title;
        if (note.isPinned) {
            display = "📌 " + display;
        }
        QListWidgetItem *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, note.id);
        ui->noteListWidget->addItem(item);
    }
}

void MainWindow::onNoteSelected(int row)
{
    if (row < 0 || row >= ui->noteListWidget->count()) {
        m_currentNoteId = -1;
        return;
    }

    QListWidgetItem *item = ui->noteListWidget->item(row);
    int noteId = item->data(Qt::UserRole).toInt();
    Note note = m_noteManager->getNote(noteId);

    m_currentNoteId = noteId;
    ui->noteTitleInput->setText(note.title);
    ui->noteTagsInput->setText(note.tags);
    ui->noteContentEdit->setText(note.content);

    // 更新置顶按钮文本
    ui->notePinBtn->setText(note.isPinned ? "取消置顶" : "置顶");
}

void MainWindow::addWorkflow()
{
    QString name = ui->workflowNameInput->text().trimmed();
    QString command = ui->workflowCommandInput->text().trimmed();
    QString desc = ui->workflowDescInput->text().trimmed();

    if (name.isEmpty() || command.isEmpty()) {
        return;
    }

    if (m_workflowManager->addWorkflow(name, command, desc)) {
        ui->workflowNameInput->clear();
        ui->workflowCommandInput->clear();
        ui->workflowDescInput->clear();
        refreshWorkflowGrid();
    }
}

void MainWindow::browseWorkflowExe()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择可执行文件",
        QString(),
        "可执行文件 (*.exe *.bat *.cmd);;所有文件 (*.*)"
    );
    if (!filePath.isEmpty()) {
        ui->workflowCommandInput->setText(filePath);
        if (ui->workflowNameInput->text().trimmed().isEmpty()) {
            QFileInfo info(filePath);
            ui->workflowNameInput->setText(info.baseName());
        }
    }
}

void MainWindow::refreshWorkflowGrid()
{
    QLayout *layout = ui->workflowGridLayout;
    while (layout->count() > 0) {
        QLayoutItem *item = layout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    QList<Workflow> workflows = m_workflowManager->getAllWorkflows();

    int row = 0;
    int col = 0;
    const int maxCols = 3;

    for (int idx = 0; idx < workflows.size(); ++idx) {
        const Workflow &workflow = workflows[idx];

        QWidget *card = new QWidget();
        card->setFixedSize(180, 120);
        card->setStyleSheet(
            "QWidget {"
            "  background-color: #313244;"
            "  border-radius: 12px;"
            "  border: 2px solid #45475a;"
            "}"
            "QWidget:hover {"
            "  border-color: #cba6f7;"
            "  background-color: #3b3d52;"
            "}"
        );
        card->setCursor(Qt::PointingHandCursor);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);

        // 图标
        QLabel *iconLabel = new QLabel();
        iconLabel->setFixedSize(32, 32);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("border: none; background: transparent;");

        if (!workflow.iconPath.isEmpty() && QFile::exists(workflow.iconPath)) {
            QFileIconProvider iconProvider;
            QIcon icon = iconProvider.icon(QFileInfo(workflow.iconPath));
            QPixmap pixmap = icon.pixmap(32, 32);
            iconLabel->setPixmap(pixmap);
        } else if (workflow.command.startsWith("http://") || workflow.command.startsWith("https://")) {
            // URL 默认图标
            iconLabel->setText("🌐");
            iconLabel->setStyleSheet("font-size: 24px; border: none; background: transparent;");
        } else {
            // 默认图标
            iconLabel->setText("🚀");
            iconLabel->setStyleSheet("font-size: 24px; border: none; background: transparent;");
        }

        QLabel *nameLabel = new QLabel(workflow.name);
        nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #cdd6f4; border: none;");
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);

        QLabel *descLabel = new QLabel(workflow.description.isEmpty() ? workflow.command : workflow.description);
        descLabel->setStyleSheet("font-size: 11px; color: #a6adc8; border: none;");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);

        cardLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
        cardLayout->addWidget(nameLabel);
        cardLayout->addWidget(descLabel);
        cardLayout->addStretch();

        int workflowId = workflow.id;

        // 右键菜单
        card->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(card, &QWidget::customContextMenuRequested, this, [this, workflowId, workflows, idx](const QPoint &pos) {
            Q_UNUSED(pos);
            QMenu menu(this);
            QAction *editAction = menu.addAction("✏ 编辑");
            menu.addSeparator();
            QAction *moveUpAction = menu.addAction("⬆ 上移");
            QAction *moveDownAction = menu.addAction("⬇ 下移");
            menu.addSeparator();
            QAction *deleteAction = menu.addAction("🗑 删除");

            // 禁用边界移动
            moveUpAction->setEnabled(idx > 0);
            moveDownAction->setEnabled(idx < workflows.size() - 1);

            QAction *selected = menu.exec(QCursor::pos());
            if (selected == editAction) {
                editWorkflow(workflowId);
            } else if (selected == moveUpAction && idx > 0) {
                m_workflowManager->swapSortOrder(workflowId, workflows[idx - 1].id);
                refreshWorkflowGrid();
            } else if (selected == moveDownAction && idx < workflows.size() - 1) {
                m_workflowManager->swapSortOrder(workflowId, workflows[idx + 1].id);
                refreshWorkflowGrid();
            } else if (selected == deleteAction) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this, "确认删除", "确定要删除这个工作流吗？",
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    deleteWorkflow(workflowId);
                }
            }
        });

        card->setProperty("workflowId", workflowId);
        card->installEventFilter(this);

        ui->workflowGridLayout->addWidget(card, row, col);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    ui->workflowGridLayout->setRowStretch(row + 1, 1);
}

void MainWindow::launchWorkflow(int id)
{
    QString errorMsg;
    if (!m_workflowManager->launchWorkflow(id, &errorMsg)) {
        qDebug() << "启动工作流失败:" << errorMsg;
    }
}

void MainWindow::deleteWorkflow(int id)
{
    if (m_workflowManager->deleteWorkflow(id)) {
        refreshWorkflowGrid();
    }
}

void MainWindow::editWorkflow(int id)
{
    Workflow workflow = m_workflowManager->getWorkflow(id);
    if (workflow.id == 0) return;

    QDialog dialog(this);
    dialog.setWindowTitle("编辑工作流");
    dialog.setMinimumWidth(400);
    dialog.setStyleSheet(
        "QDialog { background-color: #1e1e2e; }"
        "QLabel { color: #cdd6f4; font-size: 13px; }"
        "QLineEdit { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 8px; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid #cba6f7; }"
        "QPushButton { background-color: #cba6f7; color: #1e1e2e; border: none; padding: 8px 16px; border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background-color: #b4befe; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    // 名称
    layout->addWidget(new QLabel("名称:"));
    QLineEdit *nameEdit = new QLineEdit(workflow.name);
    layout->addWidget(nameEdit);

    // 命令/URL
    layout->addWidget(new QLabel("命令或URL:"));
    QLineEdit *commandEdit = new QLineEdit(workflow.command);
    layout->addWidget(commandEdit);

    // 描述
    layout->addWidget(new QLabel("描述:"));
    QLineEdit *descEdit = new QLineEdit(workflow.description);
    layout->addWidget(descEdit);

    // 图标路径
    layout->addWidget(new QLabel("图标路径:"));
    QHBoxLayout *iconLayout = new QHBoxLayout();
    QLineEdit *iconEdit = new QLineEdit(workflow.iconPath);
    QPushButton *browseBtn = new QPushButton("浏览");
    browseBtn->setMaximumWidth(60);
    connect(browseBtn, &QPushButton::clicked, [&]() {
        QString filePath = QFileDialog::getOpenFileName(&dialog, "选择图标文件", QString(),
            "图标文件 (*.exe *.ico *.png *.jpg);;所有文件 (*.*)");
        if (!filePath.isEmpty()) {
            iconEdit->setText(filePath);
        }
    });
    iconLayout->addWidget(iconEdit);
    iconLayout->addWidget(browseBtn);
    layout->addLayout(iconLayout);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *cancelBtn = new QPushButton("取消");
    QPushButton *saveBtn = new QPushButton("保存");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString command = commandEdit->text().trimmed();
        QString desc = descEdit->text().trimmed();
        QString icon = iconEdit->text().trimmed();

        if (!name.isEmpty() && !command.isEmpty()) {
            m_workflowManager->updateWorkflow(id, name, command, desc, icon);
            refreshWorkflowGrid();
        }
    }
}

void MainWindow::createChatSession()
{
    int sessionId = m_chatSessionManager->createSession("新对话");
    if (sessionId < 0) return;
    refreshSessionList();
    QListWidget *list = findChild<QListWidget*>("chatSessionList");
    list->setCurrentRow(0);
}

void MainWindow::switchChatSession(int sessionId)
{
    if (sessionId == m_currentSessionId) return;
    m_currentSessionId = sessionId;
    m_aiChatManager->setCurrentSession(sessionId);

    // 清空当前气泡
    QLayoutItem *item;
    while ((item = m_chatContentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_streamingLabels.clear();

    // 从数据库加载历史
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("SELECT role, content FROM chat_history WHERE session_id = ? ORDER BY id ASC");
    query.addBindValue(QString::number(sessionId));
    query.exec();

    while (query.next()) {
        QString role = query.value(0).toString();
        QString content = query.value(1).toString();
        addChatBubble(content, role == "user");
        m_aiChatManager->addHistoryMessage(role, content);
    }

    // 如果该会话有正在等待的AI回复，重新创建流式气泡
    if (m_pendingSessions.contains(sessionId)) {
        QLabel *streamLabel = createStreamingBubble();
        m_streamingLabels[sessionId] = streamLabel;
    }

    ChatSession session = m_chatSessionManager->getSession(sessionId);
    ui->chatHeaderLabel->setText(session.title.isEmpty() ? "AI 智能助手" : session.title);
}

void MainWindow::deleteChatSession(int sessionId)
{
    m_chatSessionManager->deleteSession(sessionId);

    if (sessionId == m_currentSessionId) {
        m_currentSessionId = -1;
        QLayoutItem *item;
        while ((item = m_chatContentLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        m_aiChatManager->clearHistory();
    }

    refreshSessionList();

    QListWidget *list = findChild<QListWidget*>("chatSessionList");
    if (list->count() > 0) {
        list->setCurrentRow(0);
    } else {
        createChatSession();
    }
}

void MainWindow::renameChatSession(int sessionId)
{
    ChatSession session = m_chatSessionManager->getSession(sessionId);
    bool ok;
    QString newTitle = QInputDialog::getText(this, "重命名对话", "对话名称:",
                                              QLineEdit::Normal, session.title, &ok);
    if (ok && !newTitle.trimmed().isEmpty()) {
        m_chatSessionManager->renameSession(sessionId, newTitle.trimmed());
        refreshSessionList();
        if (sessionId == m_currentSessionId) {
            ui->chatHeaderLabel->setText(newTitle.trimmed());
        }
    }
}

void MainWindow::refreshSessionList()
{
    QListWidget *list = findChild<QListWidget*>("chatSessionList");
    if (!list) return;

    int previousSessionId = m_currentSessionId;
    list->clear();

    QList<ChatSession> sessions = m_chatSessionManager->getAllSessions();
    for (const ChatSession &s : sessions) {
        QListWidgetItem *item = new QListWidgetItem(s.title);
        item->setData(Qt::UserRole, s.id);
        item->setToolTip(s.updatedAt);
        list->addItem(item);
    }

    if (previousSessionId > 0) {
        for (int i = 0; i < list->count(); i++) {
            if (list->item(i)->data(Qt::UserRole).toInt() == previousSessionId) {
                list->setCurrentRow(i);
                break;
            }
        }
    }
}

