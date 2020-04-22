#include "jpegavifconverter.h"
#include "jpegsegreader.h"
#include "avif.h"
#include <cstdio>
#include <QFile>
#include <QDebug>
#include <QBuffer>
#include <QThread>
#include <QApplication>
#include <QtEndian>

extern "C" {
#include "turbojpeg.h"
}

JpegAvifConverter::JpegAvifConverter(const ImgConvSettings &convSettings): settings(convSettings) {}

bool JpegAvifConverter::ConvertJpegToAvif(const QString &jpegpath, const QString &avifpath){
    tjhandle handle = nullptr;
    int width, height, subsample, colorspace;
    int ret = 0;
    int depth = 8; // I don't know how to get the real depth from turbojpeg
    avifPixelFormat yuv_format = AVIF_PIXEL_FORMAT_YUV420;

    QFile jpegFile(jpegpath);
    QFile avifFile(avifpath);

    if (!jpegFile.open(QIODevice::ReadOnly)){                        // [open]  jpegFile
        qCritical("Can't open file: %s", jpegpath.toUtf8().constData());
        return false;
    }
    if (!avifFile.open(QIODevice::ReadWrite)){                      // [open]  avifFile
        qCritical("Can't open file: %s", avifpath.toUtf8().constData());
        return false;                                               // !EXIT!
    }

    auto jpegBytes = jpegFile.readAll();

    QByteArray icc, exif;
    QBuffer jpeg_io(&jpegBytes);
    jpeg_io.open(QBuffer::ReadOnly);
    JpegSegReader jpegReader(&jpeg_io);
    auto * jpeg_buf = reinterpret_cast<const uint8_t *>(jpegBytes.constData());
    unsigned long jpeg_size = static_cast<std::make_unsigned<int>::type>(jpegBytes.length());

    jpegFile.close();                                               // [close] jpegFile

    handle = tjInitDecompress();                                    // [open] handle
    Q_CHECK_PTR(handle);

    ret = tjDecompressHeader3(handle, jpeg_buf, jpeg_size, &width, &height, &subsample, &colorspace);
    if (ret < 0){
        tjDestroy(handle);                                          // [close] handle
        qCritical("Can't read a valid jpeg head: %s", jpegpath.toUtf8().constData());
        return false;                                               // !EXIT!
    }

    qDebug() << "Loaded JPEG: w=" << width << ", h=" << height;

    // Dump ICC and EXIF here
    while (! jpegReader.atEnd()){
        qDebug("Marker 0x%4x got", jpegReader.current());
        if (jpegReader.current() == JpegSegReader::M_APP1){ // EXIF in APP1 Segment
            exif += jpegReader.read();
            qDebug() << "EXIF read, " << exif.length() << "bytes";
        }
        else if (jpegReader.current() == JpegSegReader::M_APP2){ // ICC Profile was stored in APP2 Segment
            icc += jpegReader.read();
            qDebug() << "ICC read, " << icc.length() << "bytes";
        }

        jpegReader.skip();
    }

    switch (subsample){
    case TJSAMP_420:
        yuv_format = AVIF_PIXEL_FORMAT_YUV420;
        break;
    case TJSAMP_422:
        yuv_format = AVIF_PIXEL_FORMAT_YUV422;
        break;
    case TJSAMP_444:
        yuv_format = AVIF_PIXEL_FORMAT_YUV444;
        break;
    default: // Can't handle this format, it's a FATAL ERROR
        tjDestroy(handle);
        qCritical("Unsupported format");
        return false;
    }

    auto avifImage = avifImageCreate(width, height, depth, yuv_format);
                                                                    // [open] avifImage

    if (icc.length()){
        avifImageSetProfileICC(avifImage,
            reinterpret_cast<uint8_t *>(icc.data()),
            static_cast<std::make_unsigned<int>::type>(icc.length()));
    }

    if (exif.length() && settings.isSaveAvifExif){
        avifImageSetMetadataExif(avifImage,
            reinterpret_cast<uint8_t*>(exif.data()),
            static_cast<std::make_unsigned<int>::type>(exif.length()));
    }

    // Let's try to decode/encode it in YUV PLANES WAY

    // First, allocate memory for planes
    avifImageAllocatePlanes(avifImage, AVIF_PLANES_YUV);

    tjDecompressToYUVPlanes( handle,
                             jpeg_buf,
                             jpeg_size,
                             avifImage->yuvPlanes, // YES
                             width,
                             nullptr,
                             height,
                             0);


    if (ret < 0){
        qCritical("decompress failed: %s", tjGetErrorStr());
        avifImageDestroy(avifImage);                                // [close] avifImage
        tjDestroy(handle);                                          // [close] handle
        return false;                                               // !EXIT!
    }

/*
    avifRGBImage rgb;
    memset(&rgb, 0, sizeof(rgb));

    avifRGBImageSetDefaults(&rgb, avifImage);
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 8;
    avifRGBImageAllocatePixels(&rgb);                               // [open] rgb

    ret = tjDecompress2(handle, jpeg_buf, jpeg_size,
                             rgb.pixels, width, 0, height, TJPF_RGB, 0);

    if (ret < 0){
        qCritical("decompress failed: %s", tjGetErrorStr());
        avifRGBImageFreePixels(&rgb);                               // [close] rgb
        avifImageDestroy(avifImage);                                // [close] avifImage
        tjDestroy(handle);                                          // [close] handle
        avifFile.close();                                           // [close] avifFile

        return false;                                               // !EXIT!
    }
    avifImageRGBToYUV(avifImage, &rgb);

    avifRGBImageFreePixels(&rgb);
*/

    auto encoder = avifEncoderCreate();
    if (! encoder) {
        qCritical("can't create avif encoder");
        avifImageDestroy(avifImage);
        tjDestroy(handle);
        avifFile.close();
    }
    encoder->maxThreads = QThread::idealThreadCount();
    encoder->minQuantizer = settings.minQuantizer;
    encoder->maxQuantizer = settings.maxQuantizer;
    encoder->codecChoice = AVIF_CODEC_CHOICE_RAV1E;
    encoder->speed = 8; // default speed for libavif

    qDebug("starting encode: mt=%d, minQ=%d, maxQ=%d, speed=%d",
           encoder->maxThreads,
           encoder->minQuantizer,
           encoder->maxQuantizer,
           encoder->speed);

    avifRWData raw = AVIF_DATA_EMPTY;
    auto&& encodeResult = avifEncoderWrite(encoder, avifImage, &raw);
    if (encodeResult != AVIF_RESULT_OK){
        qCritical("avif encode failed");
        avifEncoderDestroy(encoder);
        avifRWDataFree(&raw);
        avifImageDestroy(avifImage);
        tjDestroy(handle);
        avifFile.close();
        return false;
    }

    qint64&& write_size = avifFile.write(reinterpret_cast<const char*>(raw.data), static_cast<qint64>(raw.size));
    qint64&& should_write = static_cast<qint64>(raw.size);

    avifEncoderDestroy(encoder);
    avifRWDataFree(&raw);
    avifImageDestroy(avifImage);
    tjDestroy(handle);
    avifFile.close();

    if (write_size != should_write) {
        qCritical("wrote size don't match file size");
        qCritical("should write %lld, wrote %lld", should_write, write_size);
        return false;
    }

    return true;
}

