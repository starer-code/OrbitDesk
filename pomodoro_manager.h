#ifndef POMODORO_MANAGER_H
#define POMODORO_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

enum PomodoroState {
    Idle,       // 空闲
    Focusing,   // 专注中
    Paused,     // 暂停
    Resting     // 休息中
};

class PomodoroManager : public QObject
{
    Q_OBJECT

public:
    explicit PomodoroManager(QObject *parent = nullptr);

    // 操作
    void startFocus();
    void pause();
    void resume();
    void reset();
    void startRest();

    // 获取状态
    PomodoroState state() const { return m_state; }
    int remainingSeconds() const { return m_remainingSeconds; }
    int totalSeconds() const { return m_totalSeconds; }

    // 配置
    void setFocusDuration(int minutes);
    void setRestDuration(int minutes);

signals:
    void stateChanged(PomodoroState state);
    void tick(int remainingSeconds);
    void focusCompleted();
    void restCompleted();

private slots:
    void onTimerTick();

private:
    QTimer *m_timer;
    PomodoroState m_state;
    int m_remainingSeconds;
    int m_totalSeconds;
    int m_focusMinutes;
    int m_restMinutes;
};

#endif // POMODORO_MANAGER_H
