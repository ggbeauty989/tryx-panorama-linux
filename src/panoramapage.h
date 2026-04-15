#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QColor>
#include <QSlider>
#include <QListWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QFrame>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTabBar>
#include <QMap>
#include <QSettings>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>

#include <QRadioButton>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QAudioOutput>
#include "systemmonitor.h"
#include "splitconfig.h"

class DeviceManager;

struct MediaEntry;
class MediaTile;

class PanoramaPage : public QWidget {
    Q_OBJECT
public:
    explicit PanoramaPage(DeviceManager *deviceMgr, QWidget *parent = nullptr);

    bool isMetricsRunning() const { return metricsRunning_; }

signals:
    void statusMessage(const QString &msg);
    void metricsRunningChanged(bool running);

public slots:
    void startMetrics();
    void stopMetrics();

private slots:
    // Tab switching
    void onTabChanged(int index);

    // Pre-set tab
    void onMetricToggled();
    void onChooseTextColor();
    void onPresetSave();
    void onTileClicked(MediaTile *tile);

    // Customization tab
    void onUploadClicked();
    void onUploadBuiltinClicked();
    void onSetDisplayClicked();
    void onDeleteClicked();
    void onRefreshClicked();
    void onMediaListUpdated(const QStringList &files);
    void onMediaUploaded(const QString &filename);
    void onMediaDeleted();
    void onUploadStatus(const QString &status);
    void onScreenModeChanged();
    void onCustomSave();
    void onFileListContextMenu(const QPoint &pos);

    // Display settings
    void onBrightnessChanged(int value);

    // Metrics sending
    void onSendMetrics();

private:
    void setupUi();
    void setupPresetTab(QWidget *parent);
    void setupCustomizationTab(QWidget *parent);
    void setupDisplaySettings();
    void loadBuiltinMedia();
    QPixmap extractThumbnail(const QString &videoPath, const QString &cachePath);
    void applyScreenConfig();
    void savePageState();
    void restorePageState();

    void rebuildPresetGrid();
    int calculateGridColumns() const;

    static QString builtinMediaDir();
    static QString presetIdForName(const QString &name);

    DeviceManager *deviceMgr_;
    SystemMonitor *monitor_;
    QTimer *metricsTimer_;
    bool metricsRunning_ = false;

    // Tab bar
    QTabBar *tabBar_;
    QStackedWidget *tabStack_;

    // Pre-set tab - built-in media carousel
    QScrollArea *presetScrollArea_;
    QWidget *presetGridWidget_;
    QGridLayout *presetGrid_;
    QList<MediaTile *> presetTiles_;
    MediaTile *selectedPresetTile_ = nullptr;

    // Preview
    QLabel *previewLabel_ = nullptr;
    QMediaPlayer *previewPlayer_ = nullptr;
    QVideoSink *previewSink_ = nullptr;

    // Pre-set tab - sysinfo display
    struct MetricOption {
        QCheckBox *checkbox;
        QString label;
        QString unit;
    };
    QList<MetricOption> metricOptions_;
    QLabel *selectionCountLabel_;

    // Display settings controls
    QComboBox *positionCombo_;
    QComboBox *alignCombo_;
    QPushButton *textColorBtn_;
    QColor textColor_ = QColor("#FFFFFF");
    QCheckBox *cbCpuBadge_;
    QCheckBox *cbGpuBadge_;

    // Metrics status
    QLabel *metricsStatusLabel_;

    // Customization tab - file management
    QListWidget *fileList_;
    QComboBox *ratioCombo_;
    QComboBox *screenModeCombo_;
    QComboBox *playModeCombo_;
    QPushButton *uploadBtn_;
    QPushButton *setDisplayBtn_;
    QPushButton *deleteBtn_;
    QPushButton *refreshBtn_;
    QLabel *dropZone_;
    QProgressBar *progressBar_;

    // Customization tab - Screen Splitting
    QRadioButton *fullScreenRadio_;
    QRadioButton *splitScreenRadio_;
    QWidget *fullScreenControls_;
    SplitConfigWidget *splitConfigWidget_;
    QPushButton *customSaveBtn_;
    QToolButton *customMetricsBtn_;
    QMenu *customMetricsMenu_;
    QList<QCheckBox *> customMetricCheckboxes_;

    // Customization tab - user media grid
    QScrollArea *customScrollArea_;
    QWidget *customGridWidget_;
    QGridLayout *customGrid_;
    QList<MediaTile *> customTiles_;

    // Display settings panel
    QSlider *brightnessSlider_;
    QLabel *brightnessLabel_;
    QCheckBox *cbSleepMode_;
    QCheckBox *cbMirrorMode_;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
