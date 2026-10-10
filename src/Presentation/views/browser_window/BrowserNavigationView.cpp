// src/Presentation/views/browser_window/BrowserNavigationView.cpp
#include "BrowserNavigationView.h"
#include "../theme.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>

namespace presentation::views {

BrowserNavigationView::BrowserNavigationView(bridge::IBrowserController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller) {
    setupUI();
    updateTabSelection();
}

void BrowserNavigationView::setupUI() {
    // 1. Configure visual frame sizing (48px = 30px tabs + 8px top/bottom padding)
    setFixedHeight(48);
    setStyleSheet("background: transparent; border: none;");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);
    // Helper to create styled tab buttons
    auto createTabButton = [this](const QString& text, int tabId, const QString& tooltip) {
        QPushButton* btn = new QPushButton(text, this);
        btn->setCheckable(true);
        btn->setFont(theme::Font::primary(theme::Font::SizeDetail, QFont::Bold));
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(30);
        
        QColor checkedBgColor = theme::Color::AccentGlow;
        checkedBgColor.setAlphaF(0.15f);
        btn->setStyleSheet(QString(
            "QPushButton { border: none; background: transparent; padding: 4px 8px; border-radius: 8px; color: %3; }"
            "QPushButton:hover { background: #323232; color: #E8E8E8; }"
            "QPushButton:checked { background: %1; color: %2; }"
        ).arg(checkedBgColor.name(QColor::HexArgb), theme::Color::AccentGlow.name(), theme::Color::TextPrimary.name()));
        
        m_tabGroup->addButton(btn, tabId);
        return btn;
    };

    m_btnAll = createTabButton("ALL", static_cast<int>(bridge::BrowserTab::AllFolders), "All folders & resources");
    m_btnProject = createTabButton("PROJECT", static_cast<int>(bridge::BrowserTab::CurrentProject), "Current project assets");
    m_btnPlugins = createTabButton("PLUGINS", static_cast<int>(bridge::BrowserTab::PluginDatabase), "Plugin database");

    layout->addWidget(m_btnAll);
    layout->addWidget(m_btnProject);
    layout->addWidget(m_btnPlugins);

    // Expand tab buttons horizontally
    layout->addStretch();

    // Collapse All Button
    m_btnCollapse = new QPushButton(this);
    m_btnCollapse->setToolTip("Collapse all folders");
    m_btnCollapse->setCursor(Qt::PointingHandCursor);
    m_btnCollapse->setFixedSize(28, 28);
    m_btnCollapse->setStyleSheet("QPushButton { border: none; background: transparent; border-radius: 8px; } QPushButton:hover { background: #323232; }");
    m_btnCollapse->setIcon(theme::PaintHelper::createSvgIcon(":/icons/collapse.svg", QSize(16, 16)));

    // Refresh Button
    m_btnRefresh = new QPushButton(this);
    m_btnRefresh->setToolTip("Refresh sample libraries");
    m_btnRefresh->setCursor(Qt::PointingHandCursor);
    m_btnRefresh->setFixedSize(28, 28);
    m_btnRefresh->setStyleSheet("QPushButton { border: none; background: transparent; border-radius: 8px; } QPushButton:hover { background: #323232; }");
    m_btnRefresh->setIcon(theme::PaintHelper::createSvgIcon(":/icons/refresh.svg", QSize(16, 16)));

    layout->addWidget(m_btnCollapse);
    layout->addWidget(m_btnRefresh);

    // 2. Connect Slots
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_tabGroup, &QButtonGroup::idClicked, this, &BrowserNavigationView::onTabButtonClicked);
#else
    connect(m_tabGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &BrowserNavigationView::onTabButtonClicked);
#endif

    connect(m_btnCollapse, &QPushButton::clicked, this, &BrowserNavigationView::collapseAllRequested);
    connect(m_btnRefresh, &QPushButton::clicked, this, &BrowserNavigationView::refreshRequested);
}

void BrowserNavigationView::onTabButtonClicked(int id) {
    if (m_controller) {
        auto tab = static_cast<bridge::BrowserTab>(id);
        m_controller->setActiveTab(tab);
        emit tabChanged(tab);
    }
}

void BrowserNavigationView::updateTabSelection() {
    if (!m_controller) return;

    bridge::BrowserTab active = m_controller->getActiveTab();
    QAbstractButton* btn = m_tabGroup->button(static_cast<int>(active));
    if (btn) {
        btn->setChecked(true);
    }
}

void BrowserNavigationView::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw bottom subtle divider with double coordinate precision (no 'f' suffix)
    QColor dividerColor = theme::Color::BgControl;
    dividerColor.setAlphaF(0.5f); // Explicit float literal
    QPen pen(dividerColor, 1.0);
    painter.setPen(pen);
    
    double lineY = height() - 1.0;
    painter.drawLine(QPointF(0.0, lineY), QPointF(static_cast<double>(width()), lineY));
}

} // namespace presentation::views
