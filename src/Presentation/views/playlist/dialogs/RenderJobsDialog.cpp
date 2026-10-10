// src/Presentation/views/playlist/dialogs/RenderJobsDialog.cpp
#include "RenderJobsDialog.h"
#include "Middle Bridge/engine/irender_controller.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>

namespace presentation::views {

RenderJobsDialog::RenderJobsDialog(bridge::IRenderController* renderCtrl, QWidget* parent)
    : QDialog(parent)
    , m_renderCtrl(renderCtrl)
{
    setWindowTitle(QStringLiteral("Render Queue & Job Manager"));
    resize(760, 480);
    setMinimumSize(600, 360);

    setupUI();
    applyThemeStyle();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(100);
    connect(m_refreshTimer, &QTimer::timeout, this, &RenderJobsDialog::onTimerTick);
}

void RenderJobsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Header
    auto* headerLabel = new QLabel(QStringLiteral("OFFLINE RENDER QUEUE & BACKGROUND JOBS"), this);
    headerLabel->setFont(theme::Font::monospace(11, QFont::Bold));
    headerLabel->setStyleSheet(QStringLiteral("color: #00FFCC;"));
    mainLayout->addWidget(headerLabel);

    // Jobs Table
    m_jobsTable = new QTableWidget(this);
    m_jobsTable->setColumnCount(6);
    QStringList headers = {
        QStringLiteral("ID"),
        QStringLiteral("Type"),
        QStringLiteral("Job Name"),
        QStringLiteral("Status"),
        QStringLiteral("Progress"),
        QStringLiteral("Output Destination")
    };
    m_jobsTable->setHorizontalHeaderLabels(headers);
    m_jobsTable->horizontalHeader()->setStretchLastSection(true);
    m_jobsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_jobsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_jobsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_jobsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_jobsTable->verticalHeader()->setVisible(false);
    m_jobsTable->setAlternatingRowColors(true);
    m_jobsTable->setColumnWidth(0, 60);
    m_jobsTable->setColumnWidth(1, 80);
    m_jobsTable->setColumnWidth(2, 180);
    m_jobsTable->setColumnWidth(3, 100);
    m_jobsTable->setColumnWidth(4, 90);

    connect(m_jobsTable, &QTableWidget::itemSelectionChanged, this, &RenderJobsDialog::onJobSelectionChanged);
    mainLayout->addWidget(m_jobsTable, 1);

    // Details Panel
    m_detailsBox = new QGroupBox(QStringLiteral("Selected Job Details"), this);
    m_detailsBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* detailsLayout = new QVBoxLayout(m_detailsBox);
    detailsLayout->setContentsMargins(12, 12, 12, 12);
    detailsLayout->setSpacing(8);

    m_jobNameLabel = new QLabel(QStringLiteral("No job selected"), m_detailsBox);
    m_jobNameLabel->setFont(theme::Font::primary(11, QFont::Bold));
    m_jobNameLabel->setStyleSheet(QStringLiteral("color: #DDE6ED;"));
    detailsLayout->addWidget(m_jobNameLabel);

    m_jobStatusLabel = new QLabel(QStringLiteral("Status: Idle"), m_detailsBox);
    m_jobStatusLabel->setFont(theme::Font::primary(10));
    m_jobStatusLabel->setStyleSheet(QStringLiteral("color: #A0A5B5;"));
    detailsLayout->addWidget(m_jobStatusLabel);

    m_jobPathLabel = new QLabel(QStringLiteral("Destination: -"), m_detailsBox);
    m_jobPathLabel->setFont(theme::Font::monospace(9));
    m_jobPathLabel->setStyleSheet(QStringLiteral("color: #7E8A9F;"));
    m_jobPathLabel->setWordWrap(true);
    detailsLayout->addWidget(m_jobPathLabel);

    m_jobProgressBar = new QProgressBar(m_detailsBox);
    m_jobProgressBar->setFixedHeight(16);
    m_jobProgressBar->setFont(theme::Font::monospace(9, QFont::Bold));
    m_jobProgressBar->setRange(0, 100);
    m_jobProgressBar->setValue(0);
    m_jobProgressBar->setAlignment(Qt::AlignCenter);
    detailsLayout->addWidget(m_jobProgressBar);

    mainLayout->addWidget(m_detailsBox);

    // Button Bar
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnCancelJob = new QPushButton(QStringLiteral("Cancel Job"), this);
    m_btnCancelJob->setEnabled(false);
    connect(m_btnCancelJob, &QPushButton::clicked, this, &RenderJobsDialog::onCancelSelectedJob);

    m_btnShowInFinder = new QPushButton(QStringLiteral("Show in Finder / Folder"), this);
    m_btnShowInFinder->setEnabled(false);
    connect(m_btnShowInFinder, &QPushButton::clicked, this, &RenderJobsDialog::onShowInFinder);

