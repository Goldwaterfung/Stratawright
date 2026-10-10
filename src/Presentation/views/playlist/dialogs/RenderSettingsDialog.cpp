#include "RenderSettingsDialog.h"
#include "Middle Bridge/engine/irender_controller.h"
#include "Middle Bridge/timeline/itimeline_controller.h"
#include "Middle Bridge/timeline/iarrangement_controller.h"
#include "Middle Bridge/tracks/itrack_controller.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QStandardPaths>
#include <cstring>

namespace presentation::views {

RenderSettingsDialog::RenderSettingsDialog(
    bridge::IRenderController* renderCtrl,
    bridge::ITimelineController* timelineCtrl,
    bridge::IArrangementController* arrangementCtrl,
    bridge::ITrackController* trackCtrl,
    QWidget* parent
)
    : QDialog(parent)
    , m_renderCtrl(renderCtrl)
    , m_timelineCtrl(timelineCtrl)
    , m_arrangementCtrl(arrangementCtrl)
    , m_trackCtrl(trackCtrl)
{
    setWindowTitle(QStringLiteral("Render / Export Engine"));
    setMinimumSize(560, 640);
    setModal(true);

    setupUI();
    populateCapabilities();
    populateTracks();
    applyThemeStyle();

    m_simTimer = new QTimer(this);
    connect(m_simTimer, &QTimer::timeout, this, &RenderSettingsDialog::onSimulationTick);
}

void RenderSettingsDialog::setCurrentTab(RenderTab tab)
{
    if (m_tabWidget) {
        m_tabWidget->setCurrentIndex(static_cast<int>(tab));
    }
}

void RenderSettingsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_pagesStack = new QStackedWidget(this);

    // ==========================================
    // PAGE 0: Settings Panel
    // ==========================================
    auto* settingsPage = new QWidget(this);
    auto* settingsLayout = new QVBoxLayout(settingsPage);
    settingsLayout->setContentsMargins(16, 16, 16, 16);
    settingsLayout->setSpacing(12);

    // Header label
    auto* headerLabel = new QLabel(QStringLiteral("OFFLINE RENDER ENGINE CONFIG"), settingsPage);
    headerLabel->setFont(theme::Font::monospace(11, QFont::Bold));
    headerLabel->setStyleSheet(QStringLiteral("color: #00FFCC;"));
    settingsLayout->addWidget(headerLabel);

    // Tab Widget
    m_tabWidget = new QTabWidget(settingsPage);
    m_tabWidget->setFont(theme::Font::primary(11, QFont::Bold));

    // -------------------------------------------------------------------------
    // TAB 0: Master Bounce
    // -------------------------------------------------------------------------
    auto* masterTab = new QWidget();
    auto* masterLayout = new QVBoxLayout(masterTab);
    masterLayout->setContentsMargins(12, 12, 12, 12);
    masterLayout->setSpacing(10);

    // Output file picker group
    auto* masterFileBox = new QGroupBox(QStringLiteral("Target Mixdown Output"), masterTab);
    masterFileBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* masterFileLayout = new QHBoxLayout(masterFileBox);
    masterFileLayout->setContentsMargins(10, 14, 10, 10);
    masterFileLayout->setSpacing(8);

    m_masterPathEdit = new QLineEdit(masterFileBox);
    m_masterPathEdit->setFont(theme::Font::monospace(10));
    m_masterPathEdit->setPlaceholderText(QStringLiteral("Select target audio mixdown path..."));
    QString defaultMusicDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_masterPathEdit->setText(defaultMusicDir + QStringLiteral("/Mixdown.wav"));

    m_btnMasterBrowse = new QPushButton(QStringLiteral("Browse..."), masterFileBox);
    m_btnMasterBrowse->setFont(theme::Font::primary(10));
    connect(m_btnMasterBrowse, &QPushButton::clicked, this, &RenderSettingsDialog::browseMasterOutputPath);

    masterFileLayout->addWidget(m_masterPathEdit);
    masterFileLayout->addWidget(m_btnMasterBrowse);
    masterLayout->addWidget(masterFileBox);

    // Shared Format & Quality Box
    auto* qualityBox = new QGroupBox(QStringLiteral("Audio Format & Precision"), masterTab);
    qualityBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* qualityLayout = new QGridLayout(qualityBox);
    qualityLayout->setContentsMargins(10, 14, 10, 10);
    qualityLayout->setSpacing(8);

