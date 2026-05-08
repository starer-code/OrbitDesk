#ifndef CIRCULAR_PROGRESS_H
#define CIRCULAR_PROGRESS_H

#include <QWidget>
#include <QPainter>
#include <QTimer>

class CircularProgress : public QWidget
{
    Q_OBJECT

public:
    explicit CircularProgress(QWidget *parent = nullptr);

    void setProgress(double progress);  // 0.0 ~ 1.0
    void setTimeText(const QString &text);
    void setProgressColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_progress;
    QString m_timeText;
    QColor m_progressColor;
    QColor m_bgColor;
    QColor m_textColor;
};

#endif // CIRCULAR_PROGRESS_H
