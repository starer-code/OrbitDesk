#include "system_monitor.h"
#include "database_manager.h"
#include <QSysInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static ULONGLONG fileTimeToULL(const FILETIME &ft)
{
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

SystemMonitor::SystemMonitor(QObject *parent, int intervalMs)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_intervalMs(intervalMs)
{
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::poll);
}

void SystemMonitor::start()
{
    m_timer->start(m_intervalMs);
    poll();
}

void SystemMonitor::stop()
{
    m_timer->stop();
}

void SystemMonitor::poll()
{
    SystemStats stats;

    stats.cpuPercent = getCpuUsage();

    getMemoryInfo(stats.memoryUsedMB, stats.memoryTotalMB, stats.memoryPercent);
    getDiskInfo(stats.diskUsedGB, stats.diskTotalGB, stats.diskPercent);
    getNetworkSpeed(stats.netUploadKBps, stats.netDownloadKBps);

    if (!m_infoCached) {
        m_cachedHostname = getHostname();
        m_cachedOsVersion = getOsVersion();
        m_infoCached = true;
    }
    stats.hostname = m_cachedHostname;
    stats.osVersion = m_cachedOsVersion;
    stats.uptimeText = getUptime();

    emit statsUpdated(stats);
    saveToDatabase(stats);
}

double SystemMonitor::getCpuUsage()
{
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return 0.0;

    ULONGLONG idle = fileTimeToULL(idleTime);
    ULONGLONG kernel = fileTimeToULL(kernelTime);
    ULONGLONG user = fileTimeToULL(userTime);

    if (m_firstCpuSample) {
        m_prevIdleTime = idle;
        m_prevKernelTime = kernel;
        m_prevUserTime = user;
        m_firstCpuSample = false;
        return 0.0;
    }

    ULONGLONG idleDelta = idle - m_prevIdleTime;
    ULONGLONG kernelDelta = kernel - m_prevKernelTime;
    ULONGLONG userDelta = user - m_prevUserTime;
    ULONGLONG sys = kernelDelta + userDelta;

    m_prevIdleTime = idle;
    m_prevKernelTime = kernel;
    m_prevUserTime = user;

    if (sys == 0) return 0.0;
    return static_cast<double>(sys - idleDelta) / sys * 100.0;
}

void SystemMonitor::getMemoryInfo(quint64 &usedMB, quint64 &totalMB, double &percent)
{
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        totalMB = mem.ullTotalPhys / (1024 * 1024);
        quint64 availMB = mem.ullAvailPhys / (1024 * 1024);
        usedMB = totalMB - availMB;
        percent = static_cast<double>(usedMB) / totalMB * 100.0;
    }
}

void SystemMonitor::getDiskInfo(quint64 &usedGB, quint64 &totalGB, double &percent)
{
    ULARGE_INTEGER freeBytes, totalBytes, totalFree;
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytes, &totalBytes, &totalFree)) {
        totalGB = totalBytes.QuadPart / (1024ULL * 1024 * 1024);
        quint64 freeGB = totalFree.QuadPart / (1024ULL * 1024 * 1024);
        usedGB = totalGB - freeGB;
        percent = static_cast<double>(usedGB) / totalGB * 100.0;
    }
}

void SystemMonitor::getNetworkSpeed(double &upKBps, double &downKBps)
{
    DWORD dwSize = 0;
    GetIfTable(nullptr, &dwSize, FALSE);

    if (dwSize == 0) return;

    auto *pTable = reinterpret_cast<MIB_IFTABLE*>(new BYTE[dwSize]);
    if (GetIfTable(pTable, &dwSize, FALSE) != NO_ERROR) {
        delete[] reinterpret_cast<BYTE*>(pTable);
        return;
    }

    quint64 totalIn = 0;
    quint64 totalOut = 0;
    for (DWORD i = 0; i < pTable->dwNumEntries; i++) {
        const MIB_IFROW &row = pTable->table[i];
        if (row.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL &&
            row.dwType != IF_TYPE_SOFTWARE_LOOPBACK) {
            totalIn += row.dwInOctets;
            totalOut += row.dwOutOctets;
        }
    }
    delete[] reinterpret_cast<BYTE*>(pTable);

    if (m_firstNetSample) {
        m_prevBytesIn = totalIn;
        m_prevBytesOut = totalOut;
        m_firstNetSample = false;
        upKBps = 0.0;
        downKBps = 0.0;
        return;
    }

    double elapsedSec = m_intervalMs / 1000.0;
    downKBps = static_cast<double>(totalIn - m_prevBytesIn) / 1024.0 / elapsedSec;
    upKBps = static_cast<double>(totalOut - m_prevBytesOut) / 1024.0 / elapsedSec;

    m_prevBytesIn = totalIn;
    m_prevBytesOut = totalOut;
}

QString SystemMonitor::getHostname()
{
    WCHAR buf[256] = {};
    DWORD size = 256;
    GetComputerNameExW(ComputerNameDnsHostname, buf, &size);
    return QString::fromWCharArray(buf);
}

QString SystemMonitor::getOsVersion()
{
    return QSysInfo::prettyProductName();
}

QString SystemMonitor::getUptime()
{
    ULONGLONG ms = GetTickCount64();
    quint64 totalSec = ms / 1000;
    int days = totalSec / 86400;
    int hours = (totalSec % 86400) / 3600;
    int minutes = (totalSec % 3600) / 60;

    if (days > 0)
        return QString("%1天 %2小时 %3分").arg(days).arg(hours).arg(minutes);
    else if (hours > 0)
        return QString("%1小时 %2分").arg(hours).arg(minutes);
    else
        return QString("%1分钟").arg(minutes);
}

void SystemMonitor::saveToDatabase(const SystemStats &stats)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery q(db);
    q.prepare("INSERT INTO monitor_stats "
              "(cpu_percent, memory_percent, memory_used_mb, disk_percent, net_up_kbps, net_down_kbps) "
              "VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(stats.cpuPercent);
    q.addBindValue(stats.memoryPercent);
    q.addBindValue(static_cast<qint64>(stats.memoryUsedMB));
    q.addBindValue(stats.diskPercent);
    q.addBindValue(stats.netUploadKBps);
    q.addBindValue(stats.netDownloadKBps);
    if (!q.exec()) {
        qDebug() << "Monitor stats save error:" << q.lastError().text();
    }
}