    auto* formatLabel = new QLabel(QStringLiteral("Format:"), qualityBox);
    formatLabel->setFont(theme::Font::primary(10, QFont::Bold));
    formatLabel->setStyleSheet(QStringLiteral("color: #a0a5b5;"));
    m_formatCombo = new QComboBox(qualityBox);
    m_formatCombo->setFont(theme::Font::monospace(10));
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RenderSettingsDialog::onFormatChanged);
    qualityLayout->addWidget(formatLabel, 0, 0);
    qualityLayout->addWidget(m_formatCombo, 0, 1);

    auto* srLabel = new QLabel(QStringLiteral("Sample Rate:"), qualityBox);
    srLabel->setFont(theme::Font::primary(10, QFont::Bold));
    srLabel->setStyleSheet(QStringLiteral("color: #a0a5b5;"));
    m_sampleRateCombo = new QComboBox(qualityBox);
    m_sampleRateCombo->setFont(theme::Font::monospace(10));
    qualityLayout->addWidget(srLabel, 1, 0);
    qualityLayout->addWidget(m_sampleRateCombo, 1, 1);

    auto* bdLabel = new QLabel(QStringLiteral("Bit Depth:"), qualityBox);
    bdLabel->setFont(theme::Font::primary(10, QFont::Bold));
    bdLabel->setStyleSheet(QStringLiteral("color: #a0a5b5;"));
    m_bitDepthCombo = new QComboBox(qualityBox);
    m_bitDepthCombo->setFont(theme::Font::monospace(10));
    qualityLayout->addWidget(bdLabel, 2, 0);
    qualityLayout->addWidget(m_bitDepthCombo, 2, 1);

    masterLayout->addWidget(qualityBox);

    // Master Post-Processing Box
    auto* optionsBox = new QGroupBox(QStringLiteral("Post-Processing & Normalization"), masterTab);
    optionsBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* optionsLayout = new QVBoxLayout(optionsBox);
    optionsLayout->setContentsMargins(10, 14, 10, 10);
    optionsLayout->setSpacing(6);

    m_ditherCheck = new QCheckBox(QStringLiteral("Inject TPDF psychoacoustic dither (16/24-bit truncation)"), optionsBox);
    m_ditherCheck->setChecked(true);
    m_ditherCheck->setFont(theme::Font::primary(10));
    optionsLayout->addWidget(m_ditherCheck);

    m_splitPlanarCheck = new QCheckBox(QStringLiteral("Export Split L/R Planar audio files"), optionsBox);
    m_splitPlanarCheck->setChecked(false);
    m_splitPlanarCheck->setFont(theme::Font::primary(10));
    optionsLayout->addWidget(m_splitPlanarCheck);

    auto* normRow = new QHBoxLayout();
    normRow->setSpacing(8);
    m_normalizeCheck = new QCheckBox(QStringLiteral("Normalize Peak Amplitude to:"), optionsBox);
    m_normalizeCheck->setChecked(false);
    m_normalizeCheck->setFont(theme::Font::primary(10));

    m_normTargetSpin = new QDoubleSpinBox(optionsBox);
    m_normTargetSpin->setRange(-24.0, 0.0);
    m_normTargetSpin->setSingleStep(0.1);
    m_normTargetSpin->setValue(-0.1);
    m_normTargetSpin->setSuffix(QStringLiteral(" dBFS"));
    m_normTargetSpin->setEnabled(false);
    m_normTargetSpin->setFont(theme::Font::monospace(10));
    connect(m_normalizeCheck, &QCheckBox::toggled, m_normTargetSpin, &QDoubleSpinBox::setEnabled);

    normRow->addWidget(m_normalizeCheck);
    normRow->addWidget(m_normTargetSpin);
    normRow->addStretch();
    optionsLayout->addLayout(normRow);

    masterLayout->addWidget(optionsBox);

    // Bounds Box
    auto* scopeBox = new QGroupBox(QStringLiteral("Timeline Bounds & Tail"), masterTab);
    scopeBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* scopeLayout = new QVBoxLayout(scopeBox);
    scopeLayout->setContentsMargins(10, 14, 10, 10);
    scopeLayout->setSpacing(6);

    m_rangeGroup = new QButtonGroup(this);
    m_rangeGroup->setExclusive(true);

    m_fullSongRadio = new QRadioButton(QStringLiteral("Render entire arrangement timeline"), scopeBox);
    m_fullSongRadio->setChecked(true);
    m_fullSongRadio->setFont(theme::Font::primary(10));
    m_rangeGroup->addButton(m_fullSongRadio, 0);

    m_loopRegionRadio = new QRadioButton(QStringLiteral("Render active timeline loop selection"), scopeBox);
    m_loopRegionRadio->setFont(theme::Font::primary(10));
    m_rangeGroup->addButton(m_loopRegionRadio, 1);

    scopeLayout->addWidget(m_fullSongRadio);
    scopeLayout->addWidget(m_loopRegionRadio);

    auto* tailRow = new QHBoxLayout();
    tailRow->setSpacing(8);
    auto* tailLabel = new QLabel(QStringLiteral("Reverb / Delay Tail:"), scopeBox);
    tailLabel->setFont(theme::Font::primary(10));
    tailLabel->setStyleSheet(QStringLiteral("color: #a0a5b5;"));

    m_tailDurationSpin = new QSpinBox(scopeBox);
    m_tailDurationSpin->setRange(0, 30000);
    m_tailDurationSpin->setSingleStep(500);
    m_tailDurationSpin->setValue(1000);
    m_tailDurationSpin->setSuffix(QStringLiteral(" ms"));
    m_tailDurationSpin->setFont(theme::Font::monospace(10));

    connect(m_loopRegionRadio, &QRadioButton::toggled, this, [this](bool loopChecked) {
        m_tailDurationSpin->setEnabled(!loopChecked);
    });

    tailRow->addWidget(tailLabel);
    tailRow->addWidget(m_tailDurationSpin);
    tailRow->addStretch();
    scopeLayout->addLayout(tailRow);

    masterLayout->addWidget(scopeBox);
    masterLayout->addStretch();
    m_tabWidget->addTab(masterTab, QStringLiteral("Master Bounce"));

    // -------------------------------------------------------------------------
    // TAB 1: Stem Export
    // -------------------------------------------------------------------------
    auto* stemTab = new QWidget();
    auto* stemLayout = new QVBoxLayout(stemTab);
    stemLayout->setContentsMargins(12, 12, 12, 12);
    stemLayout->setSpacing(10);

    // Stem directory picker
    auto* stemDirBox = new QGroupBox(QStringLiteral("Destination Directory"), stemTab);
    stemDirBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* stemDirLayout = new QHBoxLayout(stemDirBox);
    stemDirLayout->setContentsMargins(10, 14, 10, 10);
    stemDirLayout->setSpacing(8);

    m_stemDirEdit = new QLineEdit(stemDirBox);
    m_stemDirEdit->setFont(theme::Font::monospace(10));
    m_stemDirEdit->setPlaceholderText(QStringLiteral("Select folder for stems..."));
    m_stemDirEdit->setText(defaultMusicDir + QStringLiteral("/Stems"));

    m_btnStemBrowse = new QPushButton(QStringLiteral("Browse..."), stemDirBox);
    m_btnStemBrowse->setFont(theme::Font::primary(10));
    connect(m_btnStemBrowse, &QPushButton::clicked, this, &RenderSettingsDialog::browseStemOutputDir);

    stemDirLayout->addWidget(m_stemDirEdit);
    stemDirLayout->addWidget(m_btnStemBrowse);
    stemLayout->addWidget(stemDirBox);

    // Track Selection Tree
    auto* trackListBox = new QGroupBox(QStringLiteral("Tracks to Export"), stemTab);
    trackListBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* trackListLayout = new QVBoxLayout(trackListBox);
    trackListLayout->setContentsMargins(10, 14, 10, 10);
    trackListLayout->setSpacing(6);

    auto* trackBtnBar = new QHBoxLayout();
    trackBtnBar->setSpacing(6);

    m_btnSelectAll = new QPushButton(QStringLiteral("Select All"), trackListBox);
    m_btnSelectAll->setFont(theme::Font::primary(9));
    connect(m_btnSelectAll, &QPushButton::clicked, this, &RenderSettingsDialog::onSelectAllTracks);

    m_btnClearAll = new QPushButton(QStringLiteral("Clear All"), trackListBox);
    m_btnClearAll->setFont(theme::Font::primary(9));
    connect(m_btnClearAll, &QPushButton::clicked, this, &RenderSettingsDialog::onClearAllTracks);

    m_btnSelectActive = new QPushButton(QStringLiteral("Active Only"), trackListBox);
    m_btnSelectActive->setFont(theme::Font::primary(9));
    connect(m_btnSelectActive, &QPushButton::clicked, this, &RenderSettingsDialog::onSelectActiveTracks);

    trackBtnBar->addWidget(m_btnSelectAll);
    trackBtnBar->addWidget(m_btnClearAll);
    trackBtnBar->addWidget(m_btnSelectActive);
    trackBtnBar->addStretch();
    trackListLayout->addLayout(trackBtnBar);

    m_stemTrackTree = new QTreeWidget(trackListBox);
    m_stemTrackTree->setHeaderLabels({
        QStringLiteral("Track"),
        QStringLiteral("Name"),
        QStringLiteral("Type"),
        QStringLiteral("State")
    });
    m_stemTrackTree->header()->setStretchLastSection(true);
    m_stemTrackTree->setColumnWidth(0, 90);
    m_stemTrackTree->setColumnWidth(1, 160);
    m_stemTrackTree->setColumnWidth(2, 90);
    m_stemTrackTree->setFont(theme::Font::monospace(10));
    m_stemTrackTree->setRootIsDecorated(false);
    trackListLayout->addWidget(m_stemTrackTree, 1);

    stemLayout->addWidget(trackListBox, 1);

    // Stem Naming & Policies
    auto* stemOptionsBox = new QGroupBox(QStringLiteral("Stem Naming & Routing"), stemTab);
    stemOptionsBox->setFont(theme::Font::monospace(10, QFont::Bold));
    auto* stemOptionsLayout = new QVBoxLayout(stemOptionsBox);
    stemOptionsLayout->setContentsMargins(10, 14, 10, 10);
    stemOptionsLayout->setSpacing(6);

    auto* namingRow = new QHBoxLayout();
    namingRow->setSpacing(8);
    auto* namingLabel = new QLabel(QStringLiteral("Pattern:"), stemOptionsBox);
    namingLabel->setFont(theme::Font::primary(10));
    namingLabel->setStyleSheet(QStringLiteral("color: #a0a5b5;"));

    m_stemNamingCombo = new QComboBox(stemOptionsBox);
    m_stemNamingCombo->setFont(theme::Font::monospace(10));
    m_stemNamingCombo->addItem(QStringLiteral("{track_index:02d}_{track_name}"), QStringLiteral("{track_index:02d}_{track_name}"));
    m_stemNamingCombo->addItem(QStringLiteral("{project}_{track_name}"), QStringLiteral("{project}_{track_name}"));
    m_stemNamingCombo->addItem(QStringLiteral("{track_index:02d}_{project}_{track_name}"), QStringLiteral("{track_index:02d}_{project}_{track_name}"));
    m_stemNamingCombo->addItem(QStringLiteral("Custom Pattern..."), QStringLiteral("custom"));

    m_customPatternEdit = new QLineEdit(stemOptionsBox);
    m_customPatternEdit->setFont(theme::Font::monospace(10));
    m_customPatternEdit->setPlaceholderText(QStringLiteral("{track_index:02d}_{track_name}"));
    m_customPatternEdit->setVisible(false);

    connect(m_stemNamingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        m_customPatternEdit->setVisible(idx == 3);
    });

    namingRow->addWidget(namingLabel);
    namingRow->addWidget(m_stemNamingCombo, 1);
    namingRow->addWidget(m_customPatternEdit, 1);
    stemOptionsLayout->addLayout(namingRow);

    m_printAuxCheck = new QCheckBox(QStringLiteral("Print Aux Return effects into each stem (wet bounce)"), stemOptionsBox);
    m_printAuxCheck->setChecked(false);
    m_printAuxCheck->setFont(theme::Font::primary(10));
    stemOptionsLayout->addWidget(m_printAuxCheck);

    m_keepSidechainCheck = new QCheckBox(QStringLiteral("Keep Sidechain ducking/modulation active across stems"), stemOptionsBox);
    m_keepSidechainCheck->setChecked(true);
    m_keepSidechainCheck->setFont(theme::Font::primary(10));
    stemOptionsLayout->addWidget(m_keepSidechainCheck);

    m_mutedGhostSidechainCheck = new QCheckBox(QStringLiteral("Allow muted tracks to trigger sidechain inputs"), stemOptionsBox);
    m_mutedGhostSidechainCheck->setChecked(true);
    m_mutedGhostSidechainCheck->setFont(theme::Font::primary(10));
    stemOptionsLayout->addWidget(m_mutedGhostSidechainCheck);

    stemLayout->addWidget(stemOptionsBox);
    m_tabWidget->addTab(stemTab, QStringLiteral("Stem Export"));

    settingsLayout->addWidget(m_tabWidget, 1);

    // Bottom Controls Bar
    auto* btnWidget = new QWidget(settingsPage);
    auto* btnLayout = new QHBoxLayout(btnWidget);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(10);

    m_btnEnqueue = new QPushButton(QStringLiteral("ENQUEUE TO QUEUE"), btnWidget);
    m_btnEnqueue->setFont(theme::Font::monospace(10, QFont::Bold));
    m_btnEnqueue->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #232B36;"
        "  border: none;"
        "  color: #DDE6ED;"
        "  min-width: 140px;"
        "  height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #323232;"
        "  border-color: #00FFCC;"
        "}"
    ));
    connect(m_btnEnqueue, &QPushButton::clicked, this, &RenderSettingsDialog::onEnqueueClicked);

    m_btnCancel = new QPushButton(QStringLiteral("Cancel"), btnWidget);
    m_btnCancel->setFont(theme::Font::primary(10));
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnRender = new QPushButton(QStringLiteral("START BOUNCE"), btnWidget);
    m_btnRender->setFont(theme::Font::monospace(10, QFont::Bold));
    m_btnRender->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #323232;"
        "  border: none;"
        "  border-radius: 8px;"
        "  color: #00D2B4;"
        "  min-width: 110px;"
        "  height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #00D2B4;"
        "  color: #161616;"
        "}"
    ));
    connect(m_btnRender, &QPushButton::clicked, this, &RenderSettingsDialog::onStartBounceClicked);

    btnLayout->addWidget(m_btnEnqueue);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnRender);
    settingsLayout->addWidget(btnWidget);

    m_pagesStack->addWidget(settingsPage);

    // ==========================================
    // PAGE 1: Active Progress Panel
    // ==========================================
    auto* progressPage = new QWidget(this);
    auto* progressLayout = new QVBoxLayout(progressPage);
    progressLayout->setContentsMargins(24, 24, 24, 24);
    progressLayout->setSpacing(16);
    progressLayout->addStretch();

    m_progressTitleLabel = new QLabel(QStringLiteral("OFFLINE RENDER IN PROGRESS"), progressPage);
    m_progressTitleLabel->setFont(theme::Font::monospace(12, QFont::Bold));
    m_progressTitleLabel->setAlignment(Qt::AlignCenter);
    m_progressTitleLabel->setStyleSheet(QStringLiteral("color: #00FFCC;"));
    progressLayout->addWidget(m_progressTitleLabel);

    m_statusLabel = new QLabel(QStringLiteral("Initializing render engine..."), progressPage);
    m_statusLabel->setFont(theme::Font::primary(11));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #a0a5b5;"));
    progressLayout->addWidget(m_statusLabel);

    m_itemDetailLabel = new QLabel(QStringLiteral(""), progressPage);
    m_itemDetailLabel->setFont(theme::Font::monospace(10));
    m_itemDetailLabel->setAlignment(Qt::AlignCenter);
    m_itemDetailLabel->setStyleSheet(QStringLiteral("color: #00FFCC;"));
    progressLayout->addWidget(m_itemDetailLabel);

    m_progressBar = new QProgressBar(progressPage);
    m_progressBar->setFixedHeight(18);
    m_progressBar->setFont(theme::Font::monospace(10, QFont::Bold));
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setAlignment(Qt::AlignCenter);
    progressLayout->addWidget(m_progressBar);

    auto* wrapProgressButtons = new QWidget(progressPage);
    auto* wrapProgressLayout = new QHBoxLayout(wrapProgressButtons);
    wrapProgressLayout->setContentsMargins(0, 0, 0, 0);
    wrapProgressLayout->setSpacing(12);

    m_btnCancelProgress = new QPushButton(QStringLiteral("CANCEL BOUNCE"), progressPage);
    m_btnCancelProgress->setFont(theme::Font::monospace(10, QFont::Bold));
    m_btnCancelProgress->setFixedWidth(140);
    connect(m_btnCancelProgress, &QPushButton::clicked, this, &RenderSettingsDialog::cancelRender);

    m_btnBackgroundProgress = new QPushButton(QStringLiteral("SEND TO BACKGROUND"), progressPage);
    m_btnBackgroundProgress->setFont(theme::Font::monospace(10, QFont::Bold));
    m_btnBackgroundProgress->setFixedWidth(160);
    connect(m_btnBackgroundProgress, &QPushButton::clicked, this, &RenderSettingsDialog::sendToBackground);

    wrapProgressLayout->addStretch();
    wrapProgressLayout->addWidget(m_btnCancelProgress);
    wrapProgressLayout->addWidget(m_btnBackgroundProgress);
    wrapProgressLayout->addStretch();
    progressLayout->addWidget(wrapProgressButtons);

    progressLayout->addStretch();
    m_pagesStack->addWidget(progressPage);

    mainLayout->addWidget(m_pagesStack);
}

