#include "devicemanager.h"
#include <QDir>
#include <QFileInfo>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>

// --- DeviceWorker ---

DeviceWorker::DeviceWorker(QObject *parent)
    : QObject(parent) {}

DeviceWorker::~DeviceWorker() {
    if (device_ && device_->is_connected()) {
        device_->disconnect();
    }
}

void DeviceWorker::connectDevice(const QString &port) {
    std::string portStr;

    if (port.isEmpty()) {
        std::cerr << "[device] Auto-detecting TRYX device...\n";
        auto detected = panorama::Device::find_device();
        if (!detected) {
            // Disambiguate "no device" vs "device present but unreadable":
            // the second case is by far the most common first-run failure
            // and the generic message sends users on a wild goose chase.
            QStringList candidates = QDir("/dev").entryList(
                QStringList{"ttyACM*"}, QDir::System);
            if (!candidates.isEmpty()) {
                std::cerr << "[device] Found " << candidates.size()
                          << " ttyACM device(s) but cannot access any.\n";
            }
            for (const QString &name : candidates) {
                QFileInfo fi("/dev/" + name);
                if (fi.exists() && (!fi.isReadable() || !fi.isWritable())) {
                    std::cerr << "[device] PERMISSION DENIED: /dev/" << name.toStdString()
                              << " - Run: sudo usermod -aG dialout $USER\n";
                    emit error(QString(
                        "No permission to open /dev/%1. "
                        "Add yourself to the 'dialout' group "
                        "(sudo usermod -aG dialout $USER) then log out and back in.")
                        .arg(name));
                    return;
                }
            }
            std::cerr << "[device] No TRYX device detected.\n";
            emit error("Device not found. Check the USB connection.");
            return;
        }
        std::cerr << "[device] Found TRYX device at " << detected->c_str() << "\n";
        portStr = *detected;
    } else {
        portStr = port.toStdString();
    }

    device_ = std::make_unique<panorama::Device>(portStr);
    if (!device_->connect()) {
        QFileInfo fi(QString::fromStdString(portStr));
        if (fi.exists() && (!fi.isReadable() || !fi.isWritable())) {
            emit error(QString(
                "No permission to open %1. "
                "Add yourself to the 'dialout' group "
                "(sudo usermod -aG dialout $USER) then log out and back in.")
                .arg(QString::fromStdString(portStr)));
        } else {
            emit error(QString("Failed to connect to %1").arg(QString::fromStdString(portStr)));
        }
        device_.reset();
        return;
    }

    doHandshake();
}

void DeviceWorker::disconnectDevice() {
    if (device_) {
        device_->disconnect();
        device_.reset();
    }
    panorama::Adb::reset_cache();
    emit disconnected();
}

void DeviceWorker::doHandshake() {
    if (!device_ || !device_->is_connected()) {
        emit error("Device not connected");
        return;
    }

    auto info = device_->handshake();
    if (!info) {
        emit error("Handshake failed");
        return;
    }

    emit connected(
        QString::fromStdString(info->product_id),
        QString::fromStdString(info->serial),
        QString::fromStdString(info->firmware),
        QString::fromStdString(info->app_version)
    );
}

void DeviceWorker::setBrightness(int value) {
    if (!device_ || !device_->is_connected()) {
        emit error("Device not connected");
        return;
    }

    auto resp = device_->set_brightness(value);
    if (!resp) {
        emit error("Failed to set brightness");
        return;
    }
    currentBrightness_ = value;
    emit brightnessSet(value);
}

