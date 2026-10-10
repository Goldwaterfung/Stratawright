// src/Presentation/views/rotary_dial.cpp
#include "rotary_dial.h"
#include "../theme.h"
#include <QPainter>
#include <QPen>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace presentation::views {

RotaryDial::RotaryDial(QWidget* parent)
    : BaseTactileControl(parent) {
    // Set a premium default compact size
    setMinimumSize(40, 40);
    m_sensitivity = 0.003f; // Custom dials feel slightly heavier
}

void RotaryDial::resizeEvent(QResizeEvent* event) {
    m_bgCacheValid = false;
    BaseTactileControl::resizeEvent(event);
}

void RotaryDial::renderStaticBackground() {
    qreal dpr = devicePixelRatioF();
    
    // Initialize background cache at high physical resolution to respect Retina/4K displays
    m_bgCache = QPixmap(size() * dpr);
    m_bgCache.setDevicePixelRatio(dpr);
    m_bgCache.fill(Qt::transparent);

    QPainter painter(&m_bgCache);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPointF center(width() / 2.0, height() / 2.0);
    qreal radius = std::min(width(), height()) * 0.38;

    // 1. Draw raised knob face: flat BgControl fill, no outline ring.
    // The face contrasts against BgSurface parents by fill alone, matching
    // the CyberFader grip fill language. Sweep, needle and ticks stay legible
    // even where the face meets a selected (BgControl) strip.
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme::Color::BgControl);
    painter.drawEllipse(center, radius, radius);

    // 2. Draw subtle inner bezel ring
    painter.setPen(QPen(QColor(0, 0, 0, 80), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, radius - 1.5, radius - 1.5);

    // 3. Draw minimalist tick marks around the knob sweep (from 220 degrees to -40 degrees)
    // We place 5 ticks: 220 (fully counter-clockwise), 155, 90 (center), 25, -40 (fully clockwise)
    painter.setPen(QPen(theme::Color::TextMuted, 1.0));
    
    qreal startAngleDeg = 220.0;
    qreal endAngleDeg = -40.0;
    int tickCount = 5;
    
    for (int i = 0; i < tickCount; ++i) {
        qreal angleDeg = startAngleDeg - (i * (startAngleDeg - endAngleDeg) / (tickCount - 1));
        qreal angleRad = angleDeg * M_PI / 180.0;
        
        // Tick positions relative to circular bounds
        qreal innerRadius = radius + 2.5;
        qreal outerRadius = radius + 4.5;
        
        QPointF p1(center.x() + innerRadius * std::cos(angleRad),
                   center.y() - innerRadius * std::sin(angleRad));
        QPointF p2(center.x() + outerRadius * std::cos(angleRad),
                   center.y() - outerRadius * std::sin(angleRad));
                   
        painter.drawLine(p1, p2);
    }

    m_bgCacheValid = true;
}

void RotaryDial::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    // 1. Regenerate static background pixmap if size changed or cache is invalidated
    if (!m_bgCacheValid || m_bgCache.isNull()) {
        renderStaticBackground();
    }

    QPainter painter(this);
    
    // 2. Bit-blit the cached static background (Blazing fast CPU/GPU transfer)
    painter.drawPixmap(0, 0, m_bgCache);

    painter.setRenderHint(QPainter::Antialiasing, true);

    QPointF center(width() / 2.0, height() / 2.0);
    qreal radius = std::min(width(), height()) * 0.38;

    // Knob sweeping angle parameters matching ticks: 220 degrees to -40 degrees (260 degree sweep)
    qreal startAngle = 220.0;
    qreal sweepSpan = 260.0;
    
    // --- 3. Paint Gradient Sweep Arc (violet -> teal along drag direction) ---
    if (m_value > 0.0f) {
        QRectF arcRect(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0);

        // Segment the sweep so the color can blend from AccentGlow to
        // SendPreFader along the arc; round caps overlap slightly for seamless joints.
        const int segs = 48;
        const QColor c0 = theme::Color::AccentGlow;
        const QColor c1 = theme::Color::SendPreFader;
        const qreal penW = m_isDragging ? 3.0 : 2.0;
        for (int i = 0; i < segs; ++i) {
            qreal t = (static_cast<qreal>(i) + 0.5) / static_cast<qreal>(segs);
            QColor segColor(
                static_cast<int>(c0.red() + (c1.red() - c0.red()) * t),
                static_cast<int>(c0.green() + (c1.green() - c0.green()) * t),
                static_cast<int>(c0.blue() + (c1.blue() - c0.blue()) * t));
            QPen segPen(segColor, penW);
            segPen.setCapStyle(Qt::RoundCap);
            painter.setPen(segPen);

            // Qt drawArc uses 1/16th of a degree. Negative sweeps clockwise.
            qreal segStartDeg = startAngle - static_cast<double>(m_value) * sweepSpan * static_cast<qreal>(i) / static_cast<qreal>(segs);
            int segSpanQt = static_cast<int>(static_cast<double>(m_value) * sweepSpan / static_cast<qreal>(segs) * 16.0) + 1;
            painter.drawArc(arcRect, static_cast<int>(segStartDeg * 16.0), -segSpanQt);
        }
    }

    // --- 4. Paint Position Dot (minimal pointer, no needle line) ---
    qreal dotAngleDeg = startAngle - (static_cast<double>(m_value) * sweepSpan);
    qreal dotAngleRad = dotAngleDeg * M_PI / 180.0;

    QPointF dotP(center.x() + radius * 0.65 * std::cos(dotAngleRad),
                 center.y() - radius * 0.65 * std::sin(dotAngleRad));

    // Soft glow behind the dot, stronger while dragging
    theme::PaintHelper::drawVolumetricGlow(&painter, QRectF(dotP.x() - 4.0, dotP.y() - 4.0, 8.0, 8.0), theme::Color::AccentGlow, m_isDragging ? 0.6 : 0.4);

    // Solid dot marker inside the knob face
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme::Color::TextPrimary);
    painter.drawEllipse(dotP, 2.5, 2.5);
}

} // namespace presentation::views