void RenderSettingsDialog::populateCapabilities()
{
    m_supportedFormats.clear();
    m_supportedSampleRates.clear();
    m_supportedBitDepths.clear();

    if (m_renderCtrl) {
        m_renderCtrl->getSupportedCapabilities(m_supportedFormats, m_supportedSampleRates, m_supportedBitDepths);
    }

    // 1. Formats
    m_formatCombo->clear();
    if (!m_supportedFormats.empty()) {
        for (auto fmt : m_supportedFormats) {
            switch (fmt) {
                case RenderFormat::WAV:
                    m_formatCombo->addItem(QStringLiteral("Waveform Audio File (.wav)"), static_cast<int>(fmt));
                    break;
                case RenderFormat::FLAC:
                    m_formatCombo->addItem(QStringLiteral("Free Lossless Audio Codec (.flac)"), static_cast<int>(fmt));
                    break;
                case RenderFormat::MP3:
                    m_formatCombo->addItem(QStringLiteral("MPEG-1 Audio Layer III (.mp3)"), static_cast<int>(fmt));
                    break;
                case RenderFormat::AIFF:
                    m_formatCombo->addItem(QStringLiteral("Audio Interchange File Format (.aiff)"), static_cast<int>(fmt));
                    break;
                case RenderFormat::OGG:
                    m_formatCombo->addItem(QStringLiteral("Ogg Vorbis Audio (.ogg)"), static_cast<int>(fmt));
                    break;
            }
        }
    } else {
        m_formatCombo->addItem(QStringLiteral("Waveform Audio File (.wav)"), static_cast<int>(RenderFormat::WAV));
        m_formatCombo->addItem(QStringLiteral("Free Lossless Audio Codec (.flac)"), static_cast<int>(RenderFormat::FLAC));
        m_formatCombo->addItem(QStringLiteral("MPEG-1 Audio Layer III (.mp3)"), static_cast<int>(RenderFormat::MP3));
    }

    // 2. Sample Rates
    m_sampleRateCombo->clear();
    uint32_t activeRate = m_timelineCtrl ? static_cast<uint32_t>(m_timelineCtrl->getSampleRate()) : 48000;
    int selectedSrIndex = 0;

    if (!m_supportedSampleRates.empty()) {
        for (size_t i = 0; i < m_supportedSampleRates.size(); ++i) {
            uint32_t rate = m_supportedSampleRates[i];
            double khz = static_cast<double>(rate) / 1000.0;
            m_sampleRateCombo->addItem(QStringLiteral("%1 kHz").arg(khz, 0, 'f', 1), rate);
            if (rate == activeRate) {
                selectedSrIndex = static_cast<int>(i);
            }
        }
    } else {
        const uint32_t standardRates[] = {44100, 48000, 88200, 96000};
        for (int i = 0; i < 4; ++i) {
            uint32_t rate = standardRates[i];
            double khz = static_cast<double>(rate) / 1000.0;
            m_sampleRateCombo->addItem(QStringLiteral("%1 kHz").arg(khz, 0, 'f', 1), rate);
            if (rate == activeRate) {
                selectedSrIndex = i;
            }
        }
    }
    m_sampleRateCombo->setCurrentIndex(selectedSrIndex);

    // 3. Bit Depths
    m_bitDepthCombo->clear();
    if (!m_supportedBitDepths.empty()) {
        for (auto depth : m_supportedBitDepths) {
            if (depth == 16) {
                m_bitDepthCombo->addItem(QStringLiteral("16-bit Fixed Point"), 16);
            } else if (depth == 24) {
                m_bitDepthCombo->addItem(QStringLiteral("24-bit Fixed Point"), 24);
            } else if (depth == 32) {
                m_bitDepthCombo->addItem(QStringLiteral("32-bit Floating Point"), 32);
            }
        }
    } else {
        m_bitDepthCombo->addItem(QStringLiteral("16-bit Fixed Point"), 16);
        m_bitDepthCombo->addItem(QStringLiteral("24-bit Fixed Point"), 24);
        m_bitDepthCombo->addItem(QStringLiteral("32-bit Floating Point"), 32);
    }
    // Default 24-bit or 32-bit if available
    int defaultDepthIdx = m_bitDepthCombo->findData(24);
    if (defaultDepthIdx < 0) defaultDepthIdx = m_bitDepthCombo->findData(32);
    if (defaultDepthIdx >= 0) m_bitDepthCombo->setCurrentIndex(defaultDepthIdx);
}