void DeviceWorker::setScreenConfig(const QStringList &media, const QString &ratio,
                                   const QString &screenMode, const QString &playMode,
                                   const QStringList &sysinfoLabels,
                                   const QString &settingsPosition,
                                   const QString &settingsColor,
                                   const QString &settingsAlign,
                                   const QStringList &settingsBadges,
                                   int filterOpacity,
                                   const QString &presetId,
                                   const QStringList &sysinfoLabels2,
                                   const QStringList &settingsBadges2,
                                   bool waterfallMode) {
    if (!device_ || !device_->is_connected()) {
        emit error("Device not connected");
        return;
    }

    panorama::ScreenConfig config;
    if (!presetId.isEmpty()) {
        config.preset_id = presetId.toStdString();
    }
    for (const auto &m : media) {
        config.media.push_back(m.toStdString());
    }
    config.ratio = ratio.toStdString();
    config.screen_mode = screenMode.toStdString();
    config.play_mode = playMode.toStdString();

    for (const auto &label : sysinfoLabels) {
        config.sysinfo_display.push_back(label.toStdString());
    }

    config.settings.position = settingsPosition.toStdString();
    config.settings.color = settingsColor.toStdString();
    config.settings.align = settingsAlign.toStdString();
    config.settings.filter_opacity = filterOpacity;
    for (const auto &badge : settingsBadges) {
        config.settings.badges.push_back(badge.toStdString());
    }

    config.waterfall_mode = waterfallMode;

    // Screen Splitting: populate second set of settings and sysinfo
    if (screenMode == "Screen Splitting") {
        for (const auto &label : sysinfoLabels2) {
            config.sysinfo_display2.push_back(label.toStdString());
        }
        config.settings2.position = settingsPosition.toStdString();
        config.settings2.color = settingsColor.toStdString();
        config.settings2.align = settingsAlign.toStdString();
        config.settings2.filter_opacity = filterOpacity;
        for (const auto &badge : settingsBadges2) {
            config.settings2.badges.push_back(badge.toStdString());
        }
    }

    auto resp = device_->set_screen_config(config);
    if (!resp) {
        emit error("Failed to apply screen configuration");
        return;
    }

    // Send sysinfoDisplay as separate command if metrics are selected
    if (!config.sysinfo_display.empty()) {
        device_->set_sysinfo_display(config);
    }

    // Send config with hardware names for badges
    std::string cpuName = "Unknown CPU";
    std::string gpuName = "Unknown GPU";

    // Read CPU name from /proc/cpuinfo
    {
        std::ifstream cpuFile("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuFile, line)) {
            if (line.find("model name") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos && pos + 2 < line.size()) {
                    cpuName = line.substr(pos + 2);
                }
                break;
            }
        }
    }

    // Read GPU name: try sysfs product_name first, fallback to lspci
    {
        namespace fs = std::filesystem;
        std::string drmPath = "/sys/class/drm";
        if (fs::exists(drmPath)) {
            for (const auto& entry : fs::directory_iterator(drmPath)) {
                std::string name = entry.path().filename().string();
                if (name.find("card") == 0 && name.find('-') == std::string::npos) {
                    std::string productPath = entry.path().string() + "/device/product_name";
                    std::ifstream gpuFile(productPath);
                    if (gpuFile) {
                        std::string readName;
                        std::getline(gpuFile, readName);
                        if (!readName.empty()) {
                            gpuName = readName;
                            break;
                        }
                    }
                }
            }
        }
        // Fallback 1: glxinfo gives clean name like "AMD Radeon RX 7900 XTX"
        if (gpuName == "Unknown GPU") {
            FILE* pipe = popen("glxinfo 2>/dev/null | grep 'OpenGL renderer' | head -1", "r");
            if (pipe) {
                char buf[512];
                if (fgets(buf, sizeof(buf), pipe)) {
                    std::string line(buf);
                    auto pos = line.find(": ");
                    if (pos != std::string::npos) {
                        gpuName = line.substr(pos + 2);
                        // Cut at first '(' - remove "(radeonsi, navi31, ...)"
                        auto paren = gpuName.find('(');
                        if (paren != std::string::npos)
                            gpuName = gpuName.substr(0, paren);
                        while (!gpuName.empty() && (gpuName.back() == '\n' || gpuName.back() == '\r' || gpuName.back() == ' '))
                            gpuName.pop_back();
                    }
                }
                pclose(pipe);
            }
        }
        // Fallback 2: lspci
        if (gpuName == "Unknown GPU" || gpuName.empty()) {
            FILE* pipe = popen("lspci 2>/dev/null | grep -i 'VGA\\|3D controller' | head -1", "r");
            if (pipe) {
                char buf[512];
                if (fgets(buf, sizeof(buf), pipe)) {
                    std::string line(buf);
                    auto pos = line.find(": ");
                    if (pos != std::string::npos) {
                        gpuName = line.substr(pos + 2);
                        while (!gpuName.empty() && (gpuName.back() == '\n' || gpuName.back() == '\r'))
                            gpuName.pop_back();
                    }
                }
                pclose(pipe);
            }

            // lspci returns the verbose form, e.g.
            //   "Advanced Micro Devices, Inc. [AMD/ATI] Navi 48 [Radeon RX 9070/9070 XT/9070 GRE] (rev c0)"
            // The Tryx badge has limited width, so collapse to a short form
            // ("AMD Radeon RX 9070").

            auto rev = gpuName.find(" (rev");
            if (rev != std::string::npos) gpuName.erase(rev);

            struct { const char* prefix; const char* tag; } vendors[] = {
                {"Advanced Micro Devices, Inc.", "AMD"},
                {"NVIDIA Corporation",           "NVIDIA"},
                {"Intel Corporation",            "Intel"},
            };
            std::string vendor;
            for (auto& v : vendors) {
                size_t plen = std::strlen(v.prefix);
                if (gpuName.compare(0, plen, v.prefix) == 0) {
                    vendor = v.tag;
                    gpuName.erase(0, plen);
                    break;
                }
            }
            while (!gpuName.empty() && gpuName.front() == ' ') gpuName.erase(0, 1);

            // Strip an initial vendor-tag bracket like "[AMD/ATI]"
            if (!gpuName.empty() && gpuName.front() == '[') {
                auto close = gpuName.find(']');
                if (close != std::string::npos) {
                    gpuName.erase(0, close + 1);
                    while (!gpuName.empty() && gpuName.front() == ' ') gpuName.erase(0, 1);
                }
            }

            // If a [Model …] bracket remains (after the architecture codename),
            // it carries the marketing name — prefer that
            auto open = gpuName.find('[');
            if (open != std::string::npos) {
                auto close = gpuName.find(']', open);
                if (close != std::string::npos) {
                    gpuName = gpuName.substr(open + 1, close - open - 1);
                }
            }

            // Collapse variant lists ("9070/9070 XT/9070 GRE" -> "9070")
            auto slash = gpuName.find('/');
            if (slash != std::string::npos) {
                gpuName = gpuName.substr(0, slash);
                while (!gpuName.empty() && gpuName.back() == ' ') gpuName.pop_back();
            }

            if (!vendor.empty() && !gpuName.empty()) {
                gpuName = vendor + " " + gpuName;
            }
        }
    }

    fprintf(stderr, "[config] cpu='%s' gpu='%s'\n", cpuName.c_str(), gpuName.c_str());

    // Send full config (KANALI format) - sets everything in one command
    device_->send_full_config(config, cpuName, gpuName, currentBrightness_, "Celsius");

    emit screenConfigSet();
}

