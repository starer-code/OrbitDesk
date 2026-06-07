#ifndef HISTORY_CHART_H
#define HISTORY_CHART_H

#include <QWidget>
#include <QVector>
#include <QDateTime>

struct ChartDataPoint {
    QDateTime time;
    double cpuPercent;
    double memoryPercent;
};

class HistoryChart : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryChart(QWidget *parent = nullptr);

    void addDataPoint(double cpu, double memory);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<ChartDataPoint> m_dataPoints;

    void drawChart(QPainter &p, const QRect &rect);
};

#endif // HISTORY_CHART_H
