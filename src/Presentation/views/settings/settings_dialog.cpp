// src/Presentation/views/settings/settings_dialog.cpp
#include "settings_dialog.h"
#include "../theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPainter>
#include <QPen>
#include <QGraphicsDropShadowEffect>

namespace presentation::views {

namespace {

QString getSettingsStyle()
{
    return QString(
        "QDialog {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: 12px;"
        "}"
        "QLabel {"
        "    color: %4;"
        "    font-family: 'Inter';"
        "    font-size: 14px;"
        "}"
        "QLabel#titleLabel {"
        "    color: %5;"
        "    font-size: 17px;"
        "    font-weight: bold;"
        "    font-family: 'Inter';"
        "}"
        "QLabel#errorLabel {"
        "    color: %6;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QGroupBox {"
        "    color: %2;"
        "    font-family: 'Inter';"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    border: none;"
        "    border-radius: 12px;"
        "    margin-top: 12px;"
        "    padding-top: 12px;"
        "    background-color: %7;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0px 5px;"
        "    color: %5;"
        "}"
        "QComboBox {"
        "    background-color: %3;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 4px 8px;"
        "    min-width: 180px;"
        "    font-family: 'Inter';"
        "    font-size: 14px;"
        "}"
        "QComboBox:hover {"
        "    background-color: #3A4454;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: top right;"
        "    width: 20px;"
        "    border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: %7;"
        "    color: %2;"
        "    border: none;"
        "    selection-background-color: %3;"
        "    selection-color: %5;"
        "    outline: none;"
        "}"
        "QProgressBar {"
        "    border: none;"
        "    border-radius: 4px;"
        "    text-align: center;"
        "    background-color: %1;"
        "    color: %2;"
        "    font-family: 'JetBrains Mono';"
        "    font-size: 13px;"
        "    height: 16px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: %5;"
        "    width: 4px;"
        "    margin: 0.5px;"
        "}"
        "QPushButton {"
        "    color: %4;"
        "    background-color: %3;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px 14px;"
        "    font-family: 'Inter';"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    min-width: 75px;"
        "}"
        "QPushButton:hover {"
        "    color: %2;"
        "    background-color: #3A4454;"
        "}"
        "QPushButton:pressed {"
        "    color: %5;"
        "    background-color: %7;"
        "}"
        "QPushButton#applyBtn {"
        "    color: %5;"
        "    border: none;"
        "    background-color: %1;"
        "}"
        "QPushButton#applyBtn:hover {"
        "    background-color: %7;"
        "    color: %5;"
        "}"
        "QListWidget {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 4px;"
        "    font-family: 'Inter';"
        "    font-size: 14px;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 6px;"
        "    border-radius: 6px;"
        "    color: %2;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: %3;"
        "    color: %5;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #3A4454;"
        "    color: %5;"
        "}"
        "QCheckBox {"
        "    color: %2;"
        "    font-family: 'Inter';"
        "    font-size: 14px;"
        "}"
    ).arg(theme::Color::BgBase.name())        // %1
     .arg(theme::Color::TextPrimary.name())   // %2
     .arg(theme::Color::BgControl.name())     // %3
     .arg(theme::Color::TextMuted.name())     // %4
     .arg(theme::Color::AccentGlow.name())    // %5
     .arg(theme::Color::AccentRecord.name())  // %6
     .arg(theme::Color::BgSurface.name());    // %7
}

} // anonymous namespace

SettingsDialog::SettingsDialog(bridge::IHardwareSettingsFacade* facade, QWidget* parent)
    : QDialog(parent)
    , m_facade(facade)
{
    // Apply sleek modern window settings
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
    setWindowTitle("Hardware Preferences");
    resize(720, 520);
    setStyleSheet(getSettingsStyle());

    // Initial setup of components
    setupUI();
    loadCurrentConfig();

    // Start 10Hz telemetry monitoring loop
    m_telemetryTimer = new QTimer(this);
    connect(m_telemetryTimer, &QTimer::timeout, this, &SettingsDialog::onTelemetryTick);
    m_telemetryTimer->start(100); // 100ms interval
    
    // Trigger first immediate tick
    onTelemetryTick();
}

void SettingsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // --- Header Section ---
    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("HARDWARE PREFERENCES", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setFont(theme::Font::primary(13, QFont::Bold, 110.0));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // --- Center Split Layout ---
    auto* centerLayout = new QHBoxLayout();
    centerLayout->setSpacing(15);

    // Left Column Layout
    auto* leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(12);

    // --- Audio Hardware Settings Group ---
    auto* audioGroup = new QGroupBox("AUDIO CONFIGURATION", this);
    auto* audioGroupLayout = new QFormLayout(audioGroup);
    audioGroupLayout->setContentsMargins(15, 20, 15, 15);
    audioGroupLayout->setSpacing(12);
    audioGroupLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_inputDeviceCombo = new QComboBox(this);
    audioGroupLayout->addRow("Input Device:", m_inputDeviceCombo);

    m_outputDeviceCombo = new QComboBox(this);
    audioGroupLayout->addRow("Output Device:", m_outputDeviceCombo);

    m_sampleRateCombo = new QComboBox(this);
    audioGroupLayout->addRow("Sample Rate:", m_sampleRateCombo);

    m_bufferSizeCombo = new QComboBox(this);
    audioGroupLayout->addRow("Buffer Size:", m_bufferSizeCombo);

    leftColumn->addWidget(audioGroup);

    // --- MIDI Hardware Settings Group ---
    auto* midiGroup = new QGroupBox("MIDI CONFIGURATION", this);
    auto* midiGroupLayout = new QVBoxLayout(midiGroup);
    midiGroupLayout->setContentsMargins(15, 20, 15, 15);
    midiGroupLayout->setSpacing(8);

    auto* midiInfoLabel = new QLabel("Enable / Disable input MIDI devices:", this);
    midiGroupLayout->addWidget(midiInfoLabel);

    m_midiListWidget = new QListWidget(this);
    m_midiListWidget->setMinimumHeight(100);
    midiGroupLayout->addWidget(m_midiListWidget);

    leftColumn->addWidget(midiGroup);

    centerLayout->addLayout(leftColumn, 1);

    // Right Column Layout
    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(12);

    // --- Telemetry & System Status Group ---
    auto* telemetryGroup = new QGroupBox("SYSTEM PERFORMANCE & TELEMETRY", this);
    auto* telemetryGroupLayout = new QFormLayout(telemetryGroup);
    telemetryGroupLayout->setContentsMargins(15, 20, 15, 15);
    telemetryGroupLayout->setSpacing(10);

    // CPU load meter
    auto* cpuRowLayout = new QHBoxLayout();
    m_cpuProgressBar = new QProgressBar(this);
    m_cpuProgressBar->setRange(0, 100);
    m_cpuProgressBar->setValue(0);
    m_cpuLabel = new QLabel("0%", this);
    m_cpuLabel->setFont(theme::Font::monospace(9, QFont::Bold));
    m_cpuLabel->setStyleSheet(QString("color: %1; min-width: 35px;").arg(theme::Color::AccentGlow.name()));
    cpuRowLayout->addWidget(m_cpuProgressBar, 1);
    cpuRowLayout->addWidget(m_cpuLabel);
    telemetryGroupLayout->addRow("Real-Time CPU:", cpuRowLayout);

    // Latency
    m_latencyLabel = new QLabel("0.0 ms (Roundtrip)", this);
    m_latencyLabel->setFont(theme::Font::monospace(9, QFont::Normal));
    m_latencyLabel->setStyleSheet(QString("color: %1;").arg(theme::Color::TextPrimary.name()));
    telemetryGroupLayout->addRow("System Latency:", m_latencyLabel);

    // Dropouts
    m_xrunsLabel = new QLabel("0 Buffer Dropouts (Xruns)", this);
    m_xrunsLabel->setFont(theme::Font::monospace(9, QFont::Normal));
    m_xrunsLabel->setStyleSheet(QString("color: %1;").arg(theme::Color::TextPrimary.name()));
    telemetryGroupLayout->addRow("Stream Stability:", m_xrunsLabel);

    rightColumn->addWidget(telemetryGroup);
    rightColumn->addStretch(); // Align top matching vertical heights perfectly

    centerLayout->addLayout(rightColumn, 1);

    mainLayout->addLayout(centerLayout);

    // --- Error Indicator Banner ---
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    mainLayout->addWidget(m_errorLabel);

    // --- Bottom Controls Area ---
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(8);

    auto* applyBtn = new QPushButton("Apply", this);
    applyBtn->setObjectName("applyBtn");
    bottomLayout->addWidget(applyBtn);

    bottomLayout->addStretch();

    auto* cancelBtn = new QPushButton("Cancel", this);
    bottomLayout->addWidget(cancelBtn);

    auto* okBtn = new QPushButton("OK", this);
    bottomLayout->addWidget(okBtn);

    mainLayout->addLayout(bottomLayout);

    // Connect control actions
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApplyPressed);
    connect(okBtn, &QPushButton::clicked, this, &SettingsDialog::onOkPressed);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancelPressed);
    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onInputDeviceChanged);
    connect(m_outputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onOutputDeviceChanged);
    connect(m_sampleRateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onSampleRateChanged);
    connect(m_bufferSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onBufferSizeChanged);
}

