#include "monitor_page.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

MonitorPage::MonitorPage(QWidget *parent)
    : QWidget(parent)
{
    m_monitor = new SystemMonitor(this);
    connect(m_monitor, &SystemMonitor::statsUpdated, this, &MonitorPage::onStatsUpdated);
    setupUi();
}

void MonitorPage::startMonitoring()
{
    m_monitor->start();
}

void MonitorPage::stopMonitoring()
{
    m_monitor->stop();
}

void MonitorPage::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 标题
    auto *titleLabel = new QLabel("系统监控", this);
    titleLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #cba6f7;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 仪表盘行
    auto *gaugesLayout = new QHBoxLayout();
    gaugesLayout->setAlignment(Qt::AlignCenter);
    gaugesLayout->setSpacing(40);

    auto makeGaugeColumn = [&](const QString &label, const QColor &color) -> CircularProgress * {
        auto *col = new QVBoxLayout();
        col->setAlignment(Qt::AlignCenter);
        col->setSpacing(8);

        auto *gauge = new CircularProgress(this);
        gauge->setMinimumSize(140, 140);
        gauge->setMaximumSize(180, 180);
        gauge->setProgressColor(color);
        gauge->setTimeText("0.0%");
        col->addWidget(gauge, 0, Qt::AlignCenter);

        auto *lbl = new QLabel(label, this);
        lbl->setStyleSheet("font-size: 14px; color: #a6adc8;");
        lbl->setAlignment(Qt::AlignCenter);
        col->addWidget(lbl, 0, Qt::AlignCenter);

        gaugesLayout->addLayout(col);
        return gauge;
    };

    m_cpuGauge = makeGaugeColumn("CPU", QColor("#f38ba8"));
    m_memGauge = makeGaugeColumn("内存", QColor("#cba6f7"));
    m_diskGauge = makeGaugeColumn("磁盘", QColor("#89b4fa"));

    mainLayout->addLayout(gaugesLayout);

    // 信息卡片
    auto *infoCard = new QWidget(this);
    infoCard->setObjectName("monitorInfoCard");
    infoCard->setStyleSheet(
        "QWidget#monitorInfoCard {"
        "  background-color: #313244;"
        "  border-radius: 10px;"
        "  padding: 16px;"
        "}"
        );
    infoCard->setMaximumWidth(500);

    auto *infoLayout = new QGridLayout(infoCard);
    infoLayout->setContentsMargins(20, 16, 20, 16);
    infoLayout->setVerticalSpacing(10);
    infoLayout->setHorizontalSpacing(20);

    auto addInfoRow = [&](int row, const QString &title, const QString &color) -> QLabel * {
        auto *key = new QLabel(title, this);
        key->setStyleSheet("color: #a6adc8; font-size: 13px; border: none; background: transparent;");
        key->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        infoLayout->addWidget(key, row, 0);

        auto *val = new QLabel("--", this);
        val->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; border: none; background: transparent;").arg(color));
        val->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        infoLayout->addWidget(val, row, 1);
        return val;
    };

    m_uploadLabel = addInfoRow(0, "上传速度", "#a6e3a1");
    m_downloadLabel = addInfoRow(1, "下载速度", "#89b4fa");
    m_hostnameLabel = addInfoRow(2, "主机名", "#cdd6f4");
    m_osLabel = addInfoRow(3, "操作系统", "#cdd6f4");
    m_uptimeLabel = addInfoRow(4, "运行时间", "#f9e2af");

    infoLayout->setColumnStretch(0, 0);
    infoLayout->setColumnStretch(1, 1);

    // 卡片居中
    auto *cardWrapper = new QHBoxLayout();
    cardWrapper->addStretch();
    cardWrapper->addWidget(infoCard);
    cardWrapper->addStretch();
    mainLayout->addLayout(cardWrapper);

    mainLayout->addStretch();
}

void MonitorPage::onStatsUpdated(const SystemStats &stats)
{
    m_cpuGauge->setProgress(stats.cpuPercent / 100.0);
    m_cpuGauge->setTimeText(QString::number(stats.cpuPercent, 'f', 1) + "%");

    m_memGauge->setProgress(stats.memoryPercent / 100.0);
    m_memGauge->setTimeText(QString::number(stats.memoryPercent, 'f', 1) + "%");

    m_diskGauge->setProgress(stats.diskPercent / 100.0);
    m_diskGauge->setTimeText(QString::number(stats.diskPercent, 'f', 1) + "%");

    m_uploadLabel->setText(QString::number(stats.netUploadKBps, 'f', 1) + " KB/s");
    m_downloadLabel->setText(QString::number(stats.netDownloadKBps, 'f', 1) + " KB/s");
    m_hostnameLabel->setText(stats.hostname);
    m_osLabel->setText(stats.osVersion);
    m_uptimeLabel->setText(stats.uptimeText);
}