void RenderSettingsDialog::populateTracks()
{
    m_stemTrackTree->clear();
    if (!m_trackCtrl) return;

    std::vector<bridge::TrackUIState> tracks = m_trackCtrl->getAllTracks();
    for (size_t i = 0; i < tracks.size(); ++i) {
        const auto& t = tracks[i];

        auto* item = new QTreeWidgetItem(m_stemTrackTree);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, t.isMuted ? Qt::Unchecked : Qt::Checked);

        item->setText(0, QStringLiteral("#%1").arg(i + 1));
        item->setText(1, QString::fromUtf8(t.name));

        QString typeStr;
        switch (t.type) {
            case composition::TrackType::AUDIO:      typeStr = QStringLiteral("Audio"); break;
            case composition::TrackType::INSTRUMENT: typeStr = QStringLiteral("Instrument"); break;
            case composition::TrackType::AUX:        typeStr = QStringLiteral("Aux"); break;
            case composition::TrackType::FOLDER:     typeStr = QStringLiteral("Folder"); break;
            default:                                 typeStr = QStringLiteral("Track"); break;
        }
        item->setText(2, typeStr);

        QString stateStr = QStringLiteral("Normal");
        if (t.isSoloed) {
            stateStr = QStringLiteral("Solo");
            item->setForeground(3, QBrush(QColor(QStringLiteral("#2DA87E"))));
        } else if (t.isMuted) {
            stateStr = QStringLiteral("Muted");
            item->setForeground(3, QBrush(QColor(QStringLiteral("#D4A84B"))));
        }
        item->setText(3, stateStr);

        item->setData(0, Qt::UserRole, static_cast<qulonglong>(t.trackId.id));
    }
}

