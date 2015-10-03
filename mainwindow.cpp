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
    last_point = 0;
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
    connect(audioinfo, SIGNAL(output_ready(qreal*)),
            this, SLOT(get_output(qreal*)));
    connect(audioinfo, SIGNAL(int_output_ready(quint16*)),
            this, SLOT(get_int_output(quint16*)));
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

void MainWindow::get_int_output(quint16 *output)
{
    qDebug() << "INT OUTPUT IS READY";
    /*QFile file("output.csv");
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Audio recognition"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
    } else {
        QTextStream out(&file);
        for (int i = 0; i < output_size; i++)
            out << (int) output[i] << ";";
    }*/
    if (output_size)
    {
        double time = (double)timer.elapsed() - last_time;
        for (int i = 0; i < output_size; ++i)
        {
            x.push_back(last_time + time*i/output_size);
            y.push_back(output[i]);
            max_y = qMax(max_y, output[i]);
        }
        // create graph and assign data to it:
        ui->plot->addGraph();
        ui->plot->graph(0)->setData(x, y);
        // give the axes some labels:
        ui->plot->xAxis->setLabel("Time(ms)");
        ui->plot->yAxis->setLabel("Voise(amp)");

        last_point += output_size;
        last_time += time;

        // set axes ranges, so we see all data:
        ui->plot->xAxis->setRange(0, last_time);
        ui->plot->yAxis->setRange(0, (double) max_y);
        ui->plot->replot();
    }
    if (bundle_size)
    {
        for (int i = 0; i < bundle_size; ++i)
        {
            y.push_back(output[i]);
            y.pop_front();
            max_y = qMax(max_y, output[i]);
        }
        // create graph and assign data to it:
        ui->plot->addGraph();
        ui->plot->graph(0)->setData(x, y);

        // set axes ranges, so we see all data:
        ui->plot->yAxis->setRange(0, (double) max_y);
        ui->plot->replot();

    }
    delete output;
    ui->lcd_time->display(((double)timer.elapsed())/1000);
    continue_get_int_output();
}

void MainWindow::continue_get_int_output()
{
    qDebug() << "CONTINUE WRITE OUTPUT";
    bundle_size = ui->spinBox_bundle->value();
    output_size = 0;
    if (bundle_size)
        audioinfo->set_int_output_size(bundle_size);
}

void MainWindow::get_output(qreal *output)
{
    qDebug() << "OUTPUT IS READY";
    /*double time = (double)timer.elapsed();
    double max_y = 0;
    QVector<double> x(output_size), y(output_size);
    for (int i = 0; i < output_size; ++i)
    {
      x[i] = time*i/output_size;
      y[i] = output[i];
      max_y = qMax(max_y, y[i]);
    }
    // create graph and assign data to it:
    ui->plot->addGraph();
    ui->plot->graph(0)->setData(x, y);
    // give the axes some labels:
    ui->plot->xAxis->setLabel("Time(ms)");
    ui->plot->yAxis->setLabel("Voise(%)");
    // set axes ranges, so we see all data:
    ui->plot->xAxis->setRange(0, time);
    ui->plot->yAxis->setRange(0, max_y);
    ui->plot->replot();*/

    delete output;
    ui->lcd_time->display(((double)timer.elapsed())/1000);
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
    audioinfo->set_int_output_size(output_size);
}
