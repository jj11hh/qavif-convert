#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dialogsettings.h"
#include "convertsettings.h"
#include "convertworker.h"
#include <QMainWindow>
#include <QStringList>
#include <QStringListModel>
#include <QThread>
#include <QDirIterator>
#include <QDebug>
#include <QMutex>
#include <QCloseEvent>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void closeEvent(QCloseEvent *) override;

private slots:
    void on_btnSourceSelect_clicked();
    void on_btnDestSelect_clicked();
    void handleResults(const QString &);
    void updateProgress(int);
    void handleSettings(const ConvertSettings&);
    void taskDone();
    void on_actionSettingsP_triggered();
    void on_actionStartJpegToAvif_triggered();
    void on_actionStartAvifToJpeg_triggered();
    void on_actionStop_triggered();

signals:
    void taskAbort();
    void taskStart();

private:
    void startConvert(WorkerAction);
    bool flagWorking = false;
    QThread workerThread;
    ConvertSettings settings = {10, 10, 8, 100, true, true};
    Ui::MainWindow *ui;
    DialogSettings *dialogSettings;
};

#endif // MAINWINDOW_H
