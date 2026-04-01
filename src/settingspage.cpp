#include "settingspage.h"
#include "devicemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QDir>
#include <QProcess>

#include <reed/config.hpp>

SettingsPage::SettingsPage(DeviceManager *deviceMgr, QWidget *parent)
    : QWidget(parent), deviceMgr_(deviceMgr) {
    setupUi();
    loadSettings();
}

void SettingsPage::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Port settings
    auto *portGroup = new QGroupBox("Подключение");
    auto *portLayout = new QGridLayout(portGroup);

    portCombo_ = new QComboBox;
    portCombo_->setEditable(true);
    portCombo_->addItem("Автоматически");
    refreshPortsBtn_ = new QPushButton("Обновить");

    portLayout->addWidget(new QLabel("Порт:"), 0, 0);
    portLayout->addWidget(portCombo_, 0, 1);
    portLayout->addWidget(refreshPortsBtn_, 0, 2);

    keepaliveSpin_ = new QSpinBox;
    keepaliveSpin_->setRange(5, 60);
    keepaliveSpin_->setValue(10);
    keepaliveSpin_->setSuffix(" сек");

    portLayout->addWidget(new QLabel("Keepalive интервал:"), 1, 0);
    portLayout->addWidget(keepaliveSpin_, 1, 1);

    mainLayout->addWidget(portGroup);

    connect(refreshPortsBtn_, &QPushButton::clicked, this, &SettingsPage::onRefreshPorts);

    // Behavior
    auto *behaviorGroup = new QGroupBox("Поведение");
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);

    cbMinimizeToTray_ = new QCheckBox("Сворачивать в трей при закрытии");
    cbStartMinimized_ = new QCheckBox("Запускать свёрнутым");
    cbAutostart_ = new QCheckBox("Автозапуск при входе (systemd user service)");

    cbMinimizeToTray_->setChecked(true);

    behaviorLayout->addWidget(cbMinimizeToTray_);
    behaviorLayout->addWidget(cbStartMinimized_);
    behaviorLayout->addWidget(cbAutostart_);

    mainLayout->addWidget(behaviorGroup);

    // Device info
    auto *infoGroup = new QGroupBox("Устройство");
    auto *infoLayout = new QHBoxLayout(infoGroup);

    deviceInfoBtn_ = new QPushButton("Информация об устройстве");
    infoLayout->addWidget(deviceInfoBtn_);
    infoLayout->addStretch();

    mainLayout->addWidget(infoGroup);

    connect(deviceInfoBtn_, &QPushButton::clicked, this, &SettingsPage::onShowDeviceInfo);

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    saveBtn_ = new QPushButton("Сохранить");
    resetBtn_ = new QPushButton("Сбросить");
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
    auto config = reed::ConfigManager::load_config();
    if (config) {
        if (!config->port.empty()) {
            portCombo_->setCurrentText(QString::fromStdString(config->port));
        }
        keepaliveSpin_->setValue(config->keepalive_interval);
    }
}

QString SettingsPage::selectedPort() const {
    if (portCombo_->currentText() == "Автоматически") {
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
    portCombo_->addItem("Автоматически");

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
        QMessageBox::information(this, "Устройство", "Устройство не подключено");
        return;
    }

    // Trigger handshake - info will come through signals
    emit statusMessage("Запрос информации об устройстве...");
}

void SettingsPage::onResetSettings() {
    portCombo_->setCurrentIndex(0);
    keepaliveSpin_->setValue(10);
    cbMinimizeToTray_->setChecked(true);
    cbStartMinimized_->setChecked(false);
    cbAutostart_->setChecked(false);
    emit statusMessage("Настройки сброшены");
}

void SettingsPage::onSaveSettings() {
    reed::Config config;
    config.port = selectedPort().toStdString();
    config.keepalive_interval = keepaliveSpin_->value();
    config.brightness = 75;

    reed::ConfigManager::save_config(config);

    // Handle autostart
    if (cbAutostart_->isChecked()) {
        QString serviceDir = QDir::homePath() + "/.config/systemd/user";
        QDir().mkpath(serviceDir);
        // The service file is managed by the user through the CLI
    }

    emit settingsChanged();
    emit statusMessage("Настройки сохранены");
}
