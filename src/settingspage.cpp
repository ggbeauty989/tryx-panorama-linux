#include "settingspage.h"
#include "devicemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>
#include <QTextStream>

#include <panorama/config.hpp>

static const QString AUTO_PORT_LABEL = QStringLiteral("Auto");

SettingsPage::SettingsPage(DeviceManager *deviceMgr, QWidget *parent)
    : QWidget(parent), deviceMgr_(deviceMgr) {
    setupUi();
    loadSettings();
}

void SettingsPage::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Connection settings
    auto *portGroup = new QGroupBox("Connection");
    auto *portLayout = new QGridLayout(portGroup);

    portCombo_ = new QComboBox;
    portCombo_->setEditable(true);
    portCombo_->addItem(AUTO_PORT_LABEL);
    refreshPortsBtn_ = new QPushButton("Refresh");

    portLayout->addWidget(new QLabel("Port:"), 0, 0);
    portLayout->addWidget(portCombo_, 0, 1);
    portLayout->addWidget(refreshPortsBtn_, 0, 2);

    keepaliveSpin_ = new QSpinBox;
    keepaliveSpin_->setRange(5, 60);
    keepaliveSpin_->setValue(10);
    keepaliveSpin_->setSuffix(" s");

    portLayout->addWidget(new QLabel("Keepalive interval:"), 1, 0);
    portLayout->addWidget(keepaliveSpin_, 1, 1);

    mainLayout->addWidget(portGroup);

    connect(refreshPortsBtn_, &QPushButton::clicked, this, &SettingsPage::onRefreshPorts);

    // Behavior
    auto *behaviorGroup = new QGroupBox("Behavior");
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);

    cbMinimizeToTray_ = new QCheckBox("Minimize to tray on close");
    cbStartMinimized_ = new QCheckBox("Start minimized");
    cbAutostart_ = new QCheckBox("Autostart on login (systemd user service)");

    cbMinimizeToTray_->setChecked(true);

    behaviorLayout->addWidget(cbMinimizeToTray_);
    behaviorLayout->addWidget(cbStartMinimized_);
    behaviorLayout->addWidget(cbAutostart_);

    mainLayout->addWidget(behaviorGroup);

    // Device info
    auto *infoGroup = new QGroupBox("Device");
    auto *infoLayout = new QHBoxLayout(infoGroup);

    deviceInfoBtn_ = new QPushButton("Device information");
    infoLayout->addWidget(deviceInfoBtn_);
    infoLayout->addStretch();

    mainLayout->addWidget(infoGroup);

    connect(deviceInfoBtn_, &QPushButton::clicked, this, &SettingsPage::onShowDeviceInfo);

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    saveBtn_ = new QPushButton("Save");
    resetBtn_ = new QPushButton("Reset");
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn_);
    btnLayout->addWidget(resetBtn_);

    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();

    connect(saveBtn_, &QPushButton::clicked, this, &SettingsPage::onSaveSettings);
    connect(resetBtn_, &QPushButton::clicked, this, &SettingsPage::onResetSettings);

    // Initial port scan
    onRefreshPorts();
}

void SettingsPage::loadSettings() {
    auto config = panorama::ConfigManager::load_config();
    if (!config) return;

    if (!config->port.empty()) {
        portCombo_->setCurrentText(QString::fromStdString(config->port));
    }
    keepaliveSpin_->setValue(config->keepalive_interval);
    cbMinimizeToTray_->setChecked(config->minimize_to_tray);
    cbStartMinimized_->setChecked(config->start_minimized);
    cbAutostart_->setChecked(config->autostart);
}

QString SettingsPage::selectedPort() const {
    if (portCombo_->currentText() == AUTO_PORT_LABEL) {
        return {};
    }
    return portCombo_->currentText();
}

int SettingsPage::keepaliveInterval() const {
    return keepaliveSpin_->value();
}

bool SettingsPage::minimizeToTray() const {
    return cbMinimizeToTray_->isChecked();
}

bool SettingsPage::startMinimized() const {
    return cbStartMinimized_->isChecked();
}

void SettingsPage::onRefreshPorts() {
    QString current = portCombo_->currentText();
    portCombo_->clear();
    portCombo_->addItem(AUTO_PORT_LABEL);

    QDir devDir("/dev");
    for (const auto &entry : devDir.entryList(QStringList{"ttyACM*"}, QDir::System)) {
        portCombo_->addItem("/dev/" + entry);
    }

    int idx = portCombo_->findText(current);
    if (idx >= 0) {
        portCombo_->setCurrentIndex(idx);
    }
}

void SettingsPage::onShowDeviceInfo() {
    if (!deviceMgr_->isConnected()) {
        QMessageBox::information(this, "Device", "Device not connected");
        return;
    }

    // Trigger handshake - info will come through signals
    emit statusMessage("Requesting device information...");
}

void SettingsPage::onResetSettings() {
    portCombo_->setCurrentIndex(0);
    keepaliveSpin_->setValue(10);
    cbMinimizeToTray_->setChecked(true);
    cbStartMinimized_->setChecked(false);
    cbAutostart_->setChecked(false);
    emit statusMessage("Settings reset");
}

void SettingsPage::onSaveSettings() {
    // Preserve fields not managed by this page (e.g. brightness owned by
    // PanoramaPage) by starting from the existing on-disk config.
    panorama::Config config = panorama::ConfigManager::load_config().value_or(panorama::Config{});
    config.port = selectedPort().toStdString();
    config.keepalive_interval = keepaliveSpin_->value();
    config.minimize_to_tray = cbMinimizeToTray_->isChecked();
    config.start_minimized = cbStartMinimized_->isChecked();
    config.autostart = cbAutostart_->isChecked();

    panorama::ConfigManager::save_config(config);

    applyAutostart(config.autostart);

    emit settingsChanged();
    emit statusMessage("Settings saved");
}

void SettingsPage::applyAutostart(bool enable) {
    const QString serviceDir = QDir::homePath() + "/.config/systemd/user";
    const QString servicePath = serviceDir + "/tryx-panorama.service";
    const QString unitName = "tryx-panorama.service";

    if (enable) {
        QDir().mkpath(serviceDir);

        // Write a unit pointing at the *currently running* binary so the
        // user doesn't have to remember to install to ~/.local/bin first.
        const QString execPath = QCoreApplication::applicationFilePath();

        QFile f(servicePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            emit statusMessage("Autostart: could not write " + servicePath);
            return;
        }
        QTextStream out(&f);
        out << "[Unit]\n"
            << "Description=TRYX Panorama Display Manager\n"
            << "After=graphical-session.target\n"
            << "PartOf=graphical-session.target\n"
            << "\n"
            << "[Service]\n"
            << "Type=simple\n"
            << "ExecStart=" << execPath << "\n"
            << "Restart=on-failure\n"
            << "RestartSec=5\n"
            << "\n"
            << "[Install]\n"
            << "WantedBy=graphical-session.target\n";
        f.close();

        QProcess::execute("systemctl", {"--user", "daemon-reload"});
        const int rc = QProcess::execute("systemctl", {"--user", "enable", unitName});
        if (rc != 0) {
            emit statusMessage("Autostart: systemctl enable failed (rc=" + QString::number(rc) + ")");
        }
    } else {
        // Best-effort teardown; ignore errors if the unit was never installed.
        QProcess::execute("systemctl", {"--user", "disable", unitName});
        if (QFile::exists(servicePath)) {
            QFile::remove(servicePath);
            QProcess::execute("systemctl", {"--user", "daemon-reload"});
        }
    }
}
