#ifndef EXIFDUMP_H
#define EXIFDUMP_H

#include <QIODevice>

class ExifDump
{
public:
    static bool Dump(QIODevice *source, QIODevice *exif=nullptr, QIODevice *striped=nullptr);
    static bool ReAssemble(QIODevice *source, QIODevice *exif, QIODevice *dest);
};

#endif // EXIFDUMP_H
