#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "task_manager.h"
#include "pomodoro_manager.h"
#include "circular_progress.h"
#include "ai_chat_manager.h"
#include "monitor_page.h"
#include "note_manager.h"
#include "workflow_manager.h"
#include <QScrollArea>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // 番茄钟
    void onPomodoroStateChange(PomodoroState state);
    void onPomodoroTick(int remainingSeconds);
    void onPomodoroFocusCompleted();
    void onPomodoroRestCompleted();
    void refreshPomStats();
    void refreshPomLogTable();

    void showMonitorPage();
    void showTaskPage();
    void showNotePage();
    void showPomodoroPage();
    void showChatPage();
    void showLauncherPage();
    // AI 对话
    void onAiReplyReceived(const QString &reply);
    void onAiErrorOccurred(const QString &error);
    void onAiReplyStarted();
    void addChatBubble(const QString &text, bool isUser);
    void loadChatHistory();
    void saveChatMessage(const QString &role, const QString &content);

    //任务管理
    void addTask();
    void deleteTask();
    void clearInput();
    void refreshTaskTable();

    // 快捷笔记
    void addNote();
    void saveNote();
    void deleteNote();
    void pinNote();
    void searchNotes();
    void refreshNoteList();
    void onNoteSelected(int row);

    // 工作流快捷启动
    void addWorkflow();
    void browseWorkflowExe();
    void refreshWorkflowGrid();
    void launchWorkflow(int id);
    void deleteWorkflow(int id);

private:
    CircularProgress *m_circularProgress;

    PomodoroManager *m_pomodoroManager;
    Ui::MainWindow *ui;
    QPoint m_dragPos;
    bool m_isDragging = false;
    TaskManager *m_taskManager;
    AiChatManager *m_aiChatManager;
    QVBoxLayout *m_chatContentLayout;
    MonitorPage *m_monitorPage;
    NoteManager *m_noteManager;
    WorkflowManager *m_workflowManager;
    int m_currentNoteId;

    // 系统托盘
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

};
#endif // MAINWINDOW_H