void SettingsDialog::loadCurrentConfig()
{
    if (!m_facade) return;

    // Load available audio hardware devices
    m_devices = m_facade->getAvailableDevices();
    m_inputDeviceCombo->clear();
    m_outputDeviceCombo->clear();
    
    // Add None options for ultimate flexibility
    m_inputDeviceCombo->addItem("[No Input Device]", QVariant(0xFFFFFFFFU));
    m_outputDeviceCombo->addItem("[No Output Device]", QVariant(0xFFFFFFFFU));

    bridge::HardwareConfig currentConfig = m_facade->getCurrentConfig();
    m_initialConfig = currentConfig; // Cache for cancel revertions

    int activeInputIdx = 0;
    int activeOutputIdx = 0;

    for (const auto& dev : m_devices) {
        if (dev.maxInputChannels > 0) {
            m_inputDeviceCombo->addItem(dev.name, QVariant(dev.deviceIndex));
            if (dev.deviceIndex == currentConfig.inputDeviceIndex) {
                activeInputIdx = m_inputDeviceCombo->count() - 1;
            }
        }
        if (dev.maxOutputChannels > 0) {
            m_outputDeviceCombo->addItem(dev.name, QVariant(dev.deviceIndex));
            if (dev.deviceIndex == currentConfig.outputDeviceIndex) {
                activeOutputIdx = m_outputDeviceCombo->count() - 1;
            }
        }
    }

    m_inputDeviceCombo->setCurrentIndex(activeInputIdx);
    m_outputDeviceCombo->setCurrentIndex(activeOutputIdx);

    // Populate dynamic sample rates supported by the active output device
    uint32_t activeOutDevIdx = m_outputDeviceCombo->currentData().toUInt();
    updateSampleRatesForDevice(activeOutDevIdx, currentConfig.sampleRate);
    updateBufferSizesForDevice(activeOutDevIdx, currentConfig.bufferSize, currentConfig.sampleRate);

    // Load available MIDI hardware devices
    m_midiPorts = m_facade->getAvailableMidiPorts();
    m_midiListWidget->clear();
    m_initialOpenMidiPorts.clear();

    for (const auto& port : m_midiPorts) {
        auto* item = new QListWidgetItem(QString::fromUtf8(port.name), m_midiListWidget);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        
        if (port.isOpen) {
            item->setCheckState(Qt::Checked);
            m_initialOpenMidiPorts.push_back(port.portIndex);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
        
        item->setData(Qt::UserRole, port.portIndex);
    }
}

void SettingsDialog::onInputDeviceChanged(int index)
{
    if (index < 0) return;
    uint32_t devIdx = m_inputDeviceCombo->itemData(index).toUInt();
    if (devIdx == 0xFFFFFFFFU) {
        m_inputDeviceCombo->setToolTip("No Audio Input selected");
        return;
    }
    
    for (const auto& dev : m_devices) {
        if (dev.deviceIndex == devIdx) {
            m_inputDeviceCombo->setToolTip(QString("Device: %1\nMax Input Channels: %2")
                                           .arg(dev.name)
                                           .arg(dev.maxInputChannels));
            break;
        }
    }
}

void SettingsDialog::updateSampleRatesForDevice(uint32_t deviceIndex, uint32_t preferredRate)
{
    m_sampleRateCombo->blockSignals(true);
    m_sampleRateCombo->clear();

    std::vector<uint32_t> rates;
    const bridge::AudioDeviceDescriptor* matchedDev = nullptr;

    for (const auto& dev : m_devices) {
        if (dev.deviceIndex == deviceIndex) {
            matchedDev = &dev;
            break;
        }
    }

    if (matchedDev && matchedDev->numSampleRates > 0) {
        for (uint32_t r = 0; r < matchedDev->numSampleRates; ++r) {
            rates.push_back(matchedDev->supportedSampleRates[r]);
        }
    } else if (matchedDev && matchedDev->defaultSampleRate > 0) {
        rates.push_back(matchedDev->defaultSampleRate);
    }

    if (preferredRate > 0 && std::find(rates.begin(), rates.end(), preferredRate) == rates.end()) {
        rates.push_back(preferredRate);
    }

    std::sort(rates.begin(), rates.end());
    rates.erase(std::unique(rates.begin(), rates.end()), rates.end());

    int selectIndex = -1;
    for (int i = 0; i < static_cast<int>(rates.size()); ++i) {
        uint32_t r = rates[static_cast<size_t>(i)];
        m_sampleRateCombo->addItem(QString::number(r) + " Hz", QVariant(r));
        if (preferredRate > 0 && r == preferredRate) {
            selectIndex = i;
        } else if (selectIndex == -1 && matchedDev && r == matchedDev->defaultSampleRate) {
            selectIndex = i;
        }
    }

    if (selectIndex != -1) {
        m_sampleRateCombo->setCurrentIndex(selectIndex);
    } else if (m_sampleRateCombo->count() > 0) {
        m_sampleRateCombo->setCurrentIndex(0);
    }

    m_sampleRateCombo->blockSignals(false);
}

void SettingsDialog::updateBufferSizesForDevice(uint32_t deviceIndex, uint32_t preferredBufferSize, uint32_t currentSampleRate)
{
    m_bufferSizeCombo->blockSignals(true);
    m_bufferSizeCombo->clear();

    const bridge::AudioDeviceDescriptor* matchedDev = nullptr;
    for (const auto& dev : m_devices) {
        if (dev.deviceIndex == deviceIndex) {
            matchedDev = &dev;
            break;
        }
    }

    uint32_t activeRate = currentSampleRate;
    if (activeRate == 0 && m_sampleRateCombo) {
        activeRate = m_sampleRateCombo->currentData().toUInt();
        if (activeRate == 0) {
            QString srText = m_sampleRateCombo->currentText();
            srText.remove(" Hz");
            activeRate = srText.toUInt();
        }
    }
    if (activeRate == 0 && matchedDev) {
        activeRate = matchedDev->defaultSampleRate;
    }

    std::vector<uint32_t> sizes = {32, 64, 128, 256, 512, 1024, 2048};
    if (matchedDev && matchedDev->preferredBufferSize > 0) {
        sizes.push_back(matchedDev->preferredBufferSize);
    }
    if (preferredBufferSize > 0) {
        sizes.push_back(preferredBufferSize);
    }

    std::sort(sizes.begin(), sizes.end());
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());

    int selectIndex = -1;
    uint32_t targetSelection = (preferredBufferSize > 0) ? preferredBufferSize : 
                               ((matchedDev && matchedDev->preferredBufferSize > 0) ? matchedDev->preferredBufferSize : 0);

    for (int i = 0; i < static_cast<int>(sizes.size()); ++i) {
        uint32_t s = sizes[static_cast<size_t>(i)];
        QString itemText;
        if (activeRate > 0) {
            double bufferDurationMs = (static_cast<double>(s) / static_cast<double>(activeRate)) * 1000.0;
            double roundtripMs = 2.0 * bufferDurationMs;
            itemText = QString("%1 Samples (%2 ms)").arg(s).arg(roundtripMs, 0, 'f', 1);
        } else {
            itemText = QString("%1 Samples").arg(s);
        }

        m_bufferSizeCombo->addItem(itemText, QVariant(s));

        if (targetSelection > 0 && s == targetSelection) {
            selectIndex = i;
        }
    }

    if (selectIndex != -1) {
        m_bufferSizeCombo->setCurrentIndex(selectIndex);
    } else if (m_bufferSizeCombo->count() > 0) {
        int defaultIdx = m_bufferSizeCombo->findData(QVariant(256U));
        if (defaultIdx == -1) defaultIdx = m_bufferSizeCombo->findData(QVariant(512U));
        if (defaultIdx == -1) defaultIdx = 0;
        m_bufferSizeCombo->setCurrentIndex(defaultIdx);
    }

    m_bufferSizeCombo->blockSignals(false);
    updateEstimatedLatency();
}

