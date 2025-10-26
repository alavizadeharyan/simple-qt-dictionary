#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QMutex>

class LogWindow : public QWidget {
    Q_OBJECT

public:
    static LogWindow* instance();
    void appendLog(const QString &message);

private:
    LogWindow(QWidget *parent = nullptr);
    QPlainTextEdit *logViewer;
    static LogWindow *m_instance;
    static QMutex mutex;
};

#endif // LOGWINDOW_H