    btnLayout->addWidget(m_btnCancelJob);
    btnLayout->addWidget(m_btnShowInFinder);
    btnLayout->addStretch();

    m_btnClose = new QPushButton(QStringLiteral("Close"), this);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(btnLayout);
}

void RenderJobsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    refreshJobs();
    if (m_refreshTimer && !m_refreshTimer->isActive()) {
        m_refreshTimer->start();
    }
}

void RenderJobsDialog::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);
    if (m_refreshTimer && m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
    }
}

void RenderJobsDialog::onTimerTick()
{
    refreshJobs();
}

QString RenderJobsDialog::formatStateText(RenderJobState state) const
{
    switch (state) {
        case RenderJobState::QUEUED:     return QStringLiteral("Queued");
        case RenderJobState::PREPARING:  return QStringLiteral("Preparing");
        case RenderJobState::PROCESSING: return QStringLiteral("Processing");
        case RenderJobState::FINALIZING: return QStringLiteral("Finalizing");
        case RenderJobState::COMPLETED:  return QStringLiteral("Completed");
        case RenderJobState::FAILED:     return QStringLiteral("Failed");
        case RenderJobState::CANCELLED:  return QStringLiteral("Cancelled");
    }
    return QStringLiteral("Unknown");
}

void RenderJobsDialog::refreshJobs()
{
    if (!m_renderCtrl) return;

    std::vector<RenderJobInfo> jobs = m_renderCtrl->listAllJobs();
    uint64_t currentSelectedId = getSelectedJobId();

    m_jobsTable->setRowCount(static_cast<int>(jobs.size()));

    int selectedRow = -1;
    for (size_t row = 0; row < jobs.size(); ++row) {
        const auto& job = jobs[row];
        int rowIdx = static_cast<int>(row);

        if (job.jobId == currentSelectedId) {
            selectedRow = rowIdx;
        }

        // Column 0: ID
        auto* idItem = m_jobsTable->item(rowIdx, 0);
        if (!idItem) {
            idItem = new QTableWidgetItem();
            m_jobsTable->setItem(rowIdx, 0, idItem);
        }
        idItem->setText(QString::number(job.jobId));
        idItem->setData(Qt::UserRole, static_cast<qulonglong>(job.jobId));

        // Column 1: Type
        auto* typeItem = m_jobsTable->item(rowIdx, 1);
        if (!typeItem) {
            typeItem = new QTableWidgetItem();
            m_jobsTable->setItem(rowIdx, 1, typeItem);
        }
        typeItem->setText(job.isStemExport ? QStringLiteral("Stems") : QStringLiteral("Master"));

        // Column 2: Job Name
        auto* nameItem = m_jobsTable->item(rowIdx, 2);
        if (!nameItem) {
            nameItem = new QTableWidgetItem();
            m_jobsTable->setItem(rowIdx, 2, nameItem);
        }
        nameItem->setText(QString::fromUtf8(job.jobName));

        // Column 3: Status
        auto* statusItem = m_jobsTable->item(rowIdx, 3);
        if (!statusItem) {
            statusItem = new QTableWidgetItem();
            m_jobsTable->setItem(rowIdx, 3, statusItem);
        }
        statusItem->setText(formatStateText(job.state));
        if (job.state == RenderJobState::PROCESSING || job.state == RenderJobState::PREPARING) {
            statusItem->setForeground(QBrush(QColor(QStringLiteral("#00FFCC"))));
        } else if (job.state == RenderJobState::COMPLETED) {
            statusItem->setForeground(QBrush(QColor(QStringLiteral("#2DA87E"))));
        } else if (job.state == RenderJobState::FAILED) {
            statusItem->setForeground(QBrush(QColor(QStringLiteral("#FF5555"))));
        } else {
            statusItem->setForeground(QBrush(QColor(QStringLiteral("#A0A5B5"))));
        }

        // Column 4: Progress
        auto* progItem = m_jobsTable->item(rowIdx, 4);
        if (!progItem) {
            progItem = new QTableWidgetItem();
            m_jobsTable->setItem(rowIdx, 4, progItem);
        }
        int pct = static_cast<int>(job.progress * 100.0f);
        progItem->setText(QStringLiteral("%1%").arg(pct));

        // Column 5: Output Destination
        auto* pathItem = m_jobsTable->item(rowIdx, 5);
        if (!pathItem) {
            pathItem = new QTableWidgetItem();
            m_jobsTable->setItem(rowIdx, 5, pathItem);
        }
        pathItem->setText(QString::fromUtf8(job.outputPath));
    }

    if (selectedRow >= 0 && m_jobsTable->currentRow() != selectedRow) {
        m_jobsTable->selectRow(selectedRow);
    } else if (m_jobsTable->currentRow() < 0 && !jobs.empty()) {
        m_jobsTable->selectRow(0);
    }

    onJobSelectionChanged();
}