void SettingsDialog::updateEstimatedLatency()
{
    if (!m_facade || !m_sampleRateCombo || !m_bufferSizeCombo || !m_latencyLabel) return;

    uint32_t chosenRate = m_sampleRateCombo->currentData().toUInt();
    if (chosenRate == 0) {
        QString srText = m_sampleRateCombo->currentText();
        srText.remove(" Hz");
        chosenRate = srText.toUInt();
    }

    uint32_t chosenBufferSize = m_bufferSizeCombo->currentData().toUInt();
    if (chosenBufferSize == 0) {
        QString bsText = m_bufferSizeCombo->currentText();
        auto spaceIdx = bsText.indexOf(' ');
        if (spaceIdx > 0) {
            chosenBufferSize = bsText.left(spaceIdx).toUInt();
        }
    }

    double activeStreamLatency = m_facade->getLatencyMs();
    if (chosenRate > 0 && chosenBufferSize > 0) {
        double pendingLatency = 2.0 * (static_cast<double>(chosenBufferSize) / static_cast<double>(chosenRate)) * 1000.0;
        bridge::HardwareConfig currentConfig = m_facade->getCurrentConfig();
        bool isPending = (chosenRate != currentConfig.sampleRate || chosenBufferSize != currentConfig.bufferSize);

        if (isPending && activeStreamLatency > 0.0) {
            m_latencyLabel->setText(QString("%1 ms (Current) → %2 ms (Pending)").arg(activeStreamLatency, 0, 'f', 2).arg(pendingLatency, 0, 'f', 2));
        } else {
            m_latencyLabel->setText(QString("%1 ms (Roundtrip buffer duration)").arg(pendingLatency, 0, 'f', 2));
        }
    } else if (activeStreamLatency > 0.0) {
        m_latencyLabel->setText(QString("%1 ms (Roundtrip buffer duration)").arg(activeStreamLatency, 0, 'f', 2));
    }
}

