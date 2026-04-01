#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QProgressBar>
#include <QFrame>
#include <QVBoxLayout>

#include "systemmonitor.h"

class Homepage : public QWidget {
    Q_OBJECT
public:
    explicit Homepage(QWidget *parent = nullptr);

private slots:
    void onMetricsUpdated(const SystemMetrics &metrics);

private:
    void setupUi();
    QFrame *createCard(const QString &title, QLayout *contentLayout);

    SystemMonitor *monitor_;
    QTimer *updateTimer_;

    // CPU card
    QLabel *cpuUsageLabel_;
    QLabel *cpuTempLabel_;
    QProgressBar *cpuBar_;

    // GPU card
    QLabel *gpuUsageLabel_;
    QLabel *gpuTempLabel_;
    QProgressBar *gpuBar_;

    // Memory card
    QLabel *memUsageLabel_;
    QLabel *memDetailLabel_;
    QProgressBar *memBar_;

    // Disk card
    QLabel *diskUsageLabel_;
    QLabel *diskDetailLabel_;
    QProgressBar *diskBar_;

    // Network card
    QLabel *netDownloadLabel_;
    QLabel *netUploadLabel_;
};