bool JpegAvifConverter::ConvertAvifToJpeg(const QString &avifpath, const QString &jpegpath){
    bool ret = true;

    avifROData raw;
    QFile jpegFile(jpegpath);
    QFile avifFile(avifpath);

    if (!jpegFile.open(QIODevice::ReadWrite)){                        // [open]  jpegFile
        qCritical("Can't open file: %s", jpegpath.toUtf8().constData());
        return false;
    }
    if (!avifFile.open(QIODevice::ReadOnly)){                      // [open]  avifFile
        qCritical("Can't open file: %s", avifpath.toUtf8().constData());
        return false;                                               // !EXIT!
    }

    qDebug("Encode Jpeg to file: %s", jpegpath.toUtf8().constData());

    auto avifBytes = avifFile.readAll();

    raw.data = reinterpret_cast<uint8_t*>(avifBytes.data());
    raw.size = static_cast<std::make_unsigned<int>::type>(avifBytes.length());

    avifImage *image = avifImageCreateEmpty();
    avifDecoder *decoder = avifDecoderCreate();
    avifResult decodeResult = avifDecoderRead(decoder, image, &raw);
    int encodeResult;
    tjhandle handle = nullptr;
    int subsample;
    unsigned char *jpegBuf = nullptr;
    unsigned long jpegSize = 0;

    if (decodeResult == AVIF_RESULT_OK){
        qDebug("avif decode ok");
        switch (image->yuvFormat){
        case AVIF_PIXEL_FORMAT_YUV420:
            subsample = TJSAMP_420;
            break;
        case AVIF_PIXEL_FORMAT_YUV422:
            subsample = TJSAMP_422;
            break;
        case AVIF_PIXEL_FORMAT_YUV444:
            subsample = TJSAMP_444;
            break;
        default:
            subsample = -1;
        }
        if (subsample != -1){
            handle = tjInitCompress();
            if (handle != nullptr){
                encodeResult = tjCompressFromYUVPlanes(
                            handle,
                            const_cast<const uint8_t**>(reinterpret_cast<uint8_t**>(image->yuvPlanes)),
                            static_cast<std::make_signed<unsigned int>::type>(image->width),
                            nullptr,
                            static_cast<std::make_signed<unsigned int>::type>(image->height),
                            subsample,
                            &jpegBuf,
                            &jpegSize,
                            settings.jpegQuality,
                            0
                            );
                if (encodeResult != -1){
                    qDebug("turbojpeg encode ok");
                    // Here, We got the metadatas from avif
                    // and JPEG from turbojpeg

                    if (settings.isSaveJpegExif){
                        // build a header
                        quint16 m;
                        m = qToBigEndian(JpegSegReader::M_SOI);
                        jpegFile.write( reinterpret_cast<char *>(&m), 2);

                        qDebug() << "EXIF size: "<< image->exif.size;
                        if (image->exif.size >= 2){
                            m = qToBigEndian(JpegSegReader::M_APP1);
                            jpegFile.write( reinterpret_cast<char *>(&m), 2);
                            jpegFile.write(
                                reinterpret_cast<char*>(image->exif.data),
                                static_cast<std::make_signed<size_t>::type>(image->exif.size)
                            );
                        }

                        qDebug() << "ICC size: "<< image->icc.size;
                        if (image->icc.size >= 2){
                            m = qToBigEndian(JpegSegReader::M_APP2);
                            jpegFile.write( reinterpret_cast<char *>(&m), 2);
                            jpegFile.write(
                                reinterpret_cast<char*>(image->icc.data),
                                static_cast<std::make_signed<size_t>::type>(image->icc.size)
                            );
                        }


                        int i = 0;
                        // It's quite straightforward, is it ?
#define get_word(x)  (qFromBigEndian(*reinterpret_cast<quint16 *>(&jpegBuf[x])))

                        Q_ASSERT( get_word(i) == JpegSegReader::M_SOI );
                        i += 2; // Skip SOI

                        while ( (m = get_word(i)) != JpegSegReader::M_SOS){
                            if ( (m & 0xFFE0) == 0xFFE0 ){ // APP0 to APP15 is 0xFFE0 to 0xFFEF
                                // get_word(i + 2) is the next word, the length of segment
                                // But the marker itself is not included, so plus 2 to skip it too
                                i += get_word(i + 2) + 2;
                            }
                            else {
                                jpegFile.write(
                                    reinterpret_cast<char*>(&jpegBuf[i]),
                                    get_word(i + 2) + 2);

                                i += get_word(i + 2) + 2;
                            }
                        }
#undef get_word

                        // m == M_SOS here
                        Q_ASSERT( static_cast<std::make_unsigned<int>::type>(i) < jpegSize );

                        jpegFile.write( reinterpret_cast<char*>(&jpegBuf[i]),
                                        static_cast<std::make_signed<size_t>::type>(jpegSize) - i);
                    }
                    else {
                        // TODO: NO ICC Write
                        jpegFile.write( reinterpret_cast<char*>(jpegBuf),
                                        static_cast<std::make_signed<size_t>::type>(jpegSize));
                    }
                }
                else {
                    qCritical("jpeg encode failed");
                    ret = false;
                }
            }
            else {
                qCritical("turbojpeg init failed");
                ret = false;
            }
            tjDestroy(handle);
        }
        else {
            qCritical("unsupported format");
            ret = false;
        }
    }
    else {
        qCritical("avif decode failed");
        qCritical("ERROR: Failed to decode: %s\n", avifResultToString(decodeResult));
        ret = false;
    }
    avifDecoderDestroy(decoder);
    avifImageDestroy(image);
    return ret;
}
