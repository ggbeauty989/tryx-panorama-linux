#pragma once

#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QString>

struct MediaEntry {
    QString filePath;
    QString fileName;
    QString format;
    qint64 sizeBytes;
};

class MediaTile : public QFrame {
    Q_OBJECT
public:
    explicit MediaTile(const MediaEntry &entry, QWidget *parent = nullptr);

    QString filePath() const { return entry_.filePath; }
    bool isSelected() const { return selected_; }
    void setSelected(bool sel);
    void setThumbnail(const QPixmap &pix);
    QPixmap thumbnail() const { return thumb_; }

signals:
    void clicked(MediaTile *tile);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateStyle();

    MediaEntry entry_;
    QPixmap thumb_;
    bool selected_ = false;
    QLabel *imageLabel_;
    QLabel *nameLabel_;
    QLabel *infoLabel_;
};

// Locate the built-in media directory (../media relative to the binary).
QString builtinMediaDir();
