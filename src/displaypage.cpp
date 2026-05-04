#include "displaypage.h"

#include <QVBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QMouseEvent>

static const int TILE_WIDTH = 200;
static const int TILE_IMG_HEIGHT = 100;

MediaTile::MediaTile(const MediaEntry &entry, QWidget *parent)
    : QFrame(parent), entry_(entry) {
    setFixedSize(TILE_WIDTH, TILE_IMG_HEIGHT + 50);
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    imageLabel_ = new QLabel;
    imageLabel_->setFixedSize(TILE_WIDTH - 8, TILE_IMG_HEIGHT);
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setStyleSheet("background: #1a1a1a; border-radius: 4px;");
    imageLabel_->setText("...");
    layout->addWidget(imageLabel_);

    nameLabel_ = new QLabel(entry_.fileName);
    nameLabel_->setAlignment(Qt::AlignCenter);
    nameLabel_->setWordWrap(false);
    QFont nameFont = nameLabel_->font();
    nameFont.setPointSize(8);
    nameFont.setBold(true);
    nameLabel_->setFont(nameFont);
    nameLabel_->setMaximumWidth(TILE_WIDTH - 8);
    layout->addWidget(nameLabel_);

    double sizeMB = entry_.sizeBytes / (1024.0 * 1024.0);
    infoLabel_ = new QLabel(QString("%1 MB  %2").arg(sizeMB, 0, 'f', 1).arg(entry_.format));
    infoLabel_->setAlignment(Qt::AlignCenter);
    QFont infoFont = infoLabel_->font();
    infoFont.setPointSize(7);
    infoLabel_->setFont(infoFont);
    infoLabel_->setStyleSheet("color: #888;");
    layout->addWidget(infoLabel_);

    updateStyle();
}

void MediaTile::setSelected(bool sel) {
    selected_ = sel;
    updateStyle();
}

void MediaTile::setThumbnail(const QPixmap &pix) {
    thumb_ = pix;
    if (!pix.isNull()) {
        imageLabel_->setPixmap(pix.scaled(imageLabel_->size(),
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation));
        imageLabel_->setText({});
    }
}

void MediaTile::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(this);
    }
    QFrame::mousePressEvent(event);
}

void MediaTile::updateStyle() {
    if (selected_) {
        setStyleSheet(
            "MediaTile {"
            "  border: 2px solid #4CAF50;"
            "  border-radius: 6px;"
            "  background: rgba(76, 175, 80, 40);"
            "}");
    } else {
        setStyleSheet(
            "MediaTile {"
            "  border: 2px solid transparent;"
            "  border-radius: 6px;"
            "  background: #2a2a2a;"
            "}"
            "MediaTile:hover {"
            "  border: 2px solid #555;"
            "  background: #333;"
            "}");
    }
}

QString builtinMediaDir() {
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/../media",
        appDir + "/../../media",
        appDir + "/media",
    };
    for (const auto &path : candidates) {
        QDir dir(path);
        if (dir.exists() && !dir.isEmpty()) {
            return dir.absolutePath();
        }
    }
    return {};
}