void RenderSettingsDialog::onFormatChanged(int /*index*/)
{
    int fmtData = m_formatCombo->currentData().toInt();
    QString ext = QStringLiteral(".wav");
    if (fmtData == static_cast<int>(RenderFormat::FLAC)) ext = QStringLiteral(".flac");
    else if (fmtData == static_cast<int>(RenderFormat::MP3)) ext = QStringLiteral(".mp3");
    else if (fmtData == static_cast<int>(RenderFormat::AIFF)) ext = QStringLiteral(".aiff");
    else if (fmtData == static_cast<int>(RenderFormat::OGG)) ext = QStringLiteral(".ogg");

    QString currentPath = m_masterPathEdit->text();
    if (!currentPath.isEmpty()) {
        qsizetype dotIdx = currentPath.lastIndexOf('.');
        if (dotIdx > 0) {
            m_masterPathEdit->setText(currentPath.left(dotIdx) + ext);
        }
    }
}

void RenderSettingsDialog::browseMasterOutputPath()
{
    int fmtData = m_formatCombo->currentData().toInt();
    QString filter;
    QString ext;
    if (fmtData == static_cast<int>(RenderFormat::FLAC)) {
        filter = tr("FLAC Audio (*.flac)");
        ext = QStringLiteral(".flac");
    } else if (fmtData == static_cast<int>(RenderFormat::MP3)) {
        filter = tr("MP3 Audio (*.mp3)");
        ext = QStringLiteral(".mp3");
    } else if (fmtData == static_cast<int>(RenderFormat::AIFF)) {
        filter = tr("AIFF Audio (*.aiff)");
        ext = QStringLiteral(".aiff");
    } else if (fmtData == static_cast<int>(RenderFormat::OGG)) {
        filter = tr("Ogg Vorbis Audio (*.ogg)");
        ext = QStringLiteral(".ogg");
    } else {
        filter = tr("Waveform Audio File (*.wav)");
        ext = QStringLiteral(".wav");
    }

    QString current = m_masterPathEdit->text();
    QString path = QFileDialog::getSaveFileName(this, tr("Select Render Target"), current, filter);
    if (!path.isEmpty()) {
        if (!path.endsWith(ext, Qt::CaseInsensitive)) {
            path += ext;
        }
        m_masterPathEdit->setText(path);
    }
}

