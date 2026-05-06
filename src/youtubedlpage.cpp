#include "youtubedlpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QApplication>
#include <QCoreApplication>
#include <QScrollBar>
#include <QRegularExpression>

YoutubeDlPage::YoutubeDlPage(QWidget *parent)
    : QWidget(parent), process_(nullptr) {
    setupUi();
    checkYtDlp();
    onRefreshFileList();
}

void YoutubeDlPage::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // ---- Download section ----
    auto *downloadGroup = new QGroupBox("Download from YouTube");
    auto *dlLayout = new QGridLayout(downloadGroup);
    dlLayout->setSpacing(8);

    // URL input
    auto *urlLabel = new QLabel("URL:");
    urlLabel->setStyleSheet("color: #aaa; font-size: 13px;");
    urlInput_ = new QLineEdit;
    urlInput_->setPlaceholderText("https://www.youtube.com/watch?v=...");
    urlInput_->setStyleSheet(
        "QLineEdit {"
        "  background: #2a2a3e; color: #fff; border: 1px solid #3a3a4e;"
        "  padding: 8px 12px; border-radius: 6px; font-size: 13px;"
        "}"
        "QLineEdit:focus { border-color: #DEF750; }");
    dlLayout->addWidget(urlLabel, 0, 0);
    dlLayout->addWidget(urlInput_, 0, 1);

    // Quality selector
    auto *qualLabel = new QLabel("Quality:");
    qualLabel->setStyleSheet("color: #aaa; font-size: 13px;");
    qualityCombo_ = new QComboBox;
    qualityCombo_->addItems({"Best", "1080p", "720p", "480p", "360p", "Audio only (MP3)"});
    qualityCombo_->setStyleSheet(
        "QComboBox {"
        "  background: #2a2a3e; color: #fff; border: 1px solid #3a3a4e;"
        "  padding: 8px 12px; border-radius: 6px; font-size: 13px; min-width: 160px;"
        "}"
        "QComboBox:focus { border-color: #DEF750; }"
        "QComboBox::drop-down { border: none; width: 30px; }"
        "QComboBox QAbstractItemView {"
        "  background: #2a2a3e; color: #fff; selection-background-color: #3a3a5e;"
        "  border: 1px solid #3a3a4e;"
        "}");
    dlLayout->addWidget(qualLabel, 1, 0);
    dlLayout->addWidget(qualityCombo_, 1, 1);

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    downloadBtn_ = new QPushButton("Download");
    downloadBtn_->setStyleSheet(
        "QPushButton {"
        "  background: #DEF750; color: #1a1a2e; border: none;"
        "  padding: 10px 24px; border-radius: 8px; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #c9e040; }"
        "QPushButton:pressed { background: #b0c830; }"
        "QPushButton:disabled { background: #3a3a4e; color: #666; }");
    cancelBtn_ = new QPushButton("Cancel");
    cancelBtn_->setStyleSheet(
        "QPushButton {"
        "  background: #3a3a4e; color: #fff; border: none;"
        "  padding: 10px 24px; border-radius: 8px; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #4a4a5e; }"
        "QPushButton:pressed { background: #555; }"
        "QPushButton:disabled { background: #2a2a3e; color: #555; }");
    cancelBtn_->setEnabled(false);

    btnLayout->addStretch();
    btnLayout->addWidget(downloadBtn_);
    btnLayout->addWidget(cancelBtn_);

    dlLayout->addLayout(btnLayout, 2, 0, 1, 2);

    mainLayout->addWidget(downloadGroup);

    // ---- Progress and log ----
    auto *progressGroup = new QGroupBox("Progress");
    auto *progLayout = new QVBoxLayout(progressGroup);
    progLayout->setSpacing(8);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("color: #aaa; font-size: 12px;");

    progressBar_ = new QProgressBar;
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setStyleSheet(
        "QProgressBar {"
        "  background: #2a2a3e; border: 1px solid #3a3a4e; border-radius: 6px;"
        "  height: 22px; text-align: center; color: #fff; font-size: 12px;"
        "}"
        "QProgressBar::chunk {"
        "  background: #DEF750; border-radius: 5px;"
        "}");

    logOutput_ = new QTextEdit;
    logOutput_->setReadOnly(true);
    logOutput_->setMaximumHeight(140);
    logOutput_->setStyleSheet(
        "QTextEdit {"
        "  background: #0d0d1a; color: #0f0; border: 1px solid #3a3a4e;"
        "  padding: 8px; border-radius: 6px; font-family: monospace; font-size: 11px;"
        "}");

    progLayout->addWidget(statusLabel_);
    progLayout->addWidget(progressBar_);
    progLayout->addWidget(logOutput_);

    mainLayout->addWidget(progressGroup);

    // ---- Downloaded files ----
    auto *filesGroup = new QGroupBox("Downloaded Files");
    auto *filesLayout = new QVBoxLayout(filesGroup);
    filesLayout->setSpacing(8);

    fileList_ = new QListWidget;
    fileList_->setStyleSheet(
        "QListWidget {"
        "  background: #2a2a3e; color: #fff; border: 1px solid #3a3a4e;"
        "  border-radius: 6px; font-size: 13px; padding: 4px;"
        "}"
        "QListWidget::item { padding: 6px 10px; border-radius: 4px; }"
        "QListWidget::item:hover { background: #3a3a5e; }"
        "QListWidget::item:selected { background: #2d2d4a; color: #DEF750; }");
    fileList_->setAlternatingRowColors(false);
    fileList_->setMinimumHeight(100);

    auto *fileBtnLayout = new QHBoxLayout;
    fileBtnLayout->setSpacing(8);

    auto *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton {"
        "  background: #2a2a3e; color: #aaa; border: 1px solid #3a3a4e;"
        "  padding: 8px 16px; border-radius: 6px; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #3a3a4e; color: #fff; }");

    openBtn_ = new QPushButton("Open File");
    openBtn_->setStyleSheet(
        "QPushButton {"
        "  background: #6c5ce7; color: #fff; border: none;"
        "  padding: 8px 16px; border-radius: 6px; font-size: 12px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #5b4bd5; }");

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet(
        "QPushButton {"
        "  background: #3a3a4e; color: #e74c3c; border: 1px solid #e74c3c;"
        "  padding: 8px 16px; border-radius: 6px; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #e74c3c; color: #fff; }");

    fileBtnLayout->addWidget(refreshBtn);
    fileBtnLayout->addStretch();
    fileBtnLayout->addWidget(openBtn_);
    fileBtnLayout->addWidget(deleteBtn_);

    filesLayout->addWidget(fileList_);
    filesLayout->addLayout(fileBtnLayout);

    mainLayout->addWidget(filesGroup, 1);
    mainLayout->addStretch();

    // Connect signals
    connect(downloadBtn_, &QPushButton::clicked, this, &YoutubeDlPage::onDownload);
    connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
        if (process_ && process_->state() == QProcess::Running) {
            process_->kill();
            statusLabel_->setText("Download cancelled");
            emit statusMessage("Download cancelled");
            downloadBtn_->setEnabled(true);
            cancelBtn_->setEnabled(false);
            progressBar_->setValue(0);
        }
    });
    connect(refreshBtn, &QPushButton::clicked, this, &YoutubeDlPage::onRefreshFileList);
    connect(openBtn_, &QPushButton::clicked, this, &YoutubeDlPage::onOpenFile);
    connect(deleteBtn_, &QPushButton::clicked, this, &YoutubeDlPage::onDeleteFile);
}