void DeviceWorker::sendSysinfo(const QStringList &labels, const QStringList &values,
                               const QStringList &units) {
    if (!device_ || !device_->is_connected()) {
        return;
    }

    std::vector<panorama::SysinfoData> data;
    for (int i = 0; i < labels.size() && i < values.size() && i < units.size(); ++i) {
        panorama::SysinfoData item;
        item.label = labels[i].toStdString();
        item.value = values[i].toStdString();
        item.unit = units[i].toStdString();
        data.push_back(item);
    }

    device_->send_sysinfo(data);
    emit sysinfoSent();
}

void DeviceWorker::sendSysinfoDisplay(const QStringList &labels) {
    if (!device_ || !device_->is_connected()) return;

    panorama::ScreenConfig config;
    for (const auto &l : labels)
        config.sysinfo_display.push_back(l.toStdString());

    device_->set_sysinfo_display(config);
}

void DeviceWorker::deleteMedia(const QStringList &files) {
    std::vector<std::string> filenames;
    for (const auto &f : files) {
        filenames.push_back(f.toStdString());
    }

    if (!device_ || !device_->is_connected()) {
        emit error("Device not connected");
        return;
    }

    auto resp = device_->delete_media(filenames);
    if (!resp) {
        emit error("Failed to delete media files");
        return;
    }

    for (const auto &f : files) {
        panorama::Adb::remove(f.toStdString());
    }

    emit mediaDeleted();
}

