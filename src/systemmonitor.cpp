#include "systemmonitor.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QDirIterator>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QProcess>

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent) {}

void SystemMonitor::update() {
    metrics_.cpu.temperature = readCpuTemperature();
    metrics_.cpu.usagePercent = readCpuUsage();
    metrics_.cpu.frequencyMHz = readCpuFrequency();
    metrics_.cpu.coreCount = readCpuCoreCount();
    metrics_.gpus = readGpuMetrics();
    metrics_.ram = readRamMetrics();
    metrics_.net = readNetMetrics();
    metrics_.disk = readDiskMetrics();

    emit metricsUpdated(metrics_);
}

QString SystemMonitor::readSysFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return file.readAll().trimmed();
}

QString SystemMonitor::findHwmonByName(const QString &name) {
    QDir hwmonDir("/sys/class/hwmon");
    // sysfs entries are symlinks, so QDir::Dirs (which uses lstat) misses
    // them — enumerate every entry and let the per-file checks gate access.
    for (const auto &entry : hwmonDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString namePath = hwmonDir.filePath(entry) + "/name";
        if (readSysFile(namePath) == name) {
            return hwmonDir.filePath(entry);
        }
    }
    return {};
}

double SystemMonitor::readCpuTemperature() {
    // AMD Ryzen first (k10temp / zenpower), then Intel (coretemp).
    // coretemp's temp1_input is the package temperature.
    for (const QString &name : {"k10temp", "zenpower", "coretemp"}) {
        QString hwmon = findHwmonByName(name);
        if (hwmon.isEmpty()) continue;
        QString val = readSysFile(hwmon + "/temp1_input");
        if (!val.isEmpty()) {
            return val.toDouble() / 1000.0;
        }
    }
    return 0.0;
}

double SystemMonitor::readCpuUsage() {
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0.0;
    }

    QString line = file.readLine();
    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 8 || parts[0] != "cpu") {
        return 0.0;
    }

    int64_t user = parts[1].toLongLong();
    int64_t nice = parts[2].toLongLong();
    int64_t system = parts[3].toLongLong();
    int64_t idle = parts[4].toLongLong();
    int64_t iowait = parts[5].toLongLong();
    int64_t irq = parts[6].toLongLong();
    int64_t softirq = parts[7].toLongLong();

    int64_t totalIdle = idle + iowait;
    int64_t total = user + nice + system + idle + iowait + irq + softirq;

    int64_t deltaIdle = totalIdle - prevCpuIdle_;
    int64_t deltaTotal = total - prevCpuTotal_;

    prevCpuIdle_ = totalIdle;
    prevCpuTotal_ = total;

    if (deltaTotal == 0) {
        return 0.0;
    }

    return (1.0 - static_cast<double>(deltaIdle) / deltaTotal) * 100.0;
}