void SettingsDialog::onSampleRateChanged(int index)
{
    if (index < 0) return;
    uint32_t chosenRate = m_sampleRateCombo->currentData().toUInt();
    if (chosenRate == 0) {
        QString srText = m_sampleRateCombo->currentText();
        srText.remove(" Hz");
        chosenRate = srText.toUInt();
    }

    uint32_t currentBufferSize = m_bufferSizeCombo->currentData().toUInt();
    uint32_t activeOutDevIdx = m_outputDeviceCombo->currentData().toUInt();

    updateBufferSizesForDevice(activeOutDevIdx, currentBufferSize, chosenRate);
}

void SettingsDialog::onBufferSizeChanged(int index)
{
    if (index < 0) return;
    updateEstimatedLatency();
}

void SettingsDialog::onOutputDeviceChanged(int index)
{
    if (index < 0) return;
    uint32_t devIdx = m_outputDeviceCombo->itemData(index).toUInt();
    if (devIdx == 0xFFFFFFFFU) {
        m_outputDeviceCombo->setToolTip("No Audio Output selected");
        updateSampleRatesForDevice(devIdx, 0);
        updateBufferSizesForDevice(devIdx, 0, 0);
        return;
    }
    
    for (const auto& dev : m_devices) {
        if (dev.deviceIndex == devIdx) {
            m_outputDeviceCombo->setToolTip(QString("Device: %1\nMax Output Channels: %2\nDefault Sample Rate: %3 Hz\nPreferred Buffer Size: %4 frames")
                                           .arg(dev.name)
                                           .arg(dev.maxOutputChannels)
                                           .arg(dev.defaultSampleRate)
                                           .arg(dev.preferredBufferSize));
            updateSampleRatesForDevice(devIdx, dev.defaultSampleRate);
            updateBufferSizesForDevice(devIdx, dev.preferredBufferSize, dev.defaultSampleRate);
            break;
        }
    }
}

