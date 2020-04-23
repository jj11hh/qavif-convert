#include "dialogsettings.h"
#include "ui_dialogsettings.h"

DialogSettings::DialogSettings(QWidget *parent, const ImgConvSettings &settings) :
    QDialog(parent),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);
    ui->sliderMaxQuantizer->setValue(settings.maxQuantizer);
    ui->sliderMinQuantizer->setValue(settings.minQuantizer);
    ui->labelMaxQuantizer->setText(QString::number(settings.maxQuantizer));
    ui->labelMinQuantizer->setText(QString::number(settings.minQuantizer));
    ui->checkAVIFEXIF->setChecked(settings.isSaveAvifExif);
    ui->checkJPEGEXIF->setChecked(settings.isSaveJpegExif);
    ui->sliderJpegQuality->setValue(settings.jpegQuality);
    setWindowIcon(QIcon(":/Icons/Images/icon.png"));
}

DialogSettings::~DialogSettings()
{
    delete ui;
}

void DialogSettings::on_sliderMaxQuantizer_valueChanged(int value)
{
    if (ui->sliderMinQuantizer->value() > value){
        ui->sliderMinQuantizer->setValue(value);
    }
    ui->labelMaxQuantizer->setText(QString::number(value));
}

void DialogSettings::on_sliderMinQuantizer_valueChanged(int value)
{
    if (ui->sliderMaxQuantizer->value() < value){
        ui->sliderMaxQuantizer->setValue(value);
    }
    ui->labelMinQuantizer->setText(QString::number(value));
}

void DialogSettings::on_sliderJpegQuality_valueChanged(int value)
{
    ui->labelJpegQuality->setText(QString::number(value));
}

void DialogSettings::on_DialogSettings_accepted()
{
    ImgConvSettings settings;
    settings.maxQuantizer = ui->sliderMaxQuantizer->value();
    settings.minQuantizer = ui->sliderMinQuantizer->value();
    settings.isSaveAvifExif = ui->checkAVIFEXIF->checkState();
    settings.isSaveJpegExif = ui->checkJPEGEXIF->checkState();
    settings.jpegQuality = ui->sliderJpegQuality->value();

    emit sendSettings(settings);
}