void DeviceWorker::uploadMedia(const QString &localPath) {
    if (!panorama::Adb::is_available()) {
        emit error("'adb' is not installed. Install it (e.g. sudo apt install android-tools-adb) and reconnect.");
        return;
    }
    if (!panorama::Adb::is_device_connected()) {
        emit error("ADB device not found. Run 'adb devices' to check the device is listed.");
        return;
    }

    std::string path = localPath.toStdString();
    auto type = panorama::Media::detect_type(path);

    std::string remoteName;
    std::string uploadPath = path;

    if (panorama::Media::needs_conversion(path)) {
        remoteName = panorama::Media::get_converted_name(path);
    } else {
        remoteName = panorama::Media::get_filename(path);
    }

    // Check if file already exists on device - skip upload
    if (panorama::Adb::file_exists(remoteName)) {
        emit mediaUploaded(QString::fromStdString(remoteName));
        return;
    }

    // Need to upload - convert if necessary
    if (panorama::Media::needs_conversion(path)) {
        if (!panorama::Media::is_ffmpeg_available()) {
            emit error("ffmpeg not found. Install: sudo dnf install ffmpeg");
            return;
        }
        emit uploadProgress("Converting to MP4...");
        std::string converted = std::string(panorama::Media::TMP_DIR) + remoteName;
        bool ok = (type == panorama::MediaType::Gif)
            ? panorama::Media::convert_gif_to_mp4(path, converted)
            : panorama::Media::convert_to_mp4(path, converted);
        if (!ok) {
            emit error("Conversion to MP4 failed");
            return;
        }
        uploadPath = converted;
    }

    emit uploadProgress("Uploading to device...");
    if (!panorama::Adb::push(uploadPath, remoteName)) {
        emit error("Upload to device failed");
        return;
    }

    emit mediaUploaded(QString::fromStdString(remoteName));
}

void DeviceWorker::refreshMediaList() {
    if (!panorama::Adb::is_available()) {
        emit error("'adb' is not installed. Install it (e.g. sudo apt install android-tools-adb) and reconnect.");
        return;
    }
    if (!panorama::Adb::is_device_connected()) {
        emit error("ADB device not found. Run 'adb devices' to check the device is listed.");
        return;
    }

    auto files = panorama::Adb::list_media();
    if (!files) {
        emit error("Failed to fetch file list");
        return;
    }

    QStringList list;
    for (const auto &f : *files) {
        if (!f.empty()) {
            list.append(QString::fromStdString(f));
        }
    }
    emit mediaListReady(list);
}

void DeviceWorker::sendKeepalive() {
    if (!device_ || !device_->is_connected()) {
        return;
    }
    device_->handshake();
}

void DeviceWorker::setRotation(int degrees) {
    if (!device_ || !device_->is_connected()) {
        return;
    }
    device_->set_rotation(degrees);
}

void DeviceWorker::rebootDevice() {
    // ADB reboot works on this device; the protocol-level POST reboot does not.
    if (!panorama::Adb::reboot()) {
        emit error("Failed to reboot device via ADB");
    }
}

