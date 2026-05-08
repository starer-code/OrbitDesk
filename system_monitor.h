#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <QObject>
#include <QTimer>
#include <windows.h>
#include <iphlpapi.h>

struct SystemStats {
    double cpuPercent = 0.0;
    double memoryPercent = 0.0;
    quint64 memoryUsedMB = 0;
    quint64 memoryTotalMB = 0;
    double diskPercent = 0.0;
    quint64 diskUsedGB = 0;
    quint64 diskTotalGB = 0;
    double netUploadKBps = 0.0;
    double netDownloadKBps = 0.0;
    QString hostname;
    QString osVersion;
    QString uptimeText;
};

class SystemMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SystemMonitor(QObject *parent = nullptr, int intervalMs = 2000);
    void start();
    void stop();

signals:
    void statsUpdated(const SystemStats &stats);

private slots:
    void poll();

private:
    double getCpuUsage();
    void getMemoryInfo(quint64 &usedMB, quint64 &totalMB, double &percent);
    void getDiskInfo(quint64 &usedGB, quint64 &totalGB, double &percent);
    void getNetworkSpeed(double &upKBps, double &downKBps);
    QString getHostname();
    QString getOsVersion();
    QString getUptime();
    void saveToDatabase(const SystemStats &stats);

    // CPU
    ULONGLONG m_prevIdleTime = 0;
    ULONGLONG m_prevKernelTime = 0;
    ULONGLONG m_prevUserTime = 0;
    bool m_firstCpuSample = true;

    // Network
    quint64 m_prevBytesIn = 0;
    quint64 m_prevBytesOut = 0;
    bool m_firstNetSample = true;

    QTimer *m_timer;
    int m_intervalMs;

    // Cached system info
    QString m_cachedHostname;
    QString m_cachedOsVersion;
    bool m_infoCached = false;
};

#endif // SYSTEM_MONITOR_H
