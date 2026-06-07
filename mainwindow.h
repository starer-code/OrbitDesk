#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSet>
#include <QSpinBox>
#include <QComboBox>
#include <QLocalServer>
#include <QLocalSocket>
#include "task_manager.h"
#include "pomodoro_manager.h"
#include "circular_progress.h"
#include "ai_chat_manager.h"
#include "monitor_page.h"
#include "note_manager.h"
#include "workflow_manager.h"
#include "chat_session_manager.h"
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
    void refreshPomTaskCombo();
    void loadPomSettings();
    void savePomSettings();

    void showMonitorPage();
    void showTaskPage();
    void showNotePage();
    void showPomodoroPage();
    void showChatPage();
    void showLauncherPage();
    // AI 对话
    void onAiReplyReceived(const QString &reply, int sessionId);
    void onAiReplyChunkReceived(const QString &chunk, int sessionId);
    void onAiErrorOccurred(const QString &error, int sessionId);
    void onAiReplyStarted(int sessionId);
    void addChatBubble(const QString &text, bool isUser);
    void removeThinkingBubble();
    QLabel* createStreamingBubble();
    void removeStreamingBubble(int sessionId);
    void saveChatMessage(const QString &role, const QString &content);
    void saveChatMessageToSession(int sessionId, const QString &role, const QString &content);
    QString markdownToHtml(const QString &markdown);

    // 聊天会话管理
    void createChatSession();
    void switchChatSession(int sessionId);
    void deleteChatSession(int sessionId);
    void renameChatSession(int sessionId);
    void refreshSessionList();

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
    void editWorkflow(int id);

private:
    CircularProgress *m_circularProgress;

    PomodoroManager *m_pomodoroManager;
    QComboBox *m_pomTaskCombo;
    QSpinBox *m_pomFocusSpin;
    QSpinBox *m_pomRestSpin;
    Ui::MainWindow *ui;
    QPoint m_dragPos;
    QRect m_normalGeometry;
    bool m_isDragging = false;
    TaskManager *m_taskManager;
    AiChatManager *m_aiChatManager;
    QVBoxLayout *m_chatContentLayout;
    MonitorPage *m_monitorPage;
    NoteManager *m_noteManager;
    WorkflowManager *m_workflowManager;
    int m_currentNoteId;

    // 聊天会话
    ChatSessionManager *m_chatSessionManager;
    int m_currentSessionId = -1;
    QSet<int> m_pendingSessions;  // 正在等待AI回复的会话ID集合
    QMap<int, QLabel*> m_streamingLabels;  // sessionId -> 正在流式显示的气泡Label

    // 系统托盘
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    // 单实例 IPC
    QLocalServer *m_localServer;
    void startLocalServer();

};
#endif // MAINWINDOW_H
