#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/qendian.h>
#include <QFile>
#include <QMessageBox>

const int BufferSize = 4096;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    device_info = QAudioDeviceInfo::defaultInputDevice();
    audioinfo = 0;
    input = 0;
    device = 0;
    m_buffer = QByteArray(BufferSize, 0);

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
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0; i < devices.size(); ++i)
        ui->deviceBox->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));

    setCentralWidget(ui->audio_widget);
    connect(ui->deviceBox, SIGNAL(activated(int)),
            this, SLOT(deviceChanged(int)));
    connect(ui->start_button, SIGNAL(clicked()),
            this, SLOT(start_button()));
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
    connect(audioinfo, SIGNAL(output_ready(quint16*)),
            this, SLOT(get_output(quint16*)));
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
    ui->lcd_value_slow->display(audioinfo->get_value());
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
    ui->lcd_value->display(audioinfo->get_value());
}


void MainWindow::get_output(quint16 *output)
{
    qDebug() << "OUTPUT IS READY";
    QFile file("output.csv");
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Audio recognition"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
    } else {
        QTextStream out(&file);
        for (int i = 0; i < output_size; i++)
            out << (int) output[i] << ";";
    }
    delete output;
    ui->lcd_time->display(((double)timer.elapsed())/1000);
}

void MainWindow::start_button()
{
    qDebug() << "START WRITE OUTPUT";
    timer.start();
    output_size = ui->spinBox->value();
    audioinfo->set_output_size(output_size);
}
