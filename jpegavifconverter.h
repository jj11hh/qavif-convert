#ifndef JPEGAVIFCONVERTER_H
#define JPEGAVIFCONVERTER_H

#include <QIODevice>
#include "imgconvsettings.h"


class JpegAvifConverter
{
public:
    JpegAvifConverter(const ImgConvSettings &);
    bool ConvertJpegToAvif(const QString &, const QString &);
    bool ConvertAvifToJpeg(const QString &, const QString &);

private:
    ImgConvSettings settings;
};

#endif // JPEGAVIFCONVERTER_H
