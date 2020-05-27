#ifndef CONVERTWORKER_H
#define CONVERTWORKER_H

#include <QMutex>

#include "convertsettings.h"

enum WorkerAction {
    ActionJpegToAvif,
    ActionAvifToJpeg
};

class ConvertWorker: public QObject {
    Q_OBJECT
    QString srcPath;
    QString dstPath;
    bool flagAbort = false;
    QMutex mutex;
    ConvertSettings settings;
    WorkerAction action = ActionJpegToAvif;
    QStringList files;
    int file_count;
    int file_passed;

public:
    static const WorkerAction JpegToAvif = ActionJpegToAvif;
    static const WorkerAction AvifToJpeg = ActionAvifToJpeg;

    void setAction(WorkerAction);
    void setPath(const QString &, const QString &);
    void setParameter(const ConvertSettings &);

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

#endif // CONVERTWORKER_H