double SystemMonitor::readCpuFrequency() {
    // Average frequency across all cores
    QDir cpuDir("/sys/devices/system/cpu");
    double totalFreq = 0.0;
    int count = 0;

    for (const auto &entry : cpuDir.entryList(QStringList{"cpu[0-9]*"},
                                                QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString freqPath = cpuDir.filePath(entry) + "/cpufreq/scaling_cur_freq";
        QString val = readSysFile(freqPath);
        if (!val.isEmpty()) {
            totalFreq += val.toDouble() / 1000.0; // kHz -> MHz
            count++;
        }
    }

    return count > 0 ? totalFreq / count : 0.0;
}

int SystemMonitor::readCpuCoreCount() {
    QDir cpuDir("/sys/devices/system/cpu");
    int count = cpuDir.entryList(QStringList{"cpu[0-9]*"},
                                  QDir::AllEntries | QDir::NoDotAndDotDot).size();
    return count > 0 ? count : 1;
}

static void appendNvidiaGpus(QVector<GpuMetrics> &out) {
    // nvidia-smi is the only stable user-space source for NVIDIA metrics on
    // Linux. Skip silently if the binary isn't installed (open-source nouveau
    // users won't have it, and the AMD path already covers them via sysfs).
    QProcess proc;
    proc.start("nvidia-smi", {
        "--query-gpu=name,temperature.gpu,utilization.gpu,clocks.gr,memory.used,memory.total",
        "--format=csv,noheader,nounits"
    });
    if (!proc.waitForStarted(500)) return;
    if (!proc.waitForFinished(1000)) {
        proc.kill();
        return;
    }
    if (proc.exitCode() != 0) return;

    const QString output = QString::fromUtf8(proc.readAllStandardOutput());
    for (const QString &rawLine : output.split('\n', Qt::SkipEmptyParts)) {
        const QStringList parts = rawLine.split(',');
        if (parts.size() < 6) continue;

        GpuMetrics gpu;
        gpu.name = parts[0].trimmed();
        gpu.temperature = parts[1].trimmed().toDouble();
        gpu.usagePercent = parts[2].trimmed().toDouble();
        gpu.frequencyMHz = parts[3].trimmed().toDouble();
        gpu.vramUsedMB = parts[4].trimmed().toLongLong();
        gpu.vramTotalMB = parts[5].trimmed().toLongLong();
        gpu.voltageMV = 0.0;  // not exposed by nvidia-smi in this query
        out.append(gpu);
    }
}

QVector<GpuMetrics> SystemMonitor::readGpuMetrics() {
    QVector<GpuMetrics> gpus;

    QDir drmDir("/sys/class/drm");
    for (const auto &entry : drmDir.entryList(QStringList{"card[0-9]*"},
                                                QDir::AllEntries | QDir::NoDotAndDotDot)) {
        // Skip render nodes (card0-* etc)
        if (entry.contains('-')) {
            continue;
        }

        QString cardPath = drmDir.filePath(entry) + "/device";

        // Check if it's an AMD GPU
        QString busyPath = cardPath + "/gpu_busy_percent";
        if (!QFile::exists(busyPath)) {
            continue;
        }

        GpuMetrics gpu;

        // GPU name from marketing name or product
        QString productPath = cardPath + "/product_name";
        gpu.name = readSysFile(productPath);
        if (gpu.name.isEmpty()) {
            gpu.name = "GPU " + entry.mid(4);
        }

        // GPU usage
        gpu.usagePercent = readSysFile(busyPath).toDouble();

        // Temperature via hwmon
        QDir hwmonDir(cardPath + "/hwmon");
        for (const auto &hwEntry : hwmonDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
            QString tempPath = hwmonDir.filePath(hwEntry) + "/temp1_input";
            QString val = readSysFile(tempPath);
            if (!val.isEmpty()) {
                gpu.temperature = val.toDouble() / 1000.0;
                break;
            }
        }

        // GPU frequency from pp_dpm_sclk (active line marked with *)
        QString sclkPath = cardPath + "/pp_dpm_sclk";
        QString sclkData = readSysFile(sclkPath);
        if (!sclkData.isEmpty()) {
            for (const auto &line : sclkData.split('\n')) {
                if (line.contains('*')) {
                    QRegularExpression re("(\\d+)Mhz");
                    auto match = re.match(line);
                    if (match.hasMatch()) {
                        gpu.frequencyMHz = match.captured(1).toDouble();
                    }
                    break;
                }
            }
        }

        // GPU voltage from hwmon in0_input (millivolts)
        QDir hwmonDirVolt(cardPath + "/hwmon");
        for (const auto &hwEntry : hwmonDirVolt.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
            QString voltPath = hwmonDirVolt.filePath(hwEntry) + "/in0_input";
            QString voltVal = readSysFile(voltPath);
            if (!voltVal.isEmpty()) {
                gpu.voltageMV = voltVal.toDouble();
                break;
            }
        }

        // VRAM
        QString vramUsed = readSysFile(cardPath + "/mem_info_vram_used");
        QString vramTotal = readSysFile(cardPath + "/mem_info_vram_total");
        if (!vramUsed.isEmpty()) {
            gpu.vramUsedMB = vramUsed.toLongLong() / (1024 * 1024);
        }
        if (!vramTotal.isEmpty()) {
            gpu.vramTotalMB = vramTotal.toLongLong() / (1024 * 1024);
        }

        gpus.append(gpu);
    }

    appendNvidiaGpus(gpus);

    return gpus;
}

RamMetrics SystemMonitor::readRamMetrics() {
    RamMetrics ram;

    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ram;
    }

    QMap<QString, int64_t> info;
    QTextStream in(&file);
    QString line;
    while (!(line = in.readLine()).isNull()) {
        QStringList parts = line.split(QRegularExpression("[:\\s]+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            info[parts[0]] = parts[1].toLongLong();
        }
    }

    ram.totalMB = info.value("MemTotal", 0) / 1024;
    ram.availableMB = info.value("MemAvailable", 0) / 1024;
    ram.usedMB = ram.totalMB - ram.availableMB;
    ram.usagePercent = ram.totalMB > 0
                           ? static_cast<double>(ram.usedMB) / ram.totalMB * 100.0
                           : 0.0;

    return ram;
}

NetMetrics SystemMonitor::readNetMetrics() {
    NetMetrics net;

    QFile file("/proc/net/dev");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return net;
    }

    int64_t totalRx = 0;
    int64_t totalTx = 0;

    QTextStream in(&file);
    QString line;
    while (!(line = in.readLine()).isNull()) {
        line = line.trimmed();
        if (!line.contains(':')) {
            continue;
        }

        QString iface = line.section(':', 0, 0).trimmed();
        if (iface == "lo") {
            continue;
        }

        QStringList parts = line.section(':', 1).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 9) {
            totalRx += parts[0].toLongLong();
            totalTx += parts[8].toLongLong();
        }
    }

    int64_t now = QDateTime::currentMSecsSinceEpoch();

    if (prevNetTimestamp_ > 0) {
        double dtSec = (now - prevNetTimestamp_) / 1000.0;
        if (dtSec > 0) {
            net.rxSpeedKBs = (totalRx - prevRxBytes_) / 1024.0 / dtSec;
            net.txSpeedKBs = (totalTx - prevTxBytes_) / 1024.0 / dtSec;
        }
    }

    prevRxBytes_ = totalRx;
    prevTxBytes_ = totalTx;
    prevNetTimestamp_ = now;

    return net;
}

