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
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //隐藏菜单栏
    menuBar()->hide();
    statusBar()->hide();
    //无白边框实现
    setWindowFlag(Qt::FramelessWindowHint);

    // ===== 系统监控页面 =====
    m_monitorPage = new MonitorPage(this);
    ui->monitorWrapperLayout->addWidget(m_monitorPage);
    m_monitorPage->startMonitoring();
    ui->monitorHeaderLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #cba6f7;");

    // AI 对话页面样式
    ui->chatHeaderLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #cba6f7;");

    //标题栏最大小化，以及关闭按钮的实现
    connect(ui->btnMin, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->btnMax, &QPushButton::clicked, this, [this]() {
        if (isMaximized() || isFullScreen()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(ui->btnClose, &QPushButton::clicked, this, [this]() {
        hide();
    });

    // ===== 系统托盘 =====
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
    //连接按钮和页面
    connect(ui->btnNavMonitor, &QPushButton::clicked, this, &MainWindow::showMonitorPage);
    connect(ui->btnNavTask, &QPushButton::clicked, this, &MainWindow::showTaskPage);
    connect(ui->btnNavNote, &QPushButton::clicked, this, &MainWindow::showNotePage);
    connect(ui->btnNavPomodoro, &QPushButton::clicked, this, &MainWindow::showPomodoroPage);
    connect(ui->btnNavChat, &QPushButton::clicked, this, &MainWindow::showChatPage);
    connect(ui->btnNavLauncher, &QPushButton::clicked, this, &MainWindow::showLauncherPage);
    // 按钮可选中
    ui->btnNavMonitor->setCheckable(true);
    ui->btnNavTask->setCheckable(true);
    ui->btnNavNote->setCheckable(true);
    ui->btnNavPomodoro->setCheckable(true);
    ui->btnNavChat->setCheckable(true);
    ui->btnNavLauncher->setCheckable(true);

    // 按钮互斥（点一个取消其他）
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

    // 表格基础设置
    ui->taskTable->setColumnCount(5);
    ui->taskTable->setHorizontalHeaderLabels({"标题", "优先级", "分类", "截止日期", "状态"});
    ui->taskTable->setAlternatingRowColors(true);
    ui->taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->taskTable->verticalHeader()->hide();
    ui->taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->taskPriorityCombo->clear();
    ui->taskPriorityCombo->addItems({"low", "medium", "high", "urgent"});
    connect(ui->btnAddTask, &QPushButton::clicked, this, &MainWindow::addTask);
    connect(ui->btnDeleteTask, &QPushButton::clicked, this, &MainWindow::deleteTask);
    connect(ui->btnClearTask, &QPushButton::clicked, this, &MainWindow::clearInput);
    connect(ui->taskTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        int taskId = ui->taskTable->item(row, 0)->data(Qt::UserRole).toInt();
        m_taskManager->toggleDone(taskId);
        refreshTaskTable();
    });
    refreshTaskTable();
    qApp->installEventFilter(this);
    // ===== 任务管理页面布局优化 =====

    // 页面标题样式
    ui->taskHeaderLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #cba6f7;");

    // gridLayout 列拉伸
    ui->gridLayout->setColumnStretch(0, 0);
    ui->gridLayout->setColumnStretch(1, 1);
    ui->gridLayout->setVerticalSpacing(8);
    ui->gridLayout->setHorizontalSpacing(10);

    // Label 样式
    ui->taskTitleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskPriorityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskDueDateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskCategoryLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->taskTitleLabel->setMaximumWidth(60);
    ui->taskPriorityLabel->setMaximumWidth(60);
    ui->taskDueDateLabel->setMaximumWidth(70);
    ui->taskCategoryLabel->setMaximumWidth(50);

    // 输入框宽度
    ui->taskTitleInput->setMinimumWidth(200);
    ui->taskTitleInput->setMaximumWidth(550);
    ui->taskCategoryInput->setMinimumWidth(120);
    ui->taskCategoryInput->setMaximumWidth(250);
    ui->taskPriorityCombo->setMaximumWidth(150);
    ui->taskDueDate->setDateTime(QDateTime::currentDateTime());

    // pageTask 内容居中
    ui->taskMainLayout->setAlignment(Qt::AlignHCenter);

    // 按钮间距
    ui->taskButtonLayout->setSpacing(10);
    ui->taskButtonLayout->setContentsMargins(0, 10, 0, 10);

    // 表格设置
    ui->taskTable->setMinimumHeight(200);
    ui->taskTable->setMaximumHeight(16777215);
    ui->taskTable->verticalHeader()->setDefaultSectionSize(35);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->taskTable->setColumnWidth(1, 80);
    ui->taskTable->setColumnWidth(2, 80);
    ui->taskTable->setColumnWidth(3, 100);
    ui->taskTable->setColumnWidth(4, 150);
    ui->contentLayout->setStretch(0, 1);  // sidebar 占 1
    ui->contentLayout->setStretch(1, 4);  // mainContent 占 4

    // wrapper 宽度
    ui->taskContentWrapper->setMinimumWidth(550);
    ui->taskContentWrapper->setMaximumWidth(800);

    // 表单卡片和表格卡片样式
    ui->taskFormCard->setObjectName("taskFormCard");
    ui->taskTableCard->setObjectName("taskTableCard");
    ui->taskListLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4;");
    // ===== 番茄钟 =====
    m_pomodoroManager = new PomodoroManager(this);

    // 连接信号
    connect(m_pomodoroManager, &PomodoroManager::stateChanged,
            this, &MainWindow::onPomodoroStateChange);
    connect(m_pomodoroManager, &PomodoroManager::tick,
            this, &MainWindow::onPomodoroTick);
    connect(m_pomodoroManager, &PomodoroManager::focusCompleted,
            this, &MainWindow::onPomodoroFocusCompleted);
    connect(m_pomodoroManager, &PomodoroManager::restCompleted,
            this, &MainWindow::onPomodoroRestCompleted);

    // 按钮连接
    connect(ui->pomBtnStart, &QPushButton::clicked, this, [this]() {
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

    // 表格设置
    ui->pomLogTable->setColumnCount(2);
    ui->pomLogTable->setHorizontalHeaderLabels({"开始时间", "专注时长(分钟)"});
    ui->pomLogTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->pomLogTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->pomLogTable->verticalHeader()->hide();

    // 初始显示
    //ui->pomTimeLabel->setText("25:00");
    // 环形进度条 - 插入到状态标签之后
    ui->pomTimeLabel->hide();
    m_circularProgress = new CircularProgress(ui->pomWrapper);
    // 找到状态标签在布局中的位置，将进度条插入到它后面
    QVBoxLayout *pomLayout = qobject_cast<QVBoxLayout*>(ui->pomWrapper->layout());
    int stateLabelIndex = pomLayout->indexOf(ui->pomStateLabel);
    pomLayout->insertWidget(stateLabelIndex + 1, m_circularProgress);
    m_circularProgress->setMinimumSize(200, 200);
    m_circularProgress->setMaximumSize(280, 280);
    m_circularProgress->setTimeText("25:00");
    // 番茄钟布局紧凑化
    pomLayout->setSpacing(5);
    pomLayout->setContentsMargins(20, 15, 20, 15);
    // 设置各组件的拉伸因子，防止表格被撑开
    pomLayout->setStretch(0, 0);  // 状态标签
    pomLayout->setStretch(1, 0);  // 环形进度条
    pomLayout->setStretch(2, 0);  // 按钮
    pomLayout->setStretch(3, 0);  // 统计标签
    pomLayout->setStretch(4, 0);  // 日志卡片 - 不拉伸
    // 限制表格高度
    ui->pomLogTable->setMaximumHeight(100);
    ui->pomLogTable->setMinimumHeight(80);
    ui->pomLogTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // pomLogCard也限制高度
    ui->pomLogCard->setMaximumHeight(150);
    // wrapper宽度
    ui->pomWrapper->setMinimumWidth(400);
    ui->pomWrapper->setMaximumWidth(550);


    ui->pomStateLabel->setText("准备开始");
    ui->pomStateLabel->setStyleSheet(
        "QLabel { color: #a6adc8; font-size: 18px; font-weight: bold; }");

    //测试数据
    /*m_pomodoroManager->setFocusDuration(1);
    m_pomodoroManager->setRestDuration(1);*/
    // ===== AI 对话 =====
    m_aiChatManager = new AiChatManager(this);

    // 设置 ScrollArea 内容
    QWidget *chatContentWidget = new QWidget();
    m_chatContentLayout = new QVBoxLayout(chatContentWidget);
    m_chatContentLayout->setAlignment(Qt::AlignTop);
    m_chatContentLayout->setSpacing(10);
    m_chatContentLayout->setContentsMargins(10, 10, 10, 10);
    ui->chatScrollArea->setWidget(chatContentWidget);
    ui->chatScrollArea->setWidgetResizable(true);
    // 加载历史对话
    loadChatHistory();

    // 连接信号
    connect(m_aiChatManager, &AiChatManager::replyReceived,
            this, &MainWindow::onAiReplyReceived);
    connect(m_aiChatManager, &AiChatManager::errorOccurred,
            this, &MainWindow::onAiErrorOccurred);
    connect(m_aiChatManager, &AiChatManager::replyStarted,
            this, &MainWindow::onAiReplyStarted);

    // 按钮连接
    connect(ui->chatSendBtn, &QPushButton::clicked, this, [this]() {
        QString text = ui->chatInput->text().trimmed();
        if (text.isEmpty()) return;

        addChatBubble(text, true);
        saveChatMessage("user", text);
        m_aiChatManager->sendMessage(text);
        ui->chatInput->clear();
    });

    // ===== 快捷笔记 =====
    m_noteManager = new NoteManager(this);
    m_currentNoteId = -1;

    // 笔记列表设置
    ui->noteListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 笔记页面居中
    ui->noteMainLayout->setAlignment(Qt::AlignHCenter);
    ui->noteWrapper->setMinimumWidth(600);
    ui->noteWrapper->setMaximumWidth(900);

    // 笔记标题样式
    ui->noteHeaderLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #cba6f7;");

    // 连接信号
    connect(ui->noteAddBtn, &QPushButton::clicked, this, &MainWindow::addNote);
    connect(ui->noteSaveBtn, &QPushButton::clicked, this, &MainWindow::saveNote);
    connect(ui->noteDeleteBtn, &QPushButton::clicked, this, &MainWindow::deleteNote);
    connect(ui->notePinBtn, &QPushButton::clicked, this, &MainWindow::pinNote);
    connect(ui->noteSearchBtn, &QPushButton::clicked, this, &MainWindow::searchNotes);
    connect(ui->noteSearchInput, &QLineEdit::returnPressed, this, &MainWindow::searchNotes);

    connect(ui->noteListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onNoteSelected);

    // 加载笔记列表
    refreshNoteList();

    // ===== 工作流快捷启动 =====
    m_workflowManager = new WorkflowManager(this);

    // 工作流页面居中
    ui->launcherMainLayout->setAlignment(Qt::AlignHCenter);
    ui->launcherWrapper->setMinimumWidth(600);
    ui->launcherWrapper->setMaximumWidth(1000);

    // 工作流标题样式
    ui->launcherHeaderLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #cba6f7;");

    // 连接信号
    connect(ui->workflowAddBtn, &QPushButton::clicked, this, &MainWindow::addWorkflow);
    connect(ui->workflowBrowseBtn, &QPushButton::clicked, this, &MainWindow::browseWorkflowExe);

    // 加载工作流
    refreshWorkflowGrid();




    // 回车发送
    connect(ui->chatInput, &QLineEdit::returnPressed, this, [this]() {
        ui->chatSendBtn->click();
    });

    // 清空按钮
    connect(ui->chatClearBtn, &QPushButton::clicked, this, [this]() {
        m_aiChatManager->clearHistory();
        QLayoutItem *item;
        while ((item = m_chatContentLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        // 清空数据库
        QSqlDatabase db = DatabaseManager::instance().database();
        if (db.isOpen()) {
            QSqlQuery query(db);
            query.exec("DELETE FROM chat_history");
        }
    });




}



MainWindow::~MainWindow()
{
    delete ui;
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

    QString priority = ui->taskPriorityCombo->currentText();
    QDateTime dueDate = ui->taskDueDate->dateTime();
    QString category = ui->taskCategoryInput->text().trimmed();

    if (m_taskManager->addTask(title, priority, dueDate, category)) {
        clearInput();
        refreshTaskTable();
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
        qDebug() << "任务删除成功";
    }
}

void MainWindow::clearInput()
{
    ui->taskTitleInput->clear();
    ui->taskCategoryInput->clear();
    ui->taskPriorityCombo->setCurrentIndex(1);
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

        // 优先级
        QTableWidgetItem *priorityItem = new QTableWidgetItem(task.priority);
        priorityItem->setTextAlignment(Qt::AlignCenter);
        if (task.priority == "urgent") {
            priorityItem->setForeground(QColor("#f38ba8"));
        } else if (task.priority == "high") {
            priorityItem->setForeground(QColor("#fab387"));
        } else if (task.priority == "medium") {
            priorityItem->setForeground(QColor("#f9e2af"));
        } else {
            priorityItem->setForeground(QColor("#a6e3a1"));
        }
        ui->taskTable->setItem(i, 1, priorityItem);

        // 分类
        QTableWidgetItem *categoryItem = new QTableWidgetItem(task.category);
        categoryItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTable->setItem(i, 2, categoryItem);

        // 截止日期
        QTableWidgetItem *dateItem = new QTableWidgetItem(task.dueDate.toString("MM-dd HH:mm"));
        dateItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTable->setItem(i, 3, dateItem);

        // 状态
        QTableWidgetItem *statusItem = new QTableWidgetItem(task.isDone ? "已完成" : "待完成");
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->taskTable->setItem(i, 4, statusItem);
    }
}


/*bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        qDebug() << "=== MousePress detected ===";
        qDebug() << "watched object:" << watched->objectName();
        qDebug() << "event type:" << event->type();
    }

    if (watched == ui->pageTask && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        qDebug() << "Click pos:" << mouseEvent->pos();

        QWidget *clickedWidget = ui->pageTask->childAt(mouseEvent->pos());
        qDebug() << "Clicked widget:" << (clickedWidget ? clickedWidget->objectName() : "null");

        if (clickedWidget != ui->taskTable->viewport()) {
            qDebug() << "Clearing selection";
            ui->taskTable->clearSelection();
        } else {
            qDebug() << "Clicked on table, skip";
        }
    }
    return QMainWindow::eventFilter(watched, event);
}*/
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // 判断点击位置是否在表格内
        QPoint tablePos = ui->taskTable->mapFromGlobal(mouseEvent->globalPosition().toPoint());
        bool clickOnTable = ui->taskTable->rect().contains(tablePos);

        if (!clickOnTable) {
            ui->taskTable->clearSelection();
        }

        // 处理工作流卡片点击
        QVariant workflowIdVariant = watched->property("workflowId");
        if (workflowIdVariant.isValid()) {
            int workflowId = workflowIdVariant.toInt();
            launchWorkflow(workflowId);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
// ===== 番茄钟槽函数 =====

void MainWindow::onPomodoroStateChange(PomodoroState state)
{
    switch (state) {
    case Idle:
        ui->pomStateLabel->setText("准备开始");
        ui->pomStateLabel->setStyleSheet(
            "QLabel { color: #a6adc8; font-size: 18px; font-weight: bold; }");
        ui->pomBtnStart->setEnabled(true);
        ui->pomBtnPause->setText("暂停");
        m_circularProgress->setProgressColor(QColor("#cba6f7"));
        m_circularProgress->setProgress(0.0);
        m_circularProgress->setTimeText("25:00");
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

    // 计算进度
    int total = m_pomodoroManager->totalSeconds();
    if (total > 0) {
        double progress = static_cast<double>(remainingSeconds) / total;
        m_circularProgress->setProgress(progress);
    }
}


void MainWindow::onPomodoroFocusCompleted()
{
    // 记录到数据库
    QSqlDatabase db = DatabaseManager::instance().database();
    if (db.isOpen()) {
        QSqlQuery query(db);
        query.prepare(
            "INSERT INTO pomodoro_logs (duration_min, started_at, completed) "
            "VALUES (?, ?, 1)"
            );
        query.addBindValue(m_pomodoroManager->totalSeconds() / 60);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        query.exec();
    }

    // 更新统计
    refreshPomStats();
    refreshPomLogTable();

    // 提示进入休息
    ui->pomStateLabel->setText("专注完成！点击开始进入休息");
    ui->pomBtnStart->setEnabled(true);

    // 自动进入休息
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

    // 查询今日专注次数和总时长
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
// ===== AI 对话 =====

void MainWindow::addChatBubble(const QString &text, bool isUser)
{
    // 创建气泡容器
    QWidget *bubble = new QWidget();
    QHBoxLayout *bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(0, 0, 0, 0);

    // 创建文字标签
    QLabel *label = new QLabel(text);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setMaximumWidth(400);
    label->setContentsMargins(12, 8, 12, 8);

    if (isUser) {
        // 用户消息：靠右，紫色背景
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
        // AI 回复：靠左，深色背景
        label->setStyleSheet(
            "QLabel {"
            "  background-color: #313244;"
            "  color: #cdd6f4;"
            "  border-radius: 12px;"
            "  padding: 8px 12px;"
            "  font-size: 14px;"
            "}"
            );
        bubbleLayout->addWidget(label);
        bubbleLayout->addStretch();
    }

    m_chatContentLayout->addWidget(bubble);

    // 滚动到底部
    QScrollBar *scrollBar = ui->chatScrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onAiReplyReceived(const QString &reply)
{
    addChatBubble(reply, false);
    saveChatMessage("assistant", reply);
}


void MainWindow::onAiErrorOccurred(const QString &error)
{
    addChatBubble("请求失败：" + error, false);
}

void MainWindow::onAiReplyStarted()
{
    addChatBubble("正在思考...", false);
}

void MainWindow::saveChatMessage(const QString &role, const QString &content)
{
    qDebug() << "=== Saving chat message ===";
    qDebug() << "Role:" << role;
    qDebug() << "Content:" << content;

    QSqlDatabase db = DatabaseManager::instance().database();
    qDebug() << "DB is open:" << db.isOpen();

    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO chat_history (role, content, session_id) "
        "VALUES (?, ?, ?)"
        );
    query.addBindValue(role);
    query.addBindValue(content);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyyMMdd"));

    bool ok = query.exec();
    qDebug() << "Insert success:" << ok;
    if (!ok) {
        qDebug() << "Insert error:" << query.lastError().text();
    }
}

void MainWindow::loadChatHistory()
{
    qDebug() << "=== Loading chat history ===";
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    bool ok = query.exec("SELECT role, content FROM chat_history ORDER BY id ASC");
    qDebug() << "Query success:" << ok;

    int count = 0;
    while (query.next()) {
        QString role = query.value(0).toString();
        QString content = query.value(1).toString();
        qDebug() << "Loaded:" << role;
        addChatBubble(content, role == "user");
        // 同步到 AI 记忆
        m_aiChatManager->addHistoryMessage(role, content);
        count++;
    }
    qDebug() << "Total loaded:" << count;
}

// ===== 快捷笔记实现 =====

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

// ===== 工作流快捷启动实现 =====

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
    // 清空现有内容
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

    for (const Workflow &workflow : workflows) {
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

        // 名称
        QLabel *nameLabel = new QLabel(workflow.name);
        nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4; border: none;");
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);

        // 描述
        QLabel *descLabel = new QLabel(workflow.description.isEmpty() ? workflow.command : workflow.description);
        descLabel->setStyleSheet("font-size: 11px; color: #a6adc8; border: none;");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);

        // 删除按钮
        QPushButton *deleteBtn = new QPushButton("×");
        deleteBtn->setFixedSize(24, 24);
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #f38ba8;"
            "  color: #1e1e2e;"
            "  border-radius: 12px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  border: none;"
            "}"
            "QPushButton:hover {"
            "  background-color: #eba0ac;"
            "}"
        );
        deleteBtn->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *topLayout = new QHBoxLayout();
        topLayout->addStretch();
        topLayout->addWidget(deleteBtn);

        cardLayout->addLayout(topLayout);
        cardLayout->addWidget(nameLabel);
        cardLayout->addWidget(descLabel);
        cardLayout->addStretch();

        // 连接删除按钮信号
        int workflowId = workflow.id;
        connect(deleteBtn, &QPushButton::clicked, [this, workflowId]() {
            deleteWorkflow(workflowId);
        });

        // 使用eventFilter处理卡片点击
        card->setProperty("workflowId", workflowId);
        card->installEventFilter(this);

        ui->workflowGridLayout->addWidget(card, row, col);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    // 添加弹性空间
    ui->workflowGridLayout->setRowStretch(row + 1, 1);
}

void MainWindow::launchWorkflow(int id)
{
    m_workflowManager->launchWorkflow(id);
}

void MainWindow::deleteWorkflow(int id)
{
    if (m_workflowManager->deleteWorkflow(id)) {
        refreshWorkflowGrid();
    }
}







