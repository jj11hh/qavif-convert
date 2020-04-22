#include "imgconvsettings.h"

ImgConvSettings::ImgConvSettings(int minQ, int maxQ, int jpegQ, bool saveExif, bool restoreExif)
    : minQuantizer(minQ), maxQuantizer(maxQ), jpegQuality(jpegQ), isSaveAvifExif(saveExif), isSaveJpegExif(restoreExif)
{}

ImgConvSettings::~ImgConvSettings(){}
ImgConvSettings::ImgConvSettings(const ImgConvSettings &other){
    minQuantizer = other.minQuantizer;
    maxQuantizer = other.maxQuantizer;
    jpegQuality = other.jpegQuality;
    isSaveAvifExif = other.isSaveAvifExif;
    isSaveJpegExif = other.isSaveJpegExif;
}
