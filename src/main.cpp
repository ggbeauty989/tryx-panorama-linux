#include <QApplication>
#include <QLoggingCategory>
#include <QPalette>
#include <QStyleFactory>
#include <cstdlib>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // Suppress GStreamer device enumeration spam
    setenv("GST_DEBUG", "0", 0);
    setenv("PIPEWIRE_LOG_LEVEL", "0", 0);

    QLoggingCategory::setFilterRules(
        "qt.multimedia.*=false\n"
        "qt.core.qfuture.*=false\n");

    QApplication app(argc, argv);
    app.setApplicationName("TRYX Panorama Manager");
    app.setOrganizationName("DXVSI");

    // Force a dark palette so unstyled widgets (Settings group boxes, message
    // dialogs, file picker, etc.) don't render with the system light theme,
    // which leaves our hard-coded #aaa labels unreadable.
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(0x1a, 0x1a, 0x2e));
    dark.setColor(QPalette::WindowText,      Qt::white);
    dark.setColor(QPalette::Base,            QColor(0x2a, 0x2a, 0x3e));
    dark.setColor(QPalette::AlternateBase,   QColor(0x23, 0x23, 0x35));
    dark.setColor(QPalette::Text,            Qt::white);
    dark.setColor(QPalette::Button,          QColor(0x3a, 0x3a, 0x4e));
    dark.setColor(QPalette::ButtonText,      Qt::white);
    dark.setColor(QPalette::BrightText,      Qt::red);
    dark.setColor(QPalette::Highlight,       QColor(0x6c, 0x5c, 0xe7));
    dark.setColor(QPalette::HighlightedText, Qt::white);
    dark.setColor(QPalette::ToolTipBase,     QColor(0x2a, 0x2a, 0x3e));
    dark.setColor(QPalette::ToolTipText,     Qt::white);
    dark.setColor(QPalette::PlaceholderText, QColor(0xaa, 0xaa, 0xaa));
    dark.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x80, 0x80, 0x80));
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x80, 0x80, 0x80));
    app.setPalette(dark);

    MainWindow window;
    if (window.shouldStartMinimized()) {
        // Surface a balloon so the user doesn't assume the GUI failed to start.
        window.notifyStartedInTray();
    } else {
        window.show();
    }

    return app.exec();
}
