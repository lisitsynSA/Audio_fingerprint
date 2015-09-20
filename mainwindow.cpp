#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QAudioDeviceInfo>
#include <QAudioInput>

#include <QtCore/qendian.h>

const int BufferSize = 4096;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    device_info(QAudioDeviceInfo::defaultInputDevice()),
    audioinfo(0),
    input(0),
    device(0),
    m_buffer(BufferSize, 0)
{
    ui->setupUi(this);
    initializeWindow();
    initializeAudio();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeWindow()
{
    ui->progressBar->setValue(0);
    ui->spinBox->setMaximum(10000);
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0; i < devices.size(); ++i)
        ui->deviceBox->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));

    connect(ui->deviceBox, SIGNAL(activated(int)), SLOT(deviceChanged(int)));
}

void MainWindow::initializeAudio()
{
    format.setFrequency(8000);
    format.setChannels(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultInputDevice());
    if (!info.isFormatSupported(format))
    {
        qDebug() << "Default format not supported - trying to use nearest";
        format = info.nearestFormat(format);
    }

    audioinfo  = new audioinfo_t(format, this);
    connect(audioinfo, SIGNAL(update()), SLOT(refreshDisplay()));

    createAudioInput();
}

void MainWindow::createAudioInput()
{
    input = new QAudioInput(device_info, format, this);
    connect(input, SIGNAL(notify()), SLOT(notified()));
    audioinfo->start();
    input->start(audioinfo);
}

void MainWindow::notified()
{
    qDebug() << "Voice = " << audioinfo->get_level();
}

void MainWindow::readMore()
{
    if(!input)
        return;
    qint64 len = input->bytesReady();
    if(len > 4096)
        len = 4096;
    qint64 l = device->read(m_buffer.data(), len);
    if(l > 0) {
        audioinfo->write(m_buffer.constData(), l);
    }
}

void MainWindow::deviceChanged(int index)
{
    audioinfo->stop();
    input->stop();
    input->disconnect(this);
    delete input;

    device_info = ui->deviceBox->itemData(index).value<QAudioDeviceInfo>();
    createAudioInput();
}

void MainWindow::refreshDisplay()
{
    ui->progressBar->setValue((int)(100*audioinfo->get_level()));
}
