#include "hudrenderer.h"
#include <QPainter>
#include <QFontMetrics>
#include <QProcess>
#include <QDir>
#include <QTemporaryDir>

HudRenderer::HudRenderer(QObject *parent)
    : QObject(parent) {}

void HudRenderer::setConfig(const HudConfig &config) {
    config_ = config;
}

QImage HudRenderer::renderOverlay(const SystemMetrics &metrics) {
    QImage image(DISPLAY_WIDTH, DISPLAY_HEIGHT, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QStringList lines = buildLines(metrics);
    drawText(painter, lines, image.rect());

    return image;
}

QImage HudRenderer::renderPreview(const SystemMetrics &metrics,
                                  int previewWidth, int previewHeight) {
    QImage preview(DISPLAY_WIDTH, DISPLAY_HEIGHT, QImage::Format_ARGB32);

    // Draw video frame as background (or dark fallback)
    if (!config_.sourceVideoPath.isEmpty()) {
        QImage frame = extractVideoFrame(config_.sourceVideoPath);
        if (!frame.isNull()) {
            QPainter bgPainter(&preview);
            bgPainter.drawImage(preview.rect(),
                                frame.scaled(DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                             Qt::IgnoreAspectRatio,
                                             Qt::SmoothTransformation));
        } else {
            preview.fill(QColor(20, 20, 20));
        }
    } else {
        preview.fill(QColor(20, 20, 20));
    }

    // Draw overlay on top
    QImage overlay = renderOverlay(metrics);
    QPainter painter(&preview);
    painter.drawImage(0, 0, overlay);

    return preview.scaled(previewWidth, previewHeight,
                          Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool HudRenderer::saveOverlay(const QImage &image, const QString &path) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    return image.save(path, "PNG");
}

bool HudRenderer::compositeVideo(const QString &sourceVideo,
                                 const QString &overlayPng,
                                 const QString &outputVideo) {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    QStringList args;
    args << "-y"
         << "-i" << sourceVideo
         << "-i" << overlayPng
         << "-filter_complex" << "overlay=0:0:format=auto"
         << "-c:v" << "libx264"
         << "-preset" << "ultrafast"
         << "-crf" << "23"
         << "-pix_fmt" << "yuv420p"
         << "-movflags" << "+faststart"
         << "-an"
         << outputVideo;

    proc.start("ffmpeg", args);
    if (!proc.waitForFinished(60000)) {
        emit compositeError("ffmpeg timeout");
        return false;
    }

    if (proc.exitCode() != 0) {
        emit compositeError(QString::fromUtf8(proc.readAll()));
        return false;
    }

    emit compositeFinished(true, outputVideo);
    return true;
}

QImage HudRenderer::extractVideoFrame(const QString &videoPath) {
    QString tmpFrame = "/tmp/tryx-panorama/preview_frame.png";
    QDir().mkpath("/tmp/tryx-panorama");

    QProcess proc;
    proc.start("ffmpeg", {"-y", "-i", videoPath,
                          "-vf", "select=eq(n\\,0)",
                          "-frames:v", "1",
                          tmpFrame});
    proc.waitForFinished(5000);

    if (proc.exitCode() == 0) {
        return QImage(tmpFrame);
    }
    return {};
}

QStringList HudRenderer::buildLines(const SystemMetrics &metrics) {
    QStringList lines;

    if (config_.showCpuTemp) {
        lines << QString("CPU  %1 C").arg(metrics.cpu.temperature, 0, 'f', 0);
    }
    if (config_.showCpuUsage) {
        lines << QString("CPU  %1%").arg(metrics.cpu.usagePercent, 0, 'f', 1);
    }
    if (config_.showCpuFreq) {
        lines << QString("CPU  %1 MHz").arg(metrics.cpu.frequencyMHz, 0, 'f', 0);
    }

    for (int i = 0; i < metrics.gpus.size(); ++i) {
        const auto &gpu = metrics.gpus[i];
        QString prefix = metrics.gpus.size() > 1
                             ? QString("GPU%1 ").arg(i + 1)
                             : QString("GPU  ");

        if (config_.showGpuTemp) {
            lines << prefix + QString("%1 C").arg(gpu.temperature, 0, 'f', 0);
        }
        if (config_.showGpuUsage) {
            lines << prefix + QString("%1%").arg(gpu.usagePercent, 0, 'f', 0);
        }
        if (config_.showGpuVram) {
            lines << prefix + QString("%1 / %2 MB").arg(gpu.vramUsedMB).arg(gpu.vramTotalMB);
        }
    }

    if (config_.showRam) {
        lines << QString("RAM  %1 / %2 MB (%3%)")
                     .arg(metrics.ram.usedMB)
                     .arg(metrics.ram.totalMB)
                     .arg(metrics.ram.usagePercent, 0, 'f', 1);
    }

    if (config_.showNet) {
        lines << QString("NET  D:%1 KB/s  U:%2 KB/s")
                     .arg(metrics.net.rxSpeedKBs, 0, 'f', 1)
                     .arg(metrics.net.txSpeedKBs, 0, 'f', 1);
    }

    return lines;
}

void HudRenderer::drawText(QPainter &painter, const QStringList &lines,
                           const QRect &rect) {
    painter.setFont(config_.font);
    QFontMetrics fm(config_.font);

    int lineHeight = fm.height() + config_.lineSpacing;
    int totalHeight = lines.size() * lineHeight - config_.lineSpacing;

    int startY = config_.paddingY;
    switch (config_.position) {
    case HudConfig::Top:
        startY = config_.paddingY;
        break;
    case HudConfig::Center:
        startY = (rect.height() - totalHeight) / 2;
        break;
    case HudConfig::Bottom:
        startY = rect.height() - totalHeight - config_.paddingY;
        break;
    }

    for (int i = 0; i < lines.size(); ++i) {
        int y = startY + i * lineHeight + fm.ascent();
        int textWidth = fm.horizontalAdvance(lines[i]);
        int x = config_.paddingX;

        switch (config_.alignment) {
        case HudConfig::Left:
            x = config_.paddingX;
            break;
        case HudConfig::HCenter:
            x = (rect.width() - textWidth) / 2;
            break;
        case HudConfig::Right:
            x = rect.width() - textWidth - config_.paddingX;
            break;
        }

        // Text shadow for readability on video
        painter.setPen(config_.shadowColor);
        painter.drawText(x + 2, y + 2, lines[i]);

        // Main text
        painter.setPen(config_.textColor);
        painter.drawText(x, y, lines[i]);
    }
}