void RenderSettingsDialog::browseStemOutputDir()
{
    QString current = m_stemDirEdit->text();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Stem Destination Folder"), current);
    if (!dir.isEmpty()) {
        m_stemDirEdit->setText(dir);
    }
}

void RenderSettingsDialog::onSelectAllTracks()
{
    for (int i = 0; i < m_stemTrackTree->topLevelItemCount(); ++i) {
        m_stemTrackTree->topLevelItem(i)->setCheckState(0, Qt::Checked);
    }
}

void RenderSettingsDialog::onClearAllTracks()
{
    for (int i = 0; i < m_stemTrackTree->topLevelItemCount(); ++i) {
        m_stemTrackTree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    }
}

void RenderSettingsDialog::onSelectActiveTracks()
{
    for (int i = 0; i < m_stemTrackTree->topLevelItemCount(); ++i) {
        auto* item = m_stemTrackTree->topLevelItem(i);
        bool isMuted = (item->text(3) == QStringLiteral("Muted"));
        item->setCheckState(0, isMuted ? Qt::Unchecked : Qt::Checked);
    }
}

bool RenderSettingsDialog::buildConfiguration(RenderConfiguration& outConfig, bool isStemTab)
{
    std::memset(&outConfig, 0, sizeof(RenderConfiguration));

    // Shared Format
    int fmtData = m_formatCombo->currentData().toInt();
    outConfig.format = static_cast<RenderFormat>(fmtData);

    outConfig.bitDepth = static_cast<uint8_t>(m_bitDepthCombo->currentData().toUInt());
    outConfig.sampleRate = m_sampleRateCombo->currentData().toUInt();
    outConfig.enableDither = m_ditherCheck->isChecked();
    outConfig.splitPlanar = m_splitPlanarCheck->isChecked();

    // Normalization
    outConfig.normalize = m_normalizeCheck->isChecked();
    outConfig.normalizationdB = static_cast<float>(m_normTargetSpin->value());

    // Timeline Bounds
    if (m_loopRegionRadio->isChecked()) {
        outConfig.useLoopRegion = true;
        outConfig.startFrame = m_timelineCtrl ? m_timelineCtrl->getLoopStart() : 0;
        outConfig.endFrame = m_timelineCtrl ? m_timelineCtrl->getLoopEnd() : 0;
        if (outConfig.endFrame <= outConfig.startFrame) {
            uint64_t dynamicLength = m_arrangementCtrl ? m_arrangementCtrl->getArrangementLength() : 0;
            outConfig.endFrame = (dynamicLength > 0) ? dynamicLength : outConfig.sampleRate;
        }
    } else {
        outConfig.useLoopRegion = false;
        outConfig.startFrame = 0;
        outConfig.tailDurationMs = static_cast<uint32_t>(m_tailDurationSpin->value());

        uint64_t dynamicLength = m_arrangementCtrl ? m_arrangementCtrl->getArrangementLength() : 0;
        if (dynamicLength > 0) {
            outConfig.endFrame = dynamicLength;
        } else {
            // Strictly deterministic fallback based on tempo & signature without hardcoded 5-minute magic number
            double bpm = m_timelineCtrl ? m_timelineCtrl->getBPM() : 120.0;
            if (bpm <= 0.0) bpm = 120.0;
            uint64_t barFrames = static_cast<uint64_t>(static_cast<double>(outConfig.sampleRate) * (4.0 * 60.0 / bpm));
            outConfig.endFrame = barFrames;
        }
    }

    if (isStemTab) {
        outConfig.stemExport = true;
        QString dir = m_stemDirEdit->text().trimmed();
        if (dir.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Directory"), tr("Please specify a target directory for stem export."));
            return false;
        }
        std::strncpy(outConfig.outputFilePath, dir.toUtf8().constData(), sizeof(outConfig.outputFilePath) - 1);

        uint32_t selectedCount = 0;
        for (int i = 0; i < m_stemTrackTree->topLevelItemCount(); ++i) {
            auto* item = m_stemTrackTree->topLevelItem(i);
            if (item->checkState(0) == Qt::Checked) {
                if (selectedCount < MAX_STEM_TRACKS) {
                    uint32_t rawId = static_cast<uint32_t>(item->data(0, Qt::UserRole).toULongLong());
                    outConfig.stemTrackIds[selectedCount++] = rawId;
                }
            }
        }
        outConfig.numStemTracks = selectedCount;

        if (selectedCount == 0) {
            QMessageBox::warning(this, tr("No Tracks Selected"), tr("Please select at least one track to export as a stem."));
            return false;
        }

        // Stem Naming pattern
        QString pattern;
        if (m_stemNamingCombo->currentIndex() == 3) {
            pattern = m_customPatternEdit->text().trimmed();
            if (pattern.isEmpty()) pattern = QStringLiteral("{track_index:02d}_{track_name}");
        } else {
            pattern = m_stemNamingCombo->currentData().toString();
        }
        std::strncpy(outConfig.stemNamingPattern, pattern.toUtf8().constData(), sizeof(outConfig.stemNamingPattern) - 1);

        outConfig.printAuxReturnsIntoStems = m_printAuxCheck->isChecked();
        outConfig.activeSidechainsDuringExport = m_keepSidechainCheck->isChecked();
        outConfig.allowMutedGhostSidechainTriggers = m_mutedGhostSidechainCheck->isChecked();
    } else {
        outConfig.stemExport = false;
        outConfig.numStemTracks = 0;
        QString path = m_masterPathEdit->text().trimmed();
        if (path.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Path"), tr("Please specify a target file path for the master mixdown."));
            return false;
        }
        std::strncpy(outConfig.outputFilePath, path.toUtf8().constData(), sizeof(outConfig.outputFilePath) - 1);
    }

    return true;
}

