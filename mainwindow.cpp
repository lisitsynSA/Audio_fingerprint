#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <QtCore/qendian.h>

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
    bundle_size = 0;
    current_wave = 0;
    ui->setupUi(this);
    waves.push_back(new wave_t(ui, this));

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

    ui->progress_voice->setValue(0);
    ui->progress_wavelet->setValue(0);
    setCentralWidget(ui->audio_widget);
    connect(ui->deviceBox, SIGNAL(activated(int)),
            this, SLOT(deviceChanged(int)));
    connect(ui->start_button, SIGNAL(clicked()),
            this, SLOT(start_button()));
    connect(ui->start_wavelet, SIGNAL(clicked()),
            this, SLOT(start_wavelet()));
    connect(ui->noise_button, SIGNAL(clicked()),
            this, SLOT(wave_clearing()));
    connect(ui->doaction_button, SIGNAL(clicked()),
            this, SLOT(do_button()));

    ui->actionBox->addItem("Save");
    ui->actionBox->addItem("Load");
    ui->noise_button->setEnabled(false);
    ui->doaction_button->setEnabled(false);
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
    waves[current_wave]->get_output(output, bundle_size, output_size);
    continue_get_output();
}

void MainWindow::continue_get_output()
{
    output_size -= bundle_size;
    if (output_size > 0)
        audioinfo->set_output_size(bundle_size);
    else
    {
        qDebug() << "END WRITE OUTPUT";
        ui->progress_voice->setValue(100);
        ui->doaction_button->setEnabled(true);
    }
}

void MainWindow::start_button()
{
    qDebug() << "START WRITE OUTPUT";
    output_size = ui->spinBox_number->value();
    bundle_size = ui->spinBox_bundle->value();
    ui->progress_voice->setValue(0);
    waves[current_wave]->start();
    audioinfo->set_output_size(bundle_size);
}

void MainWindow::start_wavelet()
{
    if (ui->progress_voice->value() == 100)
    {
        ui->progress_wavelet->setValue(0);
        qDebug() << "START WAVELET";
        waves[current_wave]->run_wavelet();
        qDebug() << "END WAVELET";
        ui->progress_wavelet->setValue(100);
    }
}

void MainWindow::wave_clearing()
{
    waves[current_wave]->wave_clearing();
}

void MainWindow::do_button()
{
    if (ui->actionBox->currentText() == "Save")
        waves[current_wave]->write_wave(ui->action_line->text());
    if (ui->actionBox->currentText() == "Load")
        waves[current_wave]->read_wave(ui->action_line->text());
}
