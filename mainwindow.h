#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qdir.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    QList<QString> getAllLinesFromFile(QFile &propsFile);
    
private slots:
    void on_pushButton_clicked();
    
private:
    Ui::MainWindow *ui;
    
    int32_t fileContainsText(QList<QString> &lines, QString toFind);
    void insertMultipleLines(QList<QString> &lines, int32_t k, QStringList toAdd);
    
    int32_t processLibFolder(QDir &directory);
    int32_t processLangFolder(QDir &directory);
    int32_t processProgramFolder(QDir &directory);
    void writeLinesToFile(QList<QString> lines, QFile &langFile);
    int32_t processPropertiesFile(QDir &directory);
};
#endif // MAINWINDOW_H