void RenderSettingsDialog::onStartBounceClicked()
{
    if (!m_renderCtrl) return;

    bool isStem = (m_tabWidget->currentIndex() == static_cast<int>(RenderTab::StemExport));
    RenderConfiguration config;
    if (!buildConfiguration(config, isStem)) return;

    uint64_t jobId = m_renderCtrl->enqueueRenderJob(config);
    if (jobId == 0) {
        QMessageBox::critical(this, tr("Render Failed"), tr("Could not launch render job. Please check logs."));
        return;
    }

    m_activeJobId = jobId;
    m_progressBar->setValue(0);
    m_statusLabel->setText(QStringLiteral("Initializing render buffers..."));
    m_itemDetailLabel->setText(isStem ? tr("Preparing stem exports...") : QStringLiteral(""));
    m_progressTitleLabel->setText(isStem ? tr("STEM EXPORT IN PROGRESS") : tr("OFFLINE BOUNCE IN PROGRESS"));

    m_pagesStack->setCurrentIndex(1);
    m_simTimer->start(100);
}

void RenderSettingsDialog::onEnqueueClicked()
{
    if (!m_renderCtrl) return;

    bool isStem = (m_tabWidget->currentIndex() == static_cast<int>(RenderTab::StemExport));
    RenderConfiguration config;
    if (!buildConfiguration(config, isStem)) return;

    uint64_t jobId = m_renderCtrl->enqueueRenderJob(config);
    if (jobId == 0) {
        QMessageBox::critical(this, tr("Queue Failed"), tr("Could not enqueue render job."));
        return;
    }

    m_activeJobId = jobId;
    emit jobEnqueued(jobId);
    accept();
}

