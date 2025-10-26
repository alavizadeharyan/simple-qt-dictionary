#include <QVBoxLayout>
#include "LogWindow.h"

LogWindow *LogWindow::m_instance = nullptr;
QMutex LogWindow::mutex;

LogWindow::LogWindow(QWidget *parent)
    : QWidget(parent) {
    setWindowTitle("Application Logs");
    resize(600, 400);
    logViewer = new QPlainTextEdit(this);
    logViewer->setReadOnly(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(logViewer);
    setLayout(layout);
}

LogWindow* LogWindow::instance() {
    QMutexLocker locker(&mutex);
    if (!m_instance)
        m_instance = new LogWindow();
    return m_instance;
}

void LogWindow::appendLog(const QString &message) {
    logViewer->appendPlainText(message);
}