void SettingsDialog::onTelemetryTick()
{
    refreshTelemetry();
}

void SettingsDialog::refreshTelemetry()
{
    if (!m_facade) return;

    // Double precision coordinates and parameters matching rules
    double cpuLoad = m_facade->getCpuLoad();
    uint32_t xruns = m_facade->getXrunCount();

    // 1. CPU Bar (ensure within 0.0 to 100.0)
    double cpuPercentage = qBound(0.0, cpuLoad * 100.0, 100.0);
    m_cpuProgressBar->setValue(static_cast<int>(cpuPercentage));
    m_cpuLabel->setText(QString("%1%").arg(cpuPercentage, 0, 'f', 1));

    // 2. Latency display dynamically updated based on current & pending settings
    updateEstimatedLatency();

    // 3. Buffer dropouts stability display
    if (xruns > 0) {
        m_xrunsLabel->setText(QString("%1 Dropouts detected").arg(xruns));
        m_xrunsLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme::Color::AccentRecord.name()));
    } else {
        m_xrunsLabel->setText("Stable (0 Buffer Dropouts)");
        m_xrunsLabel->setStyleSheet(QString("color: %1;").arg(theme::Color::TextPrimary.name()));
    }
}

void SettingsDialog::onApplyPressed()
{
    if (!m_facade) return;

    clearError();

    bridge::HardwareConfig targetConfig;

    // Get input device selection
    int inputIdx = m_inputDeviceCombo->currentIndex();
    uint32_t inputDevIdx = m_inputDeviceCombo->itemData(inputIdx).toUInt();
    
    if (inputDevIdx != 0xFFFFFFFFU) {
        targetConfig.inputDeviceIndex = inputDevIdx;
        uint32_t maxInChannels = 0;
        for (const auto& dev : m_devices) {
            if (dev.deviceIndex == inputDevIdx) {
                maxInChannels = dev.maxInputChannels;
                break;
            }
        }
        targetConfig.numInputChannels = std::min(2U, maxInChannels);
    } else {
        targetConfig.inputDeviceIndex = 0xFFFFFFFFU;
        targetConfig.numInputChannels = 0;
    }

    // Get output device selection
    int outputIdx = m_outputDeviceCombo->currentIndex();
    uint32_t outputDevIdx = m_outputDeviceCombo->itemData(outputIdx).toUInt();

    if (outputDevIdx != 0xFFFFFFFFU) {
        targetConfig.outputDeviceIndex = outputDevIdx;
        uint32_t maxOutChannels = 0;
        for (const auto& dev : m_devices) {
            if (dev.deviceIndex == outputDevIdx) {
                maxOutChannels = dev.maxOutputChannels;
                break;
            }
        }
        targetConfig.numOutputChannels = std::min(2U, maxOutChannels);
    } else {
        targetConfig.outputDeviceIndex = 0xFFFFFFFFU;
        targetConfig.numOutputChannels = 0;
    }

    // Extract numerical Sample Rate
    uint32_t chosenRate = m_sampleRateCombo->currentData().toUInt();
    if (chosenRate == 0) {
        QString srText = m_sampleRateCombo->currentText();
        srText.remove(" Hz");
        chosenRate = srText.toUInt();
    }
    targetConfig.sampleRate = chosenRate;

    // Extract numerical Buffer Size
    uint32_t chosenBufferSize = m_bufferSizeCombo->currentData().toUInt();
    if (chosenBufferSize == 0) {
        QString bsText = m_bufferSizeCombo->currentText();
        auto spaceIdx = bsText.indexOf(' ');
        if (spaceIdx > 0) {
            chosenBufferSize = bsText.left(spaceIdx).toUInt();
        } else {
            bsText.remove(" Samples");
            chosenBufferSize = bsText.toUInt();
        }
    }
    targetConfig.bufferSize = chosenBufferSize;

    // Apply audio config
    bool success = m_facade->applyConfig(targetConfig);
    if (!success) {
        showError("Failed to initialize target hardware configuration. Reverting settings...");
    }

    // B. Apply MIDI device configurations
    for (int i = 0; i < m_midiListWidget->count(); ++i) {
        QListWidgetItem* item = m_midiListWidget->item(i);
        uint32_t portIndex = item->data(Qt::UserRole).toUInt();
        bool isChecked = (item->checkState() == Qt::Checked);
        
        bool currentlyOpen = m_facade->isMidiPortOpen(portIndex);
        if (isChecked != currentlyOpen) {
            m_facade->setMidiPortOpen(portIndex, isChecked);
        }
    }

    // C. Re-read active configuration to keep GUI fully synchronized
    loadCurrentConfig();
}

