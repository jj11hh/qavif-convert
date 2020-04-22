#include "exifdump.h"
#include <QByteArray>
#include <QtEndian>

static const quint16 MARK_SOI = 0xFFD8;
static const quint16 MARK_SOS = 0xFFDA;
static const quint16 MASK_APP = 0xFFF0;
static const quint16 MARK_APPx = 0xFFE0;

bool ExifDump::Dump(QIODevice *source, QIODevice *exif, QIODevice *striped){
    quint16 mark = 0;
    quint16 seg_size = 0;
    QByteArray segment;

    qint64 len;

    Q_CHECK_PTR(source);

    len = source->read(reinterpret_cast<char *>(&mark), sizeof(quint16));
    if (len != 2){
        qCritical("unexcepted end");
        return false;
    }

    if (qFromBigEndian(mark) != MARK_SOI){
        qCritical("no SOI mark read");
        return false;
    }

    mark = qToBigEndian(mark);
    if (exif){
        exif->write(reinterpret_cast<char *>(&mark), sizeof(quint16));
    }

    if (striped){
        striped->write(reinterpret_cast<char *>(&mark), sizeof(quint16));
    }

    while (! source->atEnd()){
        len = source->read(reinterpret_cast<char *>(&mark), sizeof(quint16));
        if (len != 2){
            qCritical("unexcepted end");
            return false;
        }
        if (qFromBigEndian(mark) == MARK_SOS){
            if (striped){
                while (! source->atEnd()){
                    segment = source->read(4096);
                    striped->write(segment);
                }
            }
            return true;
        }

        len = source->read(reinterpret_cast<char *>(&seg_size), sizeof(quint16));
        if (len != 2){
            qCritical("unexcepted end");
            return false;
        }

        segment.clear();
        segment.append(reinterpret_cast<char *>(&mark), sizeof(quint16));
        segment.append(reinterpret_cast<char *>(&seg_size), sizeof(quint16));
        segment += source->read(qFromBigEndian(seg_size) - 2);

        if (segment.length() < qFromBigEndian(seg_size) + 2){
            qCritical("unexcepted end");
            return false;
        }

        if ((qFromBigEndian(mark) & MASK_APP ) == MARK_APPx){
            if (exif){
                exif->write(segment);
            }
        }
        else {
            if (striped){
                striped->write(segment);
            }
        }
    }

    /* no SOS mark read */
    return false;
}

bool ExifDump::ReAssemble(QIODevice *source, QIODevice *exif, QIODevice *dest){
    Q_CHECK_PTR(source);
    Q_CHECK_PTR(exif);
    Q_CHECK_PTR(dest);

    quint16 mark;
    quint16 seg_size;
    qint64 len;
    QByteArray buffer;

    len = source->read(reinterpret_cast<char *>(&mark), sizeof(quint16));
    Q_ASSERT(len == 2);
    if (qFromBigEndian(mark) != MARK_SOI){
        qCritical("no SOI mark read");
        return false;
    }

    while (! exif->atEnd()){
        buffer = exif->read(4096);
        dest->write(buffer);
    }

    while (! source->atEnd()){
        len = source->read(reinterpret_cast<char *>(&mark), sizeof(quint16));
        Q_ASSERT(len == 2);
        if (qFromBigEndian(mark) == MARK_SOS){

            dest->write(reinterpret_cast<char *>(mark), sizeof(quint16));
            while (! source->atEnd()){
                buffer = source->read(4096);
                dest->write(buffer);
            }

            return true;
        }

        len = source->read(reinterpret_cast<char *>(&seg_size), sizeof(quint16));
        Q_ASSERT(len == 2);



        if ((qFromBigEndian(mark) & MASK_APP ) == MARK_APPx){
            source->skip(seg_size - 2);
        }
        else {
            buffer.clear();
            buffer.append(reinterpret_cast<char *>(&mark), sizeof(quint16));
            buffer.append(reinterpret_cast<char *>(&seg_size), sizeof(quint16));
            buffer += source->read(qFromBigEndian(seg_size) - 2);

            if (buffer.length() < qFromBigEndian(seg_size) + 2){
                qCritical("unexcepted end");
                return false;
            }
        }
    }

    /* no SOS mark read */
    return false;
}

