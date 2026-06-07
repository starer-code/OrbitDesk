#include "history_chart.h"
#include <QPainter>
#include <QPainterPath>

HistoryChart::HistoryChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    setMaximumHeight(280);
}

void HistoryChart::addDataPoint(double cpu, double memory)
{
    ChartDataPoint point;
    point.time = QDateTime::currentDateTime();
    point.cpuPercent = cpu;
    point.memoryPercent = memory;
    m_dataPoints.append(point);

    // 只保留最近 1 小时（每 2 秒一条，约 1800 条）
    QDateTime oneHourAgo = QDateTime::currentDateTime().addSecs(-3600);
    while (!m_dataPoints.isEmpty() && m_dataPoints.first().time < oneHourAgo) {
        m_dataPoints.removeFirst();
    }

    update();
}

void HistoryChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawChart(p, rect());
}

void HistoryChart::drawChart(QPainter &p, const QRect &rect)
{
    // 背景
    p.fillRect(rect, QColor("#1e1e2e"));
    p.setPen(QPen(QColor("#45475a"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 10, 10);

    // 图表区域
    int margin = 45;
    QRect chartRect(rect.left() + margin, rect.top() + 15,
                    rect.width() - margin - 15, rect.height() - 40);

    // 无数据提示
    if (m_dataPoints.size() < 2) {
        p.setPen(QColor("#6c7086"));
        p.setFont(QFont("Microsoft YaHei UI", 11));
        p.drawText(chartRect, Qt::AlignCenter, "等待数据...");
        return;
    }

    // 网格线
    p.setPen(QPen(QColor("#313244"), 1, Qt::DashLine));
    for (int i = 0; i <= 4; ++i) {
        int y = chartRect.top() + (chartRect.height() * i) / 4;
        p.drawLine(chartRect.left(), y, chartRect.right(), y);
    }

    // 提取数据
    QVector<double> cpuVals, memVals;
    for (const auto &pt : m_dataPoints) {
        cpuVals.append(pt.cpuPercent);
        memVals.append(pt.memoryPercent);
    }

    // 画线函数
    auto drawLine = [&](const QVector<double> &vals, const QColor &color) {
        if (vals.size() < 2) return;
        double xStep = static_cast<double>(chartRect.width()) / (vals.size() - 1);

        // 渐变填充
        QLinearGradient grad(0, chartRect.top(), 0, chartRect.bottom());
        grad.setColorAt(0, QColor(color.red(), color.green(), color.blue(), 40));
        grad.setColorAt(1, QColor(color.red(), color.green(), color.blue(), 5));

        QPainterPath fillPath;
        fillPath.moveTo(chartRect.left(), chartRect.bottom());
        for (int i = 0; i < vals.size(); ++i) {
            double x = chartRect.left() + i * xStep;
            double y = chartRect.bottom() - (vals[i] / 100.0) * chartRect.height();
            fillPath.lineTo(x, y);
        }
        fillPath.lineTo(chartRect.right(), chartRect.bottom());
        fillPath.closeSubpath();
        p.fillPath(fillPath, grad);

        // 折线
        p.setPen(QPen(color, 2.5));
        QPainterPath linePath;
        for (int i = 0; i < vals.size(); ++i) {
            double x = chartRect.left() + i * xStep;
            double y = chartRect.bottom() - (vals[i] / 100.0) * chartRect.height();
            if (i == 0) linePath.moveTo(x, y);
            else linePath.lineTo(x, y);
        }
        p.drawPath(linePath);
    };

    drawLine(memVals, QColor("#cba6f7"));  // 内存：紫色
    drawLine(cpuVals, QColor("#f38ba8"));  // CPU：红色

    // Y 轴标签
    p.setPen(QColor("#a6adc8"));
    p.setFont(QFont("Microsoft YaHei UI", 9));
    for (int i = 0; i <= 4; ++i) {
        int y = chartRect.top() + (chartRect.height() * i) / 4;
        p.drawText(chartRect.left() - 40, y - 8, 35, 16, Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1%").arg(100 - i * 25));
    }

    // X 轴时间标签
    QDateTime first = m_dataPoints.first().time;
    QDateTime last = m_dataPoints.last().time;
    p.drawText(chartRect.left(), chartRect.bottom() + 8, 60, 16, Qt::AlignLeft, first.toString("HH:mm"));
    p.drawText(chartRect.right() - 55, chartRect.bottom() + 8, 55, 16, Qt::AlignRight, last.toString("HH:mm"));

    // 图例
    int lx = chartRect.right() - 130;
    int ly = chartRect.top() + 8;
    p.fillRect(lx, ly, 16, 3, QColor("#f38ba8"));
    p.setPen(QColor("#cdd6f4"));
    p.drawText(lx + 20, ly - 5, 40, 14, Qt::AlignLeft, "CPU");
    p.fillRect(lx + 65, ly, 16, 3, QColor("#cba6f7"));
    p.drawText(lx + 85, ly - 5, 40, 14, Qt::AlignLeft, "内存");
}
