#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>

class DeviceManager;
class Homepage;
class PanoramaPage;
class SettingsPage;
class TrayManager;
class YoutubeDlPage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool shouldStartMinimized() const;
    void notifyStartedInTray();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupConnections();

    DeviceManager *deviceMgr_;
    Homepage *homepage_;
    PanoramaPage *panoramaPage_;
    SettingsPage *settingsPage_;
    YoutubeDlPage *youtubeDlPage_;
    TrayManager *trayMgr_;

    QStackedWidget *stack_;
    QListWidget *navList_;
    QLabel *statusLabel_;

    bool minimizeToTray_ = true;
};
