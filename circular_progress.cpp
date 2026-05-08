#include "circular_progress.h"
#include <QPainterPath>

CircularProgress::CircularProgress(QWidget *parent)
    : QWidget(parent)
    , m_progress(0.0)
    , m_timeText("25:00")
    , m_progressColor(QColor("#cba6f7"))
    , m_bgColor(QColor("#313244"))
    , m_textColor(QColor("#cdd6f4"))
{
    setMinimumSize(200, 200);
}

void CircularProgress::setProgress(double progress)
{
    m_progress = progress;
    update();  // 触发重绘
}

void CircularProgress::setTimeText(const QString &text)
{
    m_timeText = text;
    update();
}

void CircularProgress::setProgressColor(const QColor &color)
{
    m_progressColor = color;
    update();
}

void CircularProgress::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    int penWidth = 12;
    int radius = (side - penWidth) / 2;

    // 居中
    painter.translate(width() / 2, height() / 2);

    // ===== 1. 画背景圆环 =====
    QPen bgPen(m_bgColor);
    bgPen.setWidth(penWidth);
    bgPen.setCapStyle(Qt::RoundCap);
    painter.setPen(bgPen);
    painter.drawEllipse(QPointF(0, 0), radius, radius);

    // ===== 2. 画进度圆弧 =====
    if (m_progress > 0.0) {
        QPen progressPen(m_progressColor);
        progressPen.setWidth(penWidth);
        progressPen.setCapStyle(Qt::RoundCap);
        painter.setPen(progressPen);

        // 从顶部（90度）开始，顺时针画弧
        // Qt 的角度：0度在3点钟方向，90度在6点钟方向
        // 我们要从12点钟方向开始，所以起始角度是 -90*16 = -1440
        int startAngle = -90 * 16;
        int spanAngle = -static_cast<int>(m_progress * 360 * 16);  // 负数=顺时针

        painter.drawArc(
            QRectF(-radius, -radius, radius * 2, radius * 2),
            startAngle,
            spanAngle
            );
    }

    // ===== 3. 画中间时间文字 =====
    painter.setPen(m_textColor);

    // 时间数字（大号）
    QFont timeFont("Consolas", side / 5, QFont::Bold);
    painter.setFont(timeFont);

    // 上半部分画时间
    QRectF timeRect(-radius, -radius / 3, radius * 2, radius);
    painter.drawText(timeRect, Qt::AlignCenter, m_timeText);

    // ===== 4. 画外圈装饰线 =====
    QPen dotPen(QColor("#45475a"));
    dotPen.setWidth(2);
    painter.setPen(dotPen);
    painter.drawEllipse(QPointF(0, 0), radius + 15, radius + 15);
}
