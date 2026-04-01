#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <cstdint>

struct CpuMetrics {
    double temperature = 0.0;
    double usagePercent = 0.0;
    double frequencyMHz = 0.0;
    int coreCount = 0;
};

struct GpuMetrics {
    QString name;
    double temperature = 0.0;
    double usagePercent = 0.0;
    double frequencyMHz = 0.0;
    double voltageMV = 0.0;
    int64_t vramUsedMB = 0;
    int64_t vramTotalMB = 0;
};

struct RamMetrics {
    int64_t totalMB = 0;
    int64_t usedMB = 0;
    int64_t availableMB = 0;
    double usagePercent = 0.0;
};

struct NetMetrics {
    double rxSpeedKBs = 0.0;
    double txSpeedKBs = 0.0;
};

struct DiskMetrics {
    int64_t totalGB = 0;
    int64_t usedGB = 0;
    double usagePercent = 0.0;
    double temperature = 0.0;
};

struct SystemMetrics {
    CpuMetrics cpu;
    QVector<GpuMetrics> gpus;
    RamMetrics ram;
    NetMetrics net;
    DiskMetrics disk;
};

class SystemMonitor : public QObject {
    Q_OBJECT
public:
    explicit SystemMonitor(QObject *parent = nullptr);

    SystemMetrics currentMetrics() const { return metrics_; }

public slots:
    void update();

signals:
    void metricsUpdated(const SystemMetrics &metrics);

private:
    double readCpuTemperature();
    double readCpuUsage();
    double readCpuFrequency();
    int readCpuCoreCount();
    QVector<GpuMetrics> readGpuMetrics();
    RamMetrics readRamMetrics();
    NetMetrics readNetMetrics();
    DiskMetrics readDiskMetrics();
    double readDiskTemperature();

    QString readSysFile(const QString &path);
    QString findHwmonByName(const QString &name);

    SystemMetrics metrics_;

    // For CPU usage delta calculation
    int64_t prevCpuIdle_ = 0;
    int64_t prevCpuTotal_ = 0;

    // For network speed delta calculation
    int64_t prevRxBytes_ = 0;
    int64_t prevTxBytes_ = 0;
    int64_t prevNetTimestamp_ = 0;
};