// --- DeviceManager ---

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent),
      worker_(new DeviceWorker),
      keepaliveTimer_(new QTimer(this)) {

    // Seed the worker's brightness cache from the persisted config so that
    // the first setScreenConfig() doesn't reset the device to a default.
    if (auto cfg = panorama::ConfigManager::load_config()) {
        worker_->seedBrightness(cfg->brightness);
    }

    worker_->moveToThread(&workerThread_);

    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

    // Forward signals from worker
    connect(worker_, &DeviceWorker::connected, this,
            [this](const QString &pid, const QString &serial,
                   const QString &fw, const QString &app) {
                connected_ = true;
                emit deviceConnected(pid, serial, fw, app);
            });
    connect(worker_, &DeviceWorker::disconnected, this,
            [this]() {
                connected_ = false;
                emit deviceDisconnected();
            });
    connect(worker_, &DeviceWorker::error, this, &DeviceManager::deviceError);
    connect(worker_, &DeviceWorker::brightnessSet, this, &DeviceManager::brightnessChanged);
    connect(worker_, &DeviceWorker::screenConfigSet, this, &DeviceManager::screenConfigChanged);
    connect(worker_, &DeviceWorker::mediaUploaded, this, &DeviceManager::mediaUploaded);
    connect(worker_, &DeviceWorker::mediaDeleted, this, &DeviceManager::mediaDeleted);
    connect(worker_, &DeviceWorker::mediaListReady, this, &DeviceManager::mediaListUpdated);
    connect(worker_, &DeviceWorker::uploadProgress, this, &DeviceManager::uploadStatus);

    // Connect internal signals to worker slots
    connect(this, &DeviceManager::requestConnect, worker_, &DeviceWorker::connectDevice);
    connect(this, &DeviceManager::requestDisconnect, worker_, &DeviceWorker::disconnectDevice);
    connect(this, &DeviceManager::requestBrightness, worker_, &DeviceWorker::setBrightness);
    connect(this, &DeviceManager::requestScreenConfig, worker_, &DeviceWorker::setScreenConfig);
    connect(this, &DeviceManager::requestDeleteMedia, worker_, &DeviceWorker::deleteMedia);
    connect(this, &DeviceManager::requestUploadMedia, worker_, &DeviceWorker::uploadMedia);
    connect(this, &DeviceManager::requestRefreshMedia, worker_, &DeviceWorker::refreshMediaList);
    connect(this, &DeviceManager::requestKeepalive, worker_, &DeviceWorker::sendKeepalive);
    connect(this, &DeviceManager::requestRotation, worker_, &DeviceWorker::setRotation);
    connect(this, &DeviceManager::requestReboot, worker_, &DeviceWorker::rebootDevice);
    connect(this, &DeviceManager::requestSysinfo, worker_, &DeviceWorker::sendSysinfo);
    connect(this, &DeviceManager::requestSysinfoDisplay, worker_, &DeviceWorker::sendSysinfoDisplay);
    connect(worker_, &DeviceWorker::sysinfoSent, this, &DeviceManager::sysinfoSent);

    // Keepalive timer
    connect(keepaliveTimer_, &QTimer::timeout, this,
            [this]() { emit requestKeepalive(); });

    workerThread_.start();
}

DeviceManager::~DeviceManager() {
    stopKeepalive();
    emit requestDisconnect();
    workerThread_.quit();
    workerThread_.wait();
}

void DeviceManager::connectDevice(const QString &port) {
    emit requestConnect(port);
}

void DeviceManager::disconnectDevice() {
    stopKeepalive();
    emit requestDisconnect();
}

void DeviceManager::setBrightness(int value) {
    emit requestBrightness(qBound(0, value, 100));
}

void DeviceManager::setScreenConfig(const QStringList &media, const QString &ratio,
                                    const QString &screenMode, const QString &playMode,
                                    const QStringList &sysinfoLabels,
                                    const QString &settingsPosition,
                                    const QString &settingsColor,
                                    const QString &settingsAlign,
                                    const QStringList &settingsBadges,
                                    int filterOpacity,
                                    const QString &presetId,
                                    const QStringList &sysinfoLabels2,
                                    const QStringList &settingsBadges2,
                                    bool waterfallMode) {
    emit requestScreenConfig(media, ratio, screenMode, playMode,
                             sysinfoLabels, settingsPosition, settingsColor,
                             settingsAlign, settingsBadges, filterOpacity,
                             presetId, sysinfoLabels2, settingsBadges2,
                             waterfallMode);
}

void DeviceManager::sendSysinfo(const QStringList &labels, const QStringList &values,
                                const QStringList &units) {
    emit requestSysinfo(labels, values, units);
}

void DeviceManager::sendSysinfoDisplay(const QStringList &labels) {
    emit requestSysinfoDisplay(labels);
}

void DeviceManager::setRotation(int degrees) {
    emit requestRotation(degrees);
}

void DeviceManager::rebootDevice() {
    emit requestReboot();
}

void DeviceManager::deleteMedia(const QStringList &files) {
    emit requestDeleteMedia(files);
}

void DeviceManager::uploadMedia(const QString &localPath) {
    emit requestUploadMedia(localPath);
}

void DeviceManager::refreshMediaList() {
    emit requestRefreshMedia();
}

void DeviceManager::startKeepalive(int intervalSec) {
    keepaliveTimer_->start(intervalSec * 1000);
}

void DeviceManager::stopKeepalive() {
    keepaliveTimer_->stop();
}