void YoutubeDlPage::checkYtDlp() {
    QProcess test;
    test.start("yt-dlp", {"--version"});
    test.waitForFinished(3000);
    ytDlpAvailable_ = (test.exitCode() == 0);

    if (!ytDlpAvailable_) {
        logOutput_->setHtml(
            "<span style='color:#e74c3c;'><b>yt-dlp not found.</b><br>"
            "Install it with:<br>"
            "<code>sudo apt install yt-dlp</code><br>"
            "or<br>"
            "<code>pip install yt-dlp</code></span>");
        downloadBtn_->setEnabled(false);
        statusLabel_->setText("yt-dlp not installed");
    }
}

QString YoutubeDlPage::outputDir() const {
    // Use the project's media directory, resolved relative to the executable
    // Fallback: media/ directory next to the executable, or ~/TRYX_Panorama_Downloads
    QDir exeDir(QCoreApplication::applicationDirPath());
    if (exeDir.cdUp() && exeDir.cd("media")) {
        return exeDir.absolutePath();
    }
    // Fallback: user home directory
    QString fallback = QDir::homePath() + "/TRYX_Panorama_Downloads";
    QDir().mkpath(fallback);
    return fallback;
}

QString YoutubeDlPage::formatString() const {
    switch (qualityCombo_->currentIndex()) {
        case 1:  return "bestvideo[height<=1080]+bestaudio/best[height<=1080]";
        case 2:  return "bestvideo[height<=720]+bestaudio/best[height<=720]";
        case 3:  return "bestvideo[height<=480]+bestaudio/best[height<=480]";
        case 4:  return "bestvideo[height<=360]+bestaudio/best[height<=360]";
        case 5:  return {};  // Audio only: handled separately
        case 0:
        default: return "bestvideo+bestaudio/best";
    }
}