void SettingsDialog::onOkPressed()
{
    onApplyPressed();
    if (m_errorLabel->text().isEmpty()) {
        accept(); // Close and return QDialog::Accepted
    }
}

void SettingsDialog::onCancelPressed()
{
    if (m_facade) {
        // Revert to initial loaded audio state
        m_facade->applyConfig(m_initialConfig);

        // Revert to initial loaded MIDI state
        auto availablePorts = m_facade->getAvailableMidiPorts();
        for (const auto& port : availablePorts) {
            bool wasInitiallyOpen = std::find(m_initialOpenMidiPorts.begin(),
                                              m_initialOpenMidiPorts.end(),
                                              port.portIndex) != m_initialOpenMidiPorts.end();
            if (port.isOpen != wasInitiallyOpen) {
                m_facade->setMidiPortOpen(port.portIndex, wasInitiallyOpen);
            }
        }
    }
    reject(); // Close and return QDialog::Rejected
}

void SettingsDialog::showError(const QString& message)
{
    m_errorLabel->setText(message);
}

void SettingsDialog::clearError()
{
    m_errorLabel->setText("");
}

void SettingsDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Paint a gorgeous neon framing border matching theme
    QPen borderPen(theme::Color::BgControl, 1.5);
    painter.setPen(borderPen);
    
    QRectF bounds(0.5, 0.5, static_cast<double>(width() - 1), static_cast<double>(height() - 1));
    painter.drawRoundedRect(bounds, 8.0, 8.0);
}

} // namespace presentation::views
