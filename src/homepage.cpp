#include "homepage.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QScrollArea>

static const QString CARD_STYLE =
    "QFrame {"
    "  background: #1e1e2e;"
    "  border: 1px solid #333;"
    "  border-radius: 10px;"
    "  padding: 16px;"
    "}";

static const QString BAR_STYLE =
    "QProgressBar {"
    "  border: none;"
    "  border-radius: 4px;"
    "  background: #2a2a3a;"
    "  height: 8px;"
    "  text-align: center;"
    "}"
    "QProgressBar::chunk {"
    "  border-radius: 4px;"
    "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "    stop:0 #6c5ce7, stop:1 #a29bfe);"
    "}";

Homepage::Homepage(QWidget *parent)
    : QWidget(parent) {
    monitor_ = new SystemMonitor(this);
    updateTimer_ = new QTimer(this);

    setupUi();

    connect(updateTimer_, &QTimer::timeout, monitor_, &SystemMonitor::update);
    connect(monitor_, &SystemMonitor::metricsUpdated, this, &Homepage::onMetricsUpdated);

    updateTimer_->start(2000);
    monitor_->update();
}

QFrame *Homepage::createCard(const QString &title, QLayout *contentLayout) {
    auto *card = new QFrame;
    card->setStyleSheet(CARD_STYLE);
    card->setMinimumSize(240, 140);

    auto *layout = new QVBoxLayout(card);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel(title);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #888; border: none; padding: 0;");
    layout->addWidget(titleLabel);

    layout->addLayout(contentLayout);

    return card;
}

void Homepage::setupUi() {
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    auto *scrollWidget = new QWidget;
    auto *mainLayout = new QVBoxLayout(scrollWidget);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    auto *titleLabel = new QLabel("PANORAMA");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #fff;");
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("System Monitoring Dashboard");
    subtitleLabel->setStyleSheet("color: #666; font-size: 12px; margin-bottom: 8px;");
    mainLayout->addWidget(subtitleLabel);

    // Cards grid
    auto *grid = new QGridLayout;
    grid->setSpacing(12);

    // --- CPU Card ---
    {
        auto *content = new QVBoxLayout;
        content->setSpacing(6);

        cpuUsageLabel_ = new QLabel("0%");
        QFont bigFont = cpuUsageLabel_->font();
        bigFont.setPointSize(28);
        bigFont.setBold(true);
        cpuUsageLabel_->setFont(bigFont);
        cpuUsageLabel_->setStyleSheet("color: #fff; border: none; padding: 0;");
        content->addWidget(cpuUsageLabel_);

        cpuBar_ = new QProgressBar;
        cpuBar_->setRange(0, 100);
        cpuBar_->setValue(0);
        cpuBar_->setTextVisible(false);
        cpuBar_->setFixedHeight(8);
        cpuBar_->setStyleSheet(BAR_STYLE);
        content->addWidget(cpuBar_);

        cpuTempLabel_ = new QLabel("Temperature: 0 C");
        cpuTempLabel_->setStyleSheet("color: #aaa; border: none; padding: 0;");
        content->addWidget(cpuTempLabel_);

        grid->addWidget(createCard("CPU Load", content), 0, 0);
    }

    // --- GPU Card ---
    {
        auto *content = new QVBoxLayout;
        content->setSpacing(6);

        gpuUsageLabel_ = new QLabel("0%");
        QFont bigFont = gpuUsageLabel_->font();
        bigFont.setPointSize(28);
        bigFont.setBold(true);
        gpuUsageLabel_->setFont(bigFont);
        gpuUsageLabel_->setStyleSheet("color: #fff; border: none; padding: 0;");
        content->addWidget(gpuUsageLabel_);

        gpuBar_ = new QProgressBar;
        gpuBar_->setRange(0, 100);
        gpuBar_->setValue(0);
        gpuBar_->setTextVisible(false);
        gpuBar_->setFixedHeight(8);
        gpuBar_->setStyleSheet(BAR_STYLE);
        content->addWidget(gpuBar_);

        gpuTempLabel_ = new QLabel("Temperature: 0 C");
        gpuTempLabel_->setStyleSheet("color: #aaa; border: none; padding: 0;");
        content->addWidget(gpuTempLabel_);

        grid->addWidget(createCard("GPU Load", content), 0, 1);
    }

    // --- Memory Card ---
    {
        auto *content = new QVBoxLayout;
        content->setSpacing(6);

        memUsageLabel_ = new QLabel("0%");
        QFont bigFont = memUsageLabel_->font();
        bigFont.setPointSize(28);
        bigFont.setBold(true);
        memUsageLabel_->setFont(bigFont);
        memUsageLabel_->setStyleSheet("color: #fff; border: none; padding: 0;");
        content->addWidget(memUsageLabel_);

        memBar_ = new QProgressBar;
        memBar_->setRange(0, 100);
        memBar_->setValue(0);
        memBar_->setTextVisible(false);
        memBar_->setFixedHeight(8);
        memBar_->setStyleSheet(BAR_STYLE);
        content->addWidget(memBar_);

        memDetailLabel_ = new QLabel("0 GB / 0 GB");
        memDetailLabel_->setStyleSheet("color: #aaa; border: none; padding: 0;");
        content->addWidget(memDetailLabel_);

        grid->addWidget(createCard("Memory Load", content), 1, 0);
    }

    // --- Disk Card ---
    {
        auto *content = new QVBoxLayout;
        content->setSpacing(6);

        diskUsageLabel_ = new QLabel("0%");
        QFont bigFont = diskUsageLabel_->font();
        bigFont.setPointSize(28);
        bigFont.setBold(true);
        diskUsageLabel_->setFont(bigFont);
        diskUsageLabel_->setStyleSheet("color: #fff; border: none; padding: 0;");
        content->addWidget(diskUsageLabel_);

        diskBar_ = new QProgressBar;
        diskBar_->setRange(0, 100);
        diskBar_->setValue(0);
        diskBar_->setTextVisible(false);
        diskBar_->setFixedHeight(8);
        diskBar_->setStyleSheet(BAR_STYLE);
        content->addWidget(diskBar_);

        diskDetailLabel_ = new QLabel("0 GB / 0 GB");
        diskDetailLabel_->setStyleSheet("color: #aaa; border: none; padding: 0;");
        content->addWidget(diskDetailLabel_);

        grid->addWidget(createCard("Hard Disk Load", content), 1, 1);
    }

    // --- Network Card (spans full width) ---
    {
        auto *content = new QHBoxLayout;
        content->setSpacing(24);

        auto *dlLayout = new QVBoxLayout;
        auto *dlTitle = new QLabel("Download");
        dlTitle->setStyleSheet("color: #888; border: none; padding: 0; font-size: 11px;");
        dlLayout->addWidget(dlTitle);
        netDownloadLabel_ = new QLabel("0 KB/s");
        QFont netFont = netDownloadLabel_->font();
        netFont.setPointSize(18);
        netFont.setBold(true);
        netDownloadLabel_->setFont(netFont);
        netDownloadLabel_->setStyleSheet("color: #55efc4; border: none; padding: 0;");
        dlLayout->addWidget(netDownloadLabel_);
        content->addLayout(dlLayout);

        auto *ulLayout = new QVBoxLayout;
        auto *ulTitle = new QLabel("Upload");
        ulTitle->setStyleSheet("color: #888; border: none; padding: 0; font-size: 11px;");
        ulLayout->addWidget(ulTitle);
        netUploadLabel_ = new QLabel("0 KB/s");
        QFont ulFont = netUploadLabel_->font();
        ulFont.setPointSize(18);
        ulFont.setBold(true);
        netUploadLabel_->setFont(ulFont);
        netUploadLabel_->setStyleSheet("color: #74b9ff; border: none; padding: 0;");
        ulLayout->addWidget(netUploadLabel_);
        content->addLayout(ulLayout);

        content->addStretch();

        grid->addWidget(createCard("Network Status", content), 2, 0, 1, 2);
    }

    mainLayout->addLayout(grid);
    mainLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    outerLayout->addWidget(scrollArea);
}

