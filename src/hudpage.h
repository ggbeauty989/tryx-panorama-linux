#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QLineEdit>
#include <QColor>

#include "systemmonitor.h"

class DeviceManager;

class HudPage : public QWidget {
    Q_OBJECT
public:
    explicit HudPage(DeviceManager *deviceMgr, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &msg);
    void hudRunningChanged(bool running);

public slots:
    void startHud();
    void stopHud();
    bool isHudRunning() const { return hudRunning_; }

private slots:
    void onStartStopClicked();
    void onSendMetrics();
    void onMetricToggled();
    void onChooseTextColor();
    void onApplyConfig();

private:
    void setupUi();
    void applyScreenConfig();

    DeviceManager *deviceMgr_;
    SystemMonitor *monitor_;
    QTimer *metricsTimer_;
    bool hudRunning_ = false;

    // Metric checkboxes (max 3 can be selected)
    struct MetricOption {
        QCheckBox *checkbox;
        QString label;     // protocol label: "CPU Temperature" etc.
        QString unit;      // "C", "%", "MHz", "MB" etc.
    };
    QList<MetricOption> metricOptions_;
    QLabel *selectionCountLabel_;

    // Display settings
    QComboBox *positionCombo_;
    QComboBox *alignCombo_;
    QPushButton *textColorBtn_;
    QColor textColor_ = QColor("#FFFFFF");
    QCheckBox *cbCpuBadge_;
    QCheckBox *cbGpuBadge_;
    QSpinBox *intervalSpin_;

    QPushButton *startStopBtn_;
    QPushButton *applyConfigBtn_;
    QLabel *statusLabel_;
};
