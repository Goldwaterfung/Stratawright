// src/Presentation/views/playlist/dialogs/RenderJobsDialog.h
#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QGroupBox>
#include "common/system_primitives.h"

namespace bridge {
class IRenderController;
}

namespace presentation::views {

/**
 * @brief Non-blocking queue manager window displaying active and queued render jobs.
 * Allows monitoring multi-stem and single bounce progress, inspecting output paths,
 * and cancelling background jobs safely.
 */
class RenderJobsDialog : public QDialog {
    Q_OBJECT
public:
    explicit RenderJobsDialog(bridge::IRenderController* renderCtrl, QWidget* parent = nullptr);
    ~RenderJobsDialog() override = default;

public slots:
    void refreshJobs();
    void selectJob(uint64_t jobId);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onJobSelectionChanged();
    void onCancelSelectedJob();
    void onShowInFinder();
    void onTimerTick();

private:
    void setupUI();
    void applyThemeStyle();
    uint64_t getSelectedJobId() const;
    QString formatStateText(RenderJobState state) const;

private:
    bridge::IRenderController* m_renderCtrl{nullptr};
    QTimer* m_refreshTimer{nullptr};

    QTableWidget* m_jobsTable{nullptr};

    // Details Panel
    QGroupBox*    m_detailsBox{nullptr};
    QLabel*       m_jobNameLabel{nullptr};
    QLabel*       m_jobStatusLabel{nullptr};
    QLabel*       m_jobPathLabel{nullptr};
    QProgressBar* m_jobProgressBar{nullptr};

    // Action buttons
    QPushButton*  m_btnCancelJob{nullptr};
    QPushButton*  m_btnShowInFinder{nullptr};
    QPushButton*  m_btnClose{nullptr};
};

} // namespace presentation::views