void Homepage::onMetricsUpdated(const SystemMetrics &m) {
    // CPU
    int cpuUsage = static_cast<int>(m.cpu.usagePercent);
    cpuUsageLabel_->setText(QString("%1%").arg(cpuUsage));
    cpuBar_->setValue(cpuUsage);
    cpuTempLabel_->setText(QString("Temperature: %1 C").arg(m.cpu.temperature, 0, 'f', 0));

    // GPU
    if (!m.gpus.isEmpty()) {
        int gpuUsage = static_cast<int>(m.gpus[0].usagePercent);
        gpuUsageLabel_->setText(QString("%1%").arg(gpuUsage));
        gpuBar_->setValue(gpuUsage);
        gpuTempLabel_->setText(QString("Temperature: %1 C").arg(m.gpus[0].temperature, 0, 'f', 0));
    }

    // Memory
    int memUsage = static_cast<int>(m.ram.usagePercent);
    memUsageLabel_->setText(QString("%1%").arg(memUsage));
    memBar_->setValue(memUsage);
    double usedGB = m.ram.usedMB / 1024.0;
    double totalGB = m.ram.totalMB / 1024.0;
    memDetailLabel_->setText(QString("%1 GB / %2 GB").arg(usedGB, 0, 'f', 1).arg(totalGB, 0, 'f', 0));

    // Disk
    int diskUsage = static_cast<int>(m.disk.usagePercent);
    diskUsageLabel_->setText(QString("%1%").arg(diskUsage));
    diskBar_->setValue(diskUsage);
    diskDetailLabel_->setText(QString("%1 GB / %2 GB").arg(m.disk.usedGB).arg(m.disk.totalGB));

    // Network
    auto formatSpeed = [](double kbs) -> QString {
        if (kbs >= 1024.0) {
            return QString("%1 MB/s").arg(kbs / 1024.0, 0, 'f', 1);
        }
        return QString("%1 KB/s").arg(kbs, 0, 'f', 0);
    };
    netDownloadLabel_->setText(formatSpeed(m.net.rxSpeedKBs));
    netUploadLabel_->setText(formatSpeed(m.net.txSpeedKBs));
}
