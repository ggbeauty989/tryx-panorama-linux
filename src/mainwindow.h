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

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupConnections();

    DeviceManager *deviceMgr_;
    Homepage *homepage_;
    PanoramaPage *panoramaPage_;
    SettingsPage *settingsPage_;
    TrayManager *trayMgr_;

    QStackedWidget *stack_;
    QListWidget *navList_;
    QLabel *statusLabel_;

    bool minimizeToTray_ = true;
};