DiskMetrics SystemMonitor::readDiskMetrics() {
    DiskMetrics disk;
    QStorageInfo storage = QStorageInfo::root();
    if (storage.isValid()) {
        disk.totalGB = storage.bytesTotal() / (1024LL * 1024 * 1024);
        int64_t freeGB = storage.bytesAvailable() / (1024LL * 1024 * 1024);
        disk.usedGB = disk.totalGB - freeGB;
        disk.usagePercent = disk.totalGB > 0
                                ? static_cast<double>(disk.usedGB) / disk.totalGB * 100.0
                                : 0.0;
    }
    disk.temperature = readDiskTemperature();
    return disk;
}

double SystemMonitor::readDiskTemperature() {
    double maxTemp = 0;
    QDir hwmonDir("/sys/class/hwmon");
    // sysfs entries are symlinks, so QDir::Dirs (which uses lstat) misses
    // them — enumerate every entry and let the per-file checks gate access.
    for (const auto &entry : hwmonDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString namePath = hwmonDir.filePath(entry) + "/name";
        if (readSysFile(namePath) == "nvme") {
            QString tempPath = hwmonDir.filePath(entry) + "/temp1_input";
            QString val = readSysFile(tempPath);
            if (!val.isEmpty()) {
                double temp = val.toDouble() / 1000.0;
                if (temp > maxTemp) maxTemp = temp;
            }
        }
    }
    return maxTemp;
}
