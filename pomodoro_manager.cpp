#include "pomodoro_manager.h"

PomodoroManager::PomodoroManager(QObject *parent)
    : QObject(parent)
    , m_state(Idle)
    , m_remainingSeconds(0)
    , m_totalSeconds(0)
    , m_focusMinutes(25)
    , m_restMinutes(5)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PomodoroManager::onTimerTick);
}

void PomodoroManager::setFocusDuration(int minutes)
{
    m_focusMinutes = minutes;
}

void PomodoroManager::setRestDuration(int minutes)
{
    m_restMinutes = minutes;
}

void PomodoroManager::startFocus()
{
    m_timer->stop();
    m_totalSeconds = m_focusMinutes * 60;
    m_remainingSeconds = m_totalSeconds;
    m_state = Focusing;
    m_timer->start(1000);
    emit stateChanged(m_state);
    emit tick(m_remainingSeconds);
}

void PomodoroManager::startRest()
{
    m_timer->stop();
    m_totalSeconds = m_restMinutes * 60;
    m_remainingSeconds = m_totalSeconds;
    m_state = Resting;
    m_timer->start(1000);
    emit stateChanged(m_state);
    emit tick(m_remainingSeconds);
}

void PomodoroManager::pause()
{
    if (m_state == Focusing || m_state == Resting) {
        m_timer->stop();
        m_state = Paused;
        emit stateChanged(m_state);
    }
}

void PomodoroManager::resume()
{
    if (m_state == Paused) {
        // 判断之前是专注还是休息
        if (m_remainingSeconds > m_focusMinutes * 60 / 2) {
            m_state = Focusing;
        } else {
            m_state = Resting;
        }
        m_timer->start(1000);
        emit stateChanged(m_state);
    }
}

void PomodoroManager::reset()
{
    m_timer->stop();
    m_remainingSeconds = 0;
    m_totalSeconds = 0;
    m_state = Idle;
    emit stateChanged(m_state);
    emit tick(0);
}

void PomodoroManager::onTimerTick()
{
    m_remainingSeconds--;

    if (m_remainingSeconds <= 0) {
        m_timer->stop();

        if (m_state == Focusing) {
            m_state = Idle;
            emit focusCompleted();
        } else if (m_state == Resting) {
            m_state = Idle;
            emit restCompleted();
        }

        emit stateChanged(m_state);
        emit tick(0);
    } else {
        emit tick(m_remainingSeconds);
    }
}
