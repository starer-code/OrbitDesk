#ifndef MONITOR_PAGE_H
#define MONITOR_PAGE_H

#include <QWidget>
#include <QLabel>
#include "circular_progress.h"
#include "system_monitor.h"

class MonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorPage(QWidget *parent = nullptr);
    void startMonitoring();
    void stopMonitoring();

public slots:
    void onStatsUpdated(const SystemStats &stats);

private:
    void setupUi();

    CircularProgress *m_cpuGauge;
    CircularProgress *m_memGauge;
    CircularProgress *m_diskGauge;

    QLabel *m_uploadLabel;
    QLabel *m_downloadLabel;
    QLabel *m_hostnameLabel;
    QLabel *m_osLabel;
    QLabel *m_uptimeLabel;

    SystemMonitor *m_monitor;
};

#endif // MONITOR_PAGE_H
