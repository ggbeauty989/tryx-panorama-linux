#pragma once

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QCheckBox>
#include <QPixmap>
#include <QStringList>

class SplitConfigWidget : public QWidget {
    Q_OBJECT
public:
    explicit SplitConfigWidget(QWidget *parent = nullptr);

    QStringList leftMedia() const;
    QStringList rightMedia() const;
    QStringList leftMetrics() const;
    QStringList rightMetrics() const;
    QString playMode() const;
    int rotation() const;

    void assignToLeft(const QString &filename, const QPixmap &thumb);
    void assignToRight(const QString &filename, const QPixmap &thumb);

private:
    void setupUi();
    void rebuildMetricsButtonCb(QToolButton *btn, const QList<QCheckBox *> &checkboxes, const QString &side);

    // Preview frames
    QLabel *leftPreview_;
    QLabel *rightPreview_;
    QLabel *leftFileLabel_;
    QLabel *rightFileLabel_;

    // Settings
    QComboBox *playModeCombo_;
    QComboBox *rotationCombo_;
    QToolButton *leftMetricsBtn_;
    QToolButton *rightMetricsBtn_;
    QMenu *leftMetricsMenu_;
    QMenu *rightMetricsMenu_;
    QList<QCheckBox *> leftMetricCheckboxes_;
    QList<QCheckBox *> rightMetricCheckboxes_;

    // Media assignments
    QString leftFilename_;
    QString rightFilename_;
};
