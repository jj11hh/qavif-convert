#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QStandardPaths>
#include <QDialog>
#include <QIcon>
#include <QStyle>

static const QString getPictureDir(){
    QStringList&& pictures = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (!pictures.empty()){
        return pictures[0];
    }
    else {
        return QDir::homePath();
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(700, 600); // default size

    workerThread.start();
    ui->lESrcLoc->setText(getPictureDir());
    ui->lEDstLoc->setText(getPictureDir());

    auto &&style = qApp->style();
    ui->btnDestSelect->setIcon(style->standardIcon(QStyle::SP_DirOpenIcon));
    ui->btnSourceSelect->setIcon(style->standardIcon(QStyle::SP_DirOpenIcon));

    setWindowIcon(QIcon(":/Icons/Images/icon.png"));
}

MainWindow::~MainWindow()
{
    delete ui;
    workerThread.quit();
    workerThread.wait();
}

void MainWindow::on_btnSourceSelect_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择图像文件夹"), getPictureDir());
    if (!dir.trimmed().isEmpty())
        ui->lESrcLoc->setText(dir);
}


void MainWindow::on_btnDestSelect_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择输出文件夹"), getPictureDir());
    if (!dir.trimmed().isEmpty())
        ui->lEDstLoc->setText(dir);
}

void MainWindow::handleResults(const QString &path){
    ui->listTerm->addItem(path);
}

void MainWindow::updateProgress(int percent){
    qDebug() << "progress: " << QString::number(percent);
    ui->progressBar->setValue(percent);
}

void MainWindow::handleSettings(const ConvertSettings &new_settings){
    settings = new_settings;
}

void MainWindow::on_actionSettingsP_triggered()
{
    dialogSettings = new DialogSettings(this, settings);
    connect(dialogSettings, &DialogSettings::sendSettings,
            this, &MainWindow::handleSettings);

    dialogSettings->setModal(true);
    dialogSettings->show();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if (flagWorking){
        QMessageBox::StandardButton resBtn =
                QMessageBox::question( this, tr("正在图像转换"),
                                        tr("你确定要终止吗?\n"),
                                        QMessageBox::Cancel | QMessageBox::Yes,
                                        QMessageBox::Cancel);
        if (resBtn != QMessageBox::Yes) {
            event->ignore();
        } else {
            ui->listTerm->addItem(tr("正在终止, 请稍候"));
            emit taskAbort();
        }
    }
    else {
        event->accept();
    }
}

void MainWindow::startConvert(WorkerAction _action){
    QString src = ui->lESrcLoc->text();
    QString dst = ui->lEDstLoc->text();
    qDebug() << src;
    auto info = QFileInfo(src);

    if (flagWorking){
        ui->listTerm->addItem(tr("仍在转换中, 无法开始新任务"));
        return;
    }

    if (!info.exists() || !info.isDir() || !info.isReadable()){
        ui->listTerm->addItem(tr("无法打开源文件夹, 文件夹不存在或无权限"));
        return;
    }

    auto infoDst = QFileInfo(dst);
    if (infoDst.exists() && (!infoDst.isDir())){
        ui->listTerm->addItem(tr("无法打开输出文件夹, 不是一个文件夹"));
        return;
    }

    if (!flagWorking) {
        auto worker = new ConvertWorker;
        worker->setAction(_action);
        worker->setPath(src, dst);
        worker->setParameter(settings);
        worker->moveToThread(&workerThread);
        connect(worker, &ConvertWorker::resultReady, this, &MainWindow::handleResults);
        connect(worker, &ConvertWorker::progressReady, this, &MainWindow::updateProgress);
        connect(worker, &ConvertWorker::workDone, this, &MainWindow::taskDone);
        connect(worker, &ConvertWorker::workDone, worker, &ConvertWorker::deleteLater);
        connect(this, &MainWindow::taskAbort, worker, &ConvertWorker::abort);
        connect(this, &MainWindow::taskStart, worker, &ConvertWorker::doWork);
        connect(&workerThread, &QThread::finished, this, &QThread::deleteLater);

        ui->actionStartJpegToAvif->setDisabled(true);
        ui->actionStartAvifToJpeg->setDisabled(true);
        ui->listTerm->clear();
        ui->listTerm->addItem(tr("准备开始..."));
        flagWorking = true;
        emit taskStart();
    }

}

void MainWindow::taskDone(){
    flagWorking = false;
    ui->actionStartJpegToAvif->setDisabled(false);
    ui->actionStartAvifToJpeg->setDisabled(false);
    ui->progressBar->setValue(0);
    ui->listTerm->addItem(tr("任务结束"));
}

void MainWindow::on_actionStartJpegToAvif_triggered()
{
    startConvert(ConvertWorker::JpegToAvif);
}

void MainWindow::on_actionStartAvifToJpeg_triggered()
{
    startConvert(ConvertWorker::AvifToJpeg);
}

void MainWindow::on_actionStop_triggered()
{
    ui->listTerm->addItem(tr("正在停止..."));
    emit taskAbort();
}
