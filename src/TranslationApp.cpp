#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringListModel>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include "LogWindow.h"
#include "TranslationApp.h"

const QRegularExpression TranslationApp::separatorRegex("[,،]");

TranslationApp::TranslationApp(QWidget *parent)
    : QWidget(parent),
    searchBar(new QLineEdit(this)),
    resultList(new QListView(this)),
    translationBox(new QTextEdit(this)),
    listModel(new QStringListModel(this)),
    msgBox(new QMessageBox(this)),
    progressBar(new QProgressBar(this)),
    menuBar(new QMenuBar(this)),
    idx(-1) {

    QVBoxLayout *layout = new QVBoxLayout;
    QMenu *menu = menuBar->addMenu("Menu");

    QWidgetAction *updateAction = new QWidgetAction(menu);
    QLabel *updateLabel = new QLabel("Update Database..");
    updateLabel->setContentsMargins(5, 5, 5, 0);
    updateAction->setDefaultWidget(updateLabel);
    menu->addAction(updateAction);

    QWidgetAction *logsAction = new QWidgetAction(menu);
    QLabel *logsLabel = new QLabel("Logs");
    logsLabel->setContentsMargins(5, 5, 5, 5);
    logsAction->setDefaultWidget(logsLabel);
    menu->addAction(logsAction);

    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    layout->setMenuBar(menuBar);
    layout->addWidget(searchBar);
    layout->addWidget(resultList);
    layout->addWidget(translationBox);
    layout->addWidget(progressBar);
    setLayout(layout);

    msgBox.setWindowTitle("Malformatted Lines");
    msgBox.addButton(QMessageBox::Ok);

    resultList->setModel(listModel);
    translationBox->setReadOnly(true);

    connect(searchBar, &QLineEdit::textChanged, this, &TranslationApp::searchDatabase);
    connect(resultList->selectionModel(), &QItemSelectionModel::currentChanged, this, &TranslationApp::onItemSelected);
    connect(updateAction, &QAction::triggered, this, &TranslationApp::openFileDialog);
    connect(logsAction, &QAction::triggered, this, &TranslationApp::showLogs);

    initDatabase();
}

TranslationApp::~TranslationApp() {
    if (db.isOpen()) {
        db.close();
    }
}

void TranslationApp::initDatabase() {
    // Get directory where the executable is located
    QString exeDir = QCoreApplication::applicationDirPath();

    // Create full path to config.json
    QString configPath = QDir(exeDir).filePath("config.json");

    // Check if the file exists
    if (QFile::exists(configPath)) {
        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Couldn't open config file:" << configPath;
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << parseError.errorString();
            return;
        }

        if (!doc.isObject()) {
            qWarning() << "JSON is not an object.";
            return;
        }

        QJsonObject obj = doc.object();

        // Get key-values
        host = obj.value("host").toString();
        user = obj.value("user").toString();
        database = obj.value("database").toString();
        table = obj.value("table").toString();

        qDebug() << "host:" << host;
        qDebug() << "user:" << user;
        qDebug() << "database:" << database;
        qDebug() << "table:" << table;

        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName(host);
        db.setDatabaseName(database);
        db.setUserName(user);

        if (!db.open()) {
            qCritical() << "Database error: " << db.lastError().text();
            return;
        }
    } else {
        qDebug() << "config.json not found in:" << exeDir;
    }
}

void TranslationApp::searchDatabase(const QString &query) {
    QStringList wordList;
    QString sql = QString("SELECT word FROM %1 WHERE word LIKE :ex").arg(table);
    QSqlQuery qry;
    idx = -1;

    qry.prepare(sql);
    qry.bindValue(":ex", "%" + query + "%");

    if (!qry.exec()) {
        qCritical() << "Query failed: " << qry.lastError().text();
        return;
    }

    while (qry.next()) {
        wordList.append(qry.value(0).toString());
    }

    listModel->setStringList(wordList);
}

