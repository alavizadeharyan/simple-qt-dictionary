#include <QApplication>
#include <QDateTime>
#include "LogWindow.h"
#include "TranslationApp.h"

// Custom message handler
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(context);
    QString logLevel;

    switch (type) {
        case QtDebugMsg:    logLevel = "DEBUG"; break;
        case QtInfoMsg:     logLevel = "INFO"; break;
        case QtWarningMsg:  logLevel = "WARNING"; break;
        case QtCriticalMsg: logLevel = "CRITICAL"; break;
        case QtFatalMsg:    logLevel = "FATAL"; break;
    }

    QString logMessage = QString("[%1] %2: %3")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), logLevel, msg);

    // Append to log window
    LogWindow::instance()->appendLog(logMessage);

    if (type == QtFatalMsg)
        abort(); // Fatal error, abort
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qInstallMessageHandler(customMessageHandler);
    TranslationApp window;
    window.setWindowTitle("Translation Search");
    window.resize(400, 300);
    window.show();

    return app.exec();
}
