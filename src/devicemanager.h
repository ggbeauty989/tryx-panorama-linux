#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QThread>
#include <memory>

#include <panorama/device.hpp>
#include <panorama/adb.hpp>
#include <panorama/config.hpp>
#include <panorama/media.hpp>

class DeviceWorker : public QObject {
    Q_OBJECT
public:
    explicit DeviceWorker(QObject *parent = nullptr);
    ~DeviceWorker();

public slots:
    void connectDevice(const QString &port);
    void disconnectDevice();
    void doHandshake();
    void setBrightness(int value);
    void setScreenConfig(const QStringList &media, const QString &ratio,
                         const QString &screenMode, const QString &playMode,
                         const QStringList &sysinfoLabels,
                         const QString &settingsPosition,
                         const QString &settingsColor,
                         const QString &settingsAlign,
                         const QStringList &settingsBadges,
                         int filterOpacity,
                         const QString &presetId = QString(),
                         const QStringList &sysinfoLabels2 = {},
                         const QStringList &settingsBadges2 = {},
                         bool waterfallMode = false);
    void setRotation(int degrees);
    void rebootDevice();
    void deleteMedia(const QStringList &files);
    void uploadMedia(const QString &localPath);
    void refreshMediaList();
    void sendKeepalive();
    void sendSysinfo(const QStringList &labels, const QStringList &values,
                     const QStringList &units);

signals:
    void connected(const QString &productId, const QString &serial,
                   const QString &firmware, const QString &appVersion);
    void disconnected();
    void error(const QString &message);
    void brightnessSet(int value);
    void screenConfigSet();
    void sysinfoSent();
    void mediaUploaded(const QString &filename);
    void mediaDeleted();
    void mediaListReady(const QStringList &files);
    void uploadProgress(const QString &status);

private:
    std::unique_ptr<panorama::Device> device_;
};

class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    bool isConnected() const { return connected_; }

public slots:
    void connectDevice(const QString &port = QString());
    void disconnectDevice();
    void setBrightness(int value);
    void setScreenConfig(const QStringList &media, const QString &ratio = "2:1",
                         const QString &screenMode = "Full Screen",
                         const QString &playMode = "Single",
                         const QStringList &sysinfoLabels = {},
                         const QString &settingsPosition = "Top",
                         const QString &settingsColor = "#FFFFFF",
                         const QString &settingsAlign = "Left",
                         const QStringList &settingsBadges = {},
                         int filterOpacity = 0,
                         const QString &presetId = QString(),
                         const QStringList &sysinfoLabels2 = {},
                         const QStringList &settingsBadges2 = {},
                         bool waterfallMode = false);
    void setRotation(int degrees);
    void rebootDevice();
    void deleteMedia(const QStringList &files);
    void uploadMedia(const QString &localPath);
    void refreshMediaList();
    void sendSysinfo(const QStringList &labels, const QStringList &values,
                     const QStringList &units);
    void startKeepalive(int intervalSec = 10);
    void stopKeepalive();

signals:
    void deviceConnected(const QString &productId, const QString &serial,
                         const QString &firmware, const QString &appVersion);
    void deviceDisconnected();
    void deviceError(const QString &message);
    void brightnessChanged(int value);
    void screenConfigChanged();
    void sysinfoSent();
    void mediaUploaded(const QString &filename);
    void mediaDeleted();
    void mediaListUpdated(const QStringList &files);
    void uploadStatus(const QString &status);

    // Internal signals to worker
    void requestConnect(const QString &port);
    void requestDisconnect();
    void requestBrightness(int value);
    void requestScreenConfig(const QStringList &media, const QString &ratio,
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
                             bool waterfallMode);
    void requestRotation(int degrees);
    void requestReboot();
    void requestDeleteMedia(const QStringList &files);
    void requestUploadMedia(const QString &localPath);
    void requestRefreshMedia();
    void requestKeepalive();
    void requestSysinfo(const QStringList &labels, const QStringList &values,
                        const QStringList &units);

private:
    QThread workerThread_;
    DeviceWorker *worker_;
    QTimer *keepaliveTimer_;
    bool connected_ = false;
};
