#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QList>

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject *parent = nullptr);

    void show();
    void hide();
    void showNotification(const QString &title, const QString &message);

public slots:
    void setConnected(bool connected);
    void setMetricsRunning(bool running);
    void setBrightnessValue(int value);

signals:
    void showWindowRequested();
    void hideWindowRequested();
    void quitRequested();
    void brightnessChangeRequested(int value);
    void metricsToggleRequested();

private:
    void setupTray();
    void updateTooltip();

    QSystemTrayIcon *trayIcon_;
    QMenu *trayMenu_;
    QAction *showHideAction_;
    QAction *metricsAction_;
    QMenu *brightnessMenu_;
    QList<QAction *> brightnessActions_;

    bool connected_ = false;
    bool metricsRunning_ = false;
    int brightness_ = 75;
    bool windowVisible_ = true;
};