void TranslationApp::onItemSelected(const QModelIndex &index) {
    QString selectedWord = listModel->data(index).toString();
    QString sql = QString("SELECT translations FROM %1 WHERE word = :ex").arg(table);
    QSqlQuery qry;
    idx = index.row();

    qry.prepare(sql);
    qry.bindValue(":ex", selectedWord);

    if (!qry.exec()) {
        qCritical() << "Query failed: " << qry.lastError().text();
        return;
    }

    if (qry.next()) {
        QString translations = qry.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(translations.toUtf8());

        if (doc.isArray()) {
            QJsonArray jsonArray = doc.array();
            QString formattedTranslations;
            for (auto it = jsonArray.begin(); it != jsonArray.end(); ++it) {
                QJsonValue value = *it;
                if (value.isString()) {
                    formattedTranslations += value.toString() + "\n";
                }
            }
            translationBox->setText(formattedTranslations);
        }
    } else {
        QMessageBox::warning(nullptr, "Error", "The 'translations' data is not a valid JSON array.");
    }
}

void TranslationApp::updateDatabase(const QString &inputPath) {
    QFile file(inputPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file" << inputPath << "for reading.";
        return;
    }

    QTextStream textStream(&file);
    QStringList allText = textStream.readAll().split('\n');
    int numberOfLines = allText.count();
    QStringList malformattedLinesList;
    int counter = 1;

    for (auto it = allText.begin(); it != allText.end(); ++it) {
        QString line = *it;
        progressBar->setValue(counter*100/numberOfLines);
        QStringList parts = line.split(":");

        if (parts.count() != 2) {
            malformattedLinesList.append(QString::number(counter));
        } else {
            QString word = parts[0].trimmed();

            QString insertSql = QString("INSERT IGNORE INTO %1 (word, translations) VALUES (:wd, :em)").arg(table);
            QSqlQuery insertQry;
            insertQry.prepare(insertSql);
            insertQry.bindValue(":wd", word);
            insertQry.bindValue(":em", "[]");

            if (!insertQry.exec()) {
                qCritical() << "Insert query failed: " << insertQry.lastError().text();
                return;
            }

            QString translations = parts[1];
            QStringList translationsList = translations.split(separatorRegex, Qt::SkipEmptyParts);

            QString retrieveSql = QString("SELECT translations FROM %1 WHERE word = :wd").arg(table);
            QSqlQuery retrieveQry;
            retrieveQry.prepare(retrieveSql);
            retrieveQry.bindValue(":wd", word);

            if (!retrieveQry.exec()) {
                qCritical() << "Retrieve query failed: " << retrieveQry.lastError().text();
                return;
            }

            if (!retrieveQry.next()) {
                qCritical() << QString("The word %1 could not be retrieved from the database.").arg(word);
                return;
            }

            QString dbTranslations = retrieveQry.value(0).toString();
            QJsonDocument dbTranslationsJson = QJsonDocument::fromJson(dbTranslations.toUtf8());
            QJsonArray dbTranslationsArray = dbTranslationsJson.array();
            QStringList dbTranslationsList;
            for (auto it = dbTranslationsArray.begin(); it != dbTranslationsArray.end(); ++it) {
                QJsonValue dbTranslationValue = *it;
                dbTranslationsList.append(dbTranslationValue.toString());
            }

            for (QString &translation : translationsList) {
                translation = translation.trimmed();
                bool found = false;
                for (const QString &dbTranslation : dbTranslationsList) {
                    if (translation == dbTranslation) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    dbTranslationsList.append(translation);
                }
            }

            for (QString &dbTranslation : dbTranslationsList) {
                dbTranslation = "\"" + dbTranslation + "\"";
            }

            QString updateSql = QString("UPDATE %1 SET translations = :tr WHERE word = :wd").arg(table);
            QSqlQuery updateQry;
            updateQry.prepare(updateSql);
            updateQry.bindValue(":tr", "[" + dbTranslationsList.join(", ") + "]");
            updateQry.bindValue(":wd", word);

            if (!updateQry.exec()) {
                qCritical() << "Update query failed: " << updateQry.lastError().text();
                return;
            }
        }

        counter++;
    }

    if (malformattedLinesList.count() > 0) {
        QString malformattedLines = malformattedLinesList.join(", ");

        // Create a message box for showing the malformatted lines
        msgBox.setText(malformattedLines);
        msgBox.exec();
    }

    file.close();
}

void TranslationApp::openFileDialog() {
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Select a File");

    if (!filePath.isEmpty()) {
        updateDatabase(filePath);
    }
}

void TranslationApp::showLogs() {
    LogWindow::instance()->show();
    LogWindow::instance()->raise();
    LogWindow::instance()->activateWindow();
}
