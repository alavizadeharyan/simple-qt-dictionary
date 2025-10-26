#ifndef TRANSLATIONAPP_H
#define TRANSLATIONAPP_H

#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringListModel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>


class TranslationApp : public QWidget {
    Q_OBJECT

public:
    explicit TranslationApp(QWidget *parent = nullptr);
    ~TranslationApp();
    void updateDatabase(const QString &);

private slots:
    void onItemSelected(const QModelIndex &index);
    void openFileDialog();
    void showLogs();

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Up) {
            if (idx > 0) {
                idx--;
                QModelIndex selectedIdx = listModel->index(idx, 0);
                QItemSelectionModel *selectionModel = resultList->selectionModel();
                selectionModel->select(selectedIdx, QItemSelectionModel::ClearAndSelect);
                resultList->scrollTo(selectedIdx);
                onItemSelected(selectedIdx);
            }
        }
        else if (event->key() == Qt::Key_Down) {
            if (idx < listModel->rowCount()-1 && listModel->rowCount() > 0) {
                idx++;
                QModelIndex selectedIdx = listModel->index(idx, 0);
                QItemSelectionModel *selectionModel = resultList->selectionModel();
                selectionModel->select(selectedIdx, QItemSelectionModel::ClearAndSelect);
                resultList->scrollTo(selectedIdx);
                onItemSelected(selectedIdx);
            }
        }
        else if (event->key() == Qt::Key_Return) {
            if (resultList->selectionModel()->selectedIndexes().isEmpty() && listModel->rowCount() > 0) {
                idx = 0;
                QModelIndex selectedIdx = listModel->index(idx, 0);
                QItemSelectionModel *selectionModel = resultList->selectionModel();
                selectionModel->select(selectedIdx, QItemSelectionModel::ClearAndSelect);
                resultList->scrollTo(selectedIdx);
                onItemSelected(selectedIdx);
            }
        }
        else {
            QWidget::keyPressEvent(event); // Handle other keys normally
        }
    }

private:
    static const QRegularExpression separatorRegex;
    QLineEdit *searchBar;
    QListView *resultList;
    QTextEdit *translationBox;
    QStringListModel *listModel;
    QSqlDatabase db;
    QMessageBox msgBox;
    QProgressBar *progressBar;
    QMenuBar *menuBar;
    int idx;
    QString host;
    QString user;
    QString database;
    QString table;

    void initDatabase();
    void searchDatabase(const QString &);
};

#endif
