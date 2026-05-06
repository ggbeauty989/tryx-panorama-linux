#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QProcess>
#include <QLabel>
#include <QProgressBar>

class YoutubeDlPage : public QWidget {
    Q_OBJECT
public:
    explicit YoutubeDlPage(QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &msg);

private slots:
    void onDownload();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onRefreshFileList();
    void onOpenFile();
    void onDeleteFile();

private:
    void setupUi();
    void checkYtDlp();
    QString outputDir() const;
    QString formatString() const;

    QLineEdit *urlInput_;
    QComboBox *qualityCombo_;
    QPushButton *downloadBtn_;
    QPushButton *cancelBtn_;
    QTextEdit *logOutput_;
    QProgressBar *progressBar_;
    QLabel *statusLabel_;
    QListWidget *fileList_;
    QPushButton *openBtn_;
    QPushButton *deleteBtn_;

    QProcess *process_;
    QString lastDownloadedFile_;
    bool ytDlpAvailable_ = false;
};