#ifndef CONVWORKER_H
#define CONVWORKER_H

#include <QMutex>

#include "imgconvsettings.h"

enum WorkerAction {
    ActionJpegToAvif,
    ActionAvifToJpeg
};

class ConvWorker: public QObject {
    Q_OBJECT
    QString srcPath;
    QString dstPath;
    bool flagAbort = false;
    QMutex mutex;
    ImgConvSettings settings;
    WorkerAction action = ActionJpegToAvif;
    QStringList files;
    int file_count;
    int file_passed;

public:
    static const WorkerAction JpegToAvif = ActionJpegToAvif;
    static const WorkerAction AvifToJpeg = ActionAvifToJpeg;

    void setAction(WorkerAction);
    void setPath(const QString &, const QString &);
    void setParameter(const ImgConvSettings &);

public slots:
    void abort();
    void doWork();

private slots:
    void processImage();

signals:
    void resultReady(const QString &result);
    void progressReady(int percent);
    void workDone();
};

#endif // CONVWORKER_H