void YoutubeDlPage::onDownload() {
    QString url = urlInput_->text().trimmed();
    if (url.isEmpty()) {
        emit statusMessage("Please enter a YouTube URL");
        return;
    }

    if (!ytDlpAvailable_) {
        QMessageBox::warning(this, "yt-dlp Missing",
                             "yt-dlp is not installed.\n\n"
                             "Install it with:\n"
                             "sudo apt install yt-dlp\n\n"
                             "or: pip install yt-dlp");
        return;
    }

    // Clear previous log
    logOutput_->clear();
    progressBar_->setValue(0);
    statusLabel_->setText("Starting download...");
    emit statusMessage("Download started");

    downloadBtn_->setEnabled(false);
    cancelBtn_->setEnabled(true);

    QStringList args;
    args << "--newline"
         << "--progress"
         << "--no-playlist"
         << "--js-runtimes" << "node"
         << "--remote-components" << "ejs:github"
         << "-o" << outputDir() + "/%(title)s.%(ext)s";

    // Quality / format selection
    if (qualityCombo_->currentIndex() == 5) {
        // Audio only
        args << "-x" << "--audio-format" << "mp3";
    } else {
        args << "-f" << formatString();
    }

    args << url;

    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);

    connect(process_, &QProcess::readyRead, this, &YoutubeDlPage::onProcessReadyRead);
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &YoutubeDlPage::onProcessFinished);
    connect(process_, &QProcess::errorOccurred, this, &YoutubeDlPage::onProcessError);

    process_->start("yt-dlp", args);
}

void YoutubeDlPage::onProcessReadyRead() {
    if (!process_) return;

    QString output = QString::fromUtf8(process_->readAll());
    logOutput_->append(output.trimmed());

    // Auto-scroll to bottom
    QScrollBar *sb = logOutput_->verticalScrollBar();
    sb->setValue(sb->maximum());

    // Try to parse progress percentage from yt-dlp output
    // yt-dlp outputs lines like: [download]  42.0% of ~50.00MiB at 2.5MiB/s ETA 00:12
    static QRegularExpression progressRe(R"((\d+\.?\d*)%)");
    QRegularExpressionMatch match = progressRe.match(output);
    if (match.hasMatch()) {
        bool ok;
        int pct = match.captured(1).toInt(&ok);
        if (ok) {
            progressBar_->setValue(pct);
            statusLabel_->setText(QString("Downloading... %1%").arg(pct));
        }
    } else if (output.contains("[ExtractAudio]") || output.contains("[Merger]")) {
        statusLabel_->setText("Processing...");
    } else if (output.contains("[download] Destination:")) {
        // Extract filename from destination line
        QString destLine = output.section("[download] Destination:", 1).trimmed();
        if (!destLine.isEmpty()) {
            lastDownloadedFile_ = destLine;
        }
    }
}

void YoutubeDlPage::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    if (exitCode == 0 && status == QProcess::NormalExit) {
        progressBar_->setValue(100);
        statusLabel_->setText("Download completed!");
        emit statusMessage("Download completed successfully");

        if (!lastDownloadedFile_.isEmpty() && QFile::exists(lastDownloadedFile_)) {
            logOutput_->append(QString("\n✔ Saved: %1").arg(lastDownloadedFile_));
        }
    } else {
        statusLabel_->setText("Download failed");
        emit statusMessage("Download failed");
        logOutput_->append("\n✘ Download failed. Check the URL and try again.");
    }

    downloadBtn_->setEnabled(ytDlpAvailable_);
    cancelBtn_->setEnabled(false);
    onRefreshFileList();

    process_->deleteLater();
    process_ = nullptr;
}

void YoutubeDlPage::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
    statusLabel_->setText("Process error: " + (process_ ? process_->errorString() : "unknown"));
    emit statusMessage("Download error occurred");

    downloadBtn_->setEnabled(ytDlpAvailable_);
    cancelBtn_->setEnabled(false);

    if (process_) {
        process_->deleteLater();
        process_ = nullptr;
    }
}

void YoutubeDlPage::onRefreshFileList() {
    fileList_->clear();
    QDir dir(outputDir());
    QStringList filters;
    filters << "*.mp4" << "*.webm" << "*.mkv" << "*.mp3" << "*.m4a" << "*.avi" << "*.mov";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    for (const QFileInfo &fi : files) {
        QString sizeStr;
        qint64 size = fi.size();
        if (size < 1024) {
            sizeStr = QString("%1 B").arg(size);
        } else if (size < 1024 * 1024) {
            sizeStr = QString("%1 KB").arg(size / 1024);
        } else {
            sizeStr = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
        }
        fileList_->addItem(QString("%1  [%2]").arg(fi.fileName(), sizeStr));
        fileList_->item(fileList_->count() - 1)->setData(Qt::UserRole, fi.absoluteFilePath());
    }

    if (fileList_->count() == 0) {
        fileList_->addItem("No downloaded files yet");
        fileList_->item(0)->setFlags(fileList_->item(0)->flags() & ~Qt::ItemIsSelectable);
    }
}

void YoutubeDlPage::onOpenFile() {
    QListWidgetItem *item = fileList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString().isEmpty()) return;

    QString filePath = item->data(Qt::UserRole).toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

void YoutubeDlPage::onDeleteFile() {
    QListWidgetItem *item = fileList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString().isEmpty()) return;

    QString filePath = item->data(Qt::UserRole).toString();
    QString fileName = QFileInfo(filePath).fileName();

    int ret = QMessageBox::question(this, "Delete File",
                                    QString("Delete \"%1\"?").arg(fileName),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (QFile::remove(filePath)) {
            emit statusMessage(QString("Deleted: %1").arg(fileName));
        } else {
            emit statusMessage("Error: Could not delete file");
        }
        onRefreshFileList();
    }
}