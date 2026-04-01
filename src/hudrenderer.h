#pragma once

#include <QObject>
#include <QImage>
#include <QFont>
#include <QColor>
#include <QString>
#include <QProcess>

#include "systemmonitor.h"

struct HudConfig {
    bool showCpuTemp = true;
    bool showCpuUsage = true;
    bool showCpuFreq = true;
    bool showGpuTemp = true;
    bool showGpuUsage = true;
    bool showGpuVram = true;
    bool showRam = true;
    bool showNet = true;

    QFont font = QFont("Monospace", 28);
    QColor textColor = QColor(255, 255, 255);
    QColor shadowColor = QColor(0, 0, 0, 180);

    enum Position { Top, Center, Bottom };
    enum Alignment { Left, HCenter, Right };
    Position position = Top;
    Alignment alignment = Left;

    int paddingX = 40;
    int paddingY = 40;
    int lineSpacing = 8;

    QString sourceVideoPath;
};

class HudRenderer : public QObject {
    Q_OBJECT
public:
    static constexpr int DISPLAY_WIDTH = 2240;
    static constexpr int DISPLAY_HEIGHT = 1080;

    explicit HudRenderer(QObject *parent = nullptr);

    void setConfig(const HudConfig &config);
    HudConfig config() const { return config_; }

    // Render transparent overlay PNG (for ffmpeg compositing)
    QImage renderOverlay(const SystemMetrics &metrics);

    // Render preview with video frame background + overlay
    QImage renderPreview(const SystemMetrics &metrics, int previewWidth, int previewHeight);

    bool saveOverlay(const QImage &image, const QString &path);

    // ffmpeg: composite overlay onto source video -> output video
    bool compositeVideo(const QString &sourceVideo, const QString &overlayPng,
                        const QString &outputVideo);

    // Extract single frame from video for preview
    QImage extractVideoFrame(const QString &videoPath);

signals:
    void compositeFinished(bool success, const QString &outputPath);
    void compositeError(const QString &message);

private:
    QStringList buildLines(const SystemMetrics &metrics);
    void drawText(QPainter &painter, const QStringList &lines, const QRect &rect);

    HudConfig config_;
};
