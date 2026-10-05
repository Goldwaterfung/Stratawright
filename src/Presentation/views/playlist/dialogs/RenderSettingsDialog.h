// src/Presentation/views/playlist/dialogs/RenderSettingsDialog.h
#pragma once

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QTabWidget>
#include <QStackedWidget>
#include <QProgressBar>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTreeWidget>
#include <QTimer>
#include <vector>
#include "theme.h"
#include "common/system_primitives.h"

namespace bridge {
class IRenderController;
class ITimelineController;
class IArrangementController;
class ITrackController;
}

namespace presentation::views {

enum class RenderTab {
    MasterBounce = 0,
    StemExport = 1
};

/**
 * @brief Modal configuration and progress dialog for offline bouncing & stem exporting.
 * Contains high-fidelity options matching professional DAW capabilities and provides
 * both direct monitored rendering and non-blocking asynchronous queue submission.
 */
class RenderSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit RenderSettingsDialog(
        bridge::IRenderController* renderCtrl,
        bridge::ITimelineController* timelineCtrl,
        bridge::IArrangementController* arrangementCtrl,
        bridge::ITrackController* trackCtrl = nullptr,
        QWidget* parent = nullptr
    );
    ~RenderSettingsDialog() override = default;

    void setCurrentTab(RenderTab tab);

    // Config options
    QString getFileFormat() const;
    QString getSampleRate() const;
    QString getBitDepth() const;
    bool getEnableDither() const;
    bool getSplitPlanar() const;
    int getRenderRange() const; // 0: Full Song, 1: Loop Region

    uint64_t getActiveJobId() const { return m_activeJobId; }

signals:
    void jobEnqueued(uint64_t jobId);

private slots:
    void onFormatChanged(int index);
    void browseMasterOutputPath();
    void browseStemOutputDir();
    void onSelectAllTracks();
    void onClearAllTracks();
    void onSelectActiveTracks();
    void onStartBounceClicked();
    void onEnqueueClicked();
    void onSimulationTick();
    void cancelRender();
    void sendToBackground();

private:
    void setupUI();
    void applyThemeStyle();
    void populateCapabilities();
    void populateTracks();
    bool buildConfiguration(RenderConfiguration& outConfig, bool isStemTab);

private:
    // Core Layout Panels (Stacked)
    QStackedWidget* m_pagesStack{nullptr};

    // Page 0: Settings Panel
    QTabWidget*   m_tabWidget{nullptr};

    // --- Tab 0: Master Bounce Widgets ---
    QLineEdit*    m_masterPathEdit{nullptr};
    QPushButton*  m_btnMasterBrowse{nullptr};

    // Format & Quality Controls
    QComboBox*    m_formatCombo{nullptr};
    QComboBox*    m_sampleRateCombo{nullptr};
    QComboBox*    m_bitDepthCombo{nullptr};
    QCheckBox*    m_ditherCheck{nullptr};
    QCheckBox*    m_splitPlanarCheck{nullptr};

    // Normalization
    QCheckBox*      m_normalizeCheck{nullptr};
    QDoubleSpinBox* m_normTargetSpin{nullptr};

    // Bounds & Timeline Scope
    QRadioButton* m_fullSongRadio{nullptr};
    QRadioButton* m_loopRegionRadio{nullptr};
    QButtonGroup* m_rangeGroup{nullptr};
    QSpinBox*     m_tailDurationSpin{nullptr}; // ms

    // --- Tab 1: Stem Export Widgets ---
    QLineEdit*    m_stemDirEdit{nullptr};
    QPushButton*  m_btnStemBrowse{nullptr};
    QTreeWidget*  m_stemTrackTree{nullptr};
    QPushButton*  m_btnSelectAll{nullptr};
    QPushButton*  m_btnClearAll{nullptr};
    QPushButton*  m_btnSelectActive{nullptr};

    QComboBox*    m_stemNamingCombo{nullptr};
    QLineEdit*    m_customPatternEdit{nullptr};

    // Routing policies
    QCheckBox*    m_printAuxCheck{nullptr};
    QCheckBox*    m_keepSidechainCheck{nullptr};
    QCheckBox*    m_mutedGhostSidechainCheck{nullptr};

    // Action buttons on Page 0
    QPushButton*  m_btnEnqueue{nullptr};
    QPushButton*  m_btnRender{nullptr};
    QPushButton*  m_btnCancel{nullptr};

    // Page 1: Progress Simulation / Monitored Panel Widgets
    QLabel*       m_progressTitleLabel{nullptr};
    QLabel*       m_statusLabel{nullptr};
    QLabel*       m_itemDetailLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QPushButton*  m_btnCancelProgress{nullptr};
    QPushButton*  m_btnBackgroundProgress{nullptr};

    // Simulation / Monitoring variables
    QTimer*  m_simTimer{nullptr};
    uint64_t m_activeJobId{0};

    // Bridge Controllers
    bridge::IRenderController*      m_renderCtrl{nullptr};
    bridge::ITimelineController*   m_timelineCtrl{nullptr};
    bridge::IArrangementController* m_arrangementCtrl{nullptr};
    bridge::ITrackController*       m_trackCtrl{nullptr};

    // Cached engine capabilities
    std::vector<RenderFormat> m_supportedFormats;
    std::vector<uint32_t>     m_supportedSampleRates;
    std::vector<uint8_t>      m_supportedBitDepths;
};

} // namespace presentation::views
