#ifndef JPEGAVIFCONVERTOR_H
#define JPEGAVIFCONVERTOR_H

#include <QIODevice>
#include "imgconvsettings.h"


class JpegAvifConvertor
{
public:
    JpegAvifConvertor(const ImgConvSettings &);
    bool ConvertJpegToAvif(const QString &, const QString &);
    bool ConvertAvifToJpeg(const QString &, const QString &);

private:
    ImgConvSettings settings;
};

#endif // JPEGAVIFCONVERTOR_H
