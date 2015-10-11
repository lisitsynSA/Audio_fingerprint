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
    max_y = 0;
    m_buffer = QByteArray(BufferSize, 0);
    bundle_size = 0;
    last_time = 0;

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

    ui->progress_voice->setValue(0);
    ui->progress_wavelet->setValue(0);
    setCentralWidget(ui->audio_widget);
    connect(ui->deviceBox, SIGNAL(activated(int)),
            this, SLOT(deviceChanged(int)));
    connect(ui->start_button, SIGNAL(clicked()),
            this, SLOT(start_button()));

    // create graph:
    ui->plot->addGraph();
    ui->wavelet->addGraph();
    // give the axes some labels:
    ui->plot->xAxis->setLabel("Time(ms)");
    ui->plot->yAxis->setLabel("Voise(amp)");
    ui->wavelet->xAxis->setLabel("Time(ms)");
    ui->wavelet->yAxis->setLabel("Frequency(Hz)");
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

void MainWindow::print_output_csv(quint16 *output)
{
    qDebug() << "SAVE OUTPUT";
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
}

void MainWindow::get_output(quint16 *output)
{
    double time = (double)timer.elapsed() - last_time;
    for (int i = 0; i < bundle_size; ++i)
    {
        x.push_back(last_time + time*i/bundle_size);
        y.push_back(output[i]);
        max_y = qMax(max_y, output[i]);
    }
    ui->plot->graph(0)->setData(x, y);

    last_time += time;

    // set axes ranges, so we see all data:
    ui->plot->xAxis->setRange(0, last_time);
    ui->plot->yAxis->setRange(0, (double) max_y);
    ui->plot->replot();

    delete output;
    ui->lcd_time->display(((double)timer.elapsed())/1000);
    continue_get_output();
}

void MainWindow::continue_get_output()
{
    output_size -= bundle_size;
    ui->progress_voice->setValue((int)(100*x.size()/(x.size() + output_size)));
    if (output_size > 0)
        audioinfo->set_output_size(bundle_size);
}

void MainWindow::start_button()
{
    qDebug() << "START WRITE OUTPUT";
    timer.start();
    max_y = 0;
    x.clear();
    y.clear();
    last_time = 0;
    output_size = ui->spinBox_number->value();
    bundle_size = ui->spinBox_bundle->value();
    audioinfo->set_output_size(bundle_size);
    ui->progress_voice->setValue(0);
}

void MainWindow::start_wavelet()
{
    ui->progress_wavelet->setValue(0);
}