uint64_t RenderJobsDialog::getSelectedJobId() const
{
    int row = m_jobsTable->currentRow();
    if (row < 0 || row >= m_jobsTable->rowCount()) return 0;
    auto* item = m_jobsTable->item(row, 0);
    if (!item) return 0;
    return item->data(Qt::UserRole).toULongLong();
}

void RenderJobsDialog::selectJob(uint64_t jobId)
{
    for (int row = 0; row < m_jobsTable->rowCount(); ++row) {
        auto* item = m_jobsTable->item(row, 0);
        if (item && item->data(Qt::UserRole).toULongLong() == jobId) {
            m_jobsTable->selectRow(row);
            break;
        }
    }
}

void RenderJobsDialog::onJobSelectionChanged()
{
    uint64_t jobId = getSelectedJobId();
    if (jobId == 0 || !m_renderCtrl) {
        m_btnCancelJob->setEnabled(false);
        m_btnShowInFinder->setEnabled(false);
        m_jobNameLabel->setText(QStringLiteral("No job selected"));
        m_jobStatusLabel->setText(QStringLiteral("Status: -"));
        m_jobPathLabel->setText(QStringLiteral("Destination: -"));
        m_jobProgressBar->setValue(0);
        return;
    }

    RenderJobInfo info{};
    if (m_renderCtrl->getJobInfo(jobId, info)) {
        m_jobNameLabel->setText(QStringLiteral("#%1: %2").arg(info.jobId).arg(QString::fromUtf8(info.jobName)));

        QString detailStatus = formatStateText(info.state);
        if (info.isStemExport && info.totalItemCount > 0) {
            detailStatus += QStringLiteral(" (Stem %1 of %2)").arg(info.currentItemIndex).arg(info.totalItemCount);
        }
        if (info.statusMessage[0] != '\0') {
            detailStatus += QStringLiteral(" - ") + QString::fromUtf8(info.statusMessage);
        }
        m_jobStatusLabel->setText(detailStatus);
        m_jobPathLabel->setText(QStringLiteral("Destination: ") + QString::fromUtf8(info.outputPath));
        m_jobProgressBar->setValue(static_cast<int>(info.progress * 100.0f));

        bool canCancel = (info.state == RenderJobState::QUEUED ||
                          info.state == RenderJobState::PREPARING ||
                          info.state == RenderJobState::PROCESSING ||
                          info.state == RenderJobState::FINALIZING);
        m_btnCancelJob->setEnabled(canCancel);
        m_btnShowInFinder->setEnabled(info.outputPath[0] != '\0');
    }
}

void RenderJobsDialog::onCancelSelectedJob()
{
    uint64_t jobId = getSelectedJobId();
    if (jobId == 0 || !m_renderCtrl) return;

    m_renderCtrl->cancelJob(jobId);
    refreshJobs();
}

void RenderJobsDialog::onShowInFinder()
{
    uint64_t jobId = getSelectedJobId();
    if (jobId == 0 || !m_renderCtrl) return;

    RenderJobInfo info{};
    if (m_renderCtrl->getJobInfo(jobId, info)) {
        QString path = QString::fromUtf8(info.outputPath);
        if (path.isEmpty()) return;

        QFileInfo fi(path);
        QString dirPath = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
    }
}

void RenderJobsDialog::applyThemeStyle()
{
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #222831;"
        "}"
        "QGroupBox {"
        "  border: none;"
        "  border-radius: 8px;"
        "  margin-top: 10px;"
        "  color: #9DB2BF;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 8px;"
        "  padding: 0px 4px;"
        "}"
        "QTableWidget {"
        "  background-color: #1A2029;"
        "  alternate-background-color: #232B36;"
        "  border: none;"
        "  border-radius: 8px;"
        "  gridline-color: #303D49;"
        "  color: #DDE6ED;"
        "  font-size: 11px;"
        "  selection-background-color: #2B4C5F;"
        "  selection-color: #00FFCC;"
        "}"
        "QHeaderView::section {"
        "  background-color: #2B3542;"
        "  color: #A0A5B5;"
        "  padding: 4px;"
        "  border: none;"
        "  font-weight: bold;"
        "}"
        "QPushButton {"
        "  background-color: #303D49;"
        "  border: none;"
        "  border-radius: 2px;"
        "  color: #A0A5B5;"
        "  min-width: 90px;"
        "  height: 26px;"
        "  padding: 2px 10px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #323232;"
        "  color: #DDE6ED;"
        "  border-color: #00FFCC;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #1C222B;"
        "  border-color: #2D3542;"
        "  color: #555E6D;"
        "}"
        "QProgressBar {"
        "  border: none;"
        "  border-radius: 3px;"
        "  background: #1A2029;"
        "  color: #DDE6ED;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #00FFCC;"
        "  border-radius: 2px;"
        "}"
    ));
}

} // namespace presentation::views