void RenderSettingsDialog::onSimulationTick()
{
    if (!m_renderCtrl || m_activeJobId == 0) return;

    RenderJobInfo info{};
    if (m_renderCtrl->getJobInfo(m_activeJobId, info)) {
        m_progressBar->setValue(static_cast<int>(info.progress * 100.0f));
        m_statusLabel->setText(QString::fromUtf8(info.statusMessage));

        if (info.isStemExport && info.totalItemCount > 0) {
            m_itemDetailLabel->setText(tr("Track %1 of %2: %3")
                .arg(info.currentItemIndex)
                .arg(info.totalItemCount)
                .arg(QString::fromUtf8(info.jobName)));
        }

        if (info.state == RenderJobState::COMPLETED) {
            m_simTimer->stop();
            m_progressBar->setValue(100);
            m_statusLabel->setText(tr("Render completed successfully!"));
            QTimer::singleShot(400, this, &QDialog::accept);
        } else if (info.state == RenderJobState::FAILED) {
            m_simTimer->stop();
            QMessageBox::critical(this, tr("Render Failed"), QString::fromUtf8(info.statusMessage));
            m_pagesStack->setCurrentIndex(0);
        } else if (info.state == RenderJobState::CANCELLED) {
            m_simTimer->stop();
            m_pagesStack->setCurrentIndex(0);
        }
    } else {
        // Fallback for single job legacy
        char errorMsg[256];
        if (m_renderCtrl->hasFailed(errorMsg, sizeof(errorMsg))) {
            m_simTimer->stop();
            QMessageBox::critical(this, tr("Render Failed"), QString::fromUtf8(errorMsg));
            m_pagesStack->setCurrentIndex(0);
            return;
        }

        float progress = m_renderCtrl->getRenderProgress();
        m_progressBar->setValue(static_cast<int>(progress * 100.0f));
        m_statusLabel->setText(QString::fromUtf8(m_renderCtrl->getRenderStatusMessage()));

        if (!m_renderCtrl->isRenderingActive() && progress >= 1.0f) {
            m_simTimer->stop();
            m_statusLabel->setText(tr("Render complete!"));
            QTimer::singleShot(200, this, &QDialog::accept);
        }
    }
}

void RenderSettingsDialog::sendToBackground()
{
    m_simTimer->stop();
    if (m_activeJobId != 0) {
        emit jobEnqueued(m_activeJobId);
    }
    accept();
}

void RenderSettingsDialog::cancelRender()
{
    if (m_renderCtrl) {
        if (m_activeJobId != 0) {
            m_renderCtrl->cancelJob(m_activeJobId);
        } else {
            m_renderCtrl->cancelOfflineRender();
        }
    }
    m_simTimer->stop();
    m_progressBar->setValue(0);
    m_pagesStack->setCurrentIndex(0);
}

QString RenderSettingsDialog::getFileFormat() const
{
    return m_formatCombo->currentText();
}

QString RenderSettingsDialog::getSampleRate() const
{
    return m_sampleRateCombo->currentText();
}

QString RenderSettingsDialog::getBitDepth() const
{
    return m_bitDepthCombo->currentText();
}

bool RenderSettingsDialog::getEnableDither() const
{
    return m_ditherCheck->isChecked();
}

bool RenderSettingsDialog::getSplitPlanar() const
{
    return m_splitPlanarCheck->isChecked();
}

int RenderSettingsDialog::getRenderRange() const
{
    if (m_loopRegionRadio->isChecked()) return 1;
    return 0;
}

void RenderSettingsDialog::applyThemeStyle()
{
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #222831;"
        "}"
        "QTabWidget::pane {"
        "  border: none;"
        "  border-radius: 8px;"
        "  background-color: #222831;"
        "}"
        "QTabBar::tab {"
        "  background: #1A2029;"
        "  color: #A0A5B5;"
        "  padding: 6px 16px;"
        "  border: none;"
        "  border-bottom: none;"
        "  border-top-left-radius: 4px;"
        "  border-top-right-radius: 4px;"
        "  min-width: 100px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #222831;"
        "  color: #00FFCC;"
        "  border-color: #323232;"
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
        "QLineEdit, QSpinBox, QDoubleSpinBox {"
        "  background-color: #1A2029;"
        "  border: none;"
        "  border-radius: 2px;"
        "  color: #DDE6ED;"
        "  padding: 3px 6px;"
        "}"
        "QComboBox {"
        "  background-color: #303D49;"
        "  border: none;"
        "  border-radius: 2px;"
        "  color: #DDE6ED;"
        "  padding: 2px 6px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 16px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border: none;"
        "  background-color: #323232;"
        "  width: 4px;"
        "  height: 4px;"
        "}"
        "QTreeWidget {"
        "  background-color: #1A2029;"
        "  border: none;"
        "  border-radius: 2px;"
        "  color: #DDE6ED;"
        "}"
        "QHeaderView::section {"
        "  background-color: #2B3542;"
        "  color: #A0A5B5;"
        "  padding: 3px;"
        "  border: none;"
        "}"
        "QPushButton {"
        "  background-color: #303D49;"
        "  border: none;"
        "  border-radius: 2px;"
        "  color: #a0a5b5;"
        "  min-width: 80px;"
        "  height: 26px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #323232;"
        "  color: #DDE6ED;"
        "  border-color: #00FFCC;"
        "}"
        "QCheckBox::indicator, QRadioButton::indicator {"
        "  width: 14px;"
        "  height: 14px;"
        "  border: none;"
        "  background: #303D49;"
        "  border-radius: 2px;"
        "}"
        "QRadioButton::indicator {"
        "  border-radius: 7px;"
        "}"
        "QCheckBox::indicator:checked, QRadioButton::indicator:checked {"
        "  background: #00FFCC;"
        "  border-color: #00FFCC;"
        "}"
        "QProgressBar {"
        "  border: none;"
        "  border-radius: 8px;"
        "  background: #303D49;"
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

