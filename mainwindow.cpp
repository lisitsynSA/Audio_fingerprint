#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <QtCore/qendian.h>
#include <QDebug>

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

    initializeWindow();
    initializeAudio();
    waves.push_back(new wave_t(ui, colorMap, colorScale, this));
    waves.push_back(new wave_t(ui, colorMap, colorScale, this));
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
    ui->actionBox->addItem("Save");
    ui->actionBox->addItem("Load");
    ui->actionBox->addItem("Calculate diff");
    ui->currentBox->addItem("Main wave");
    ui->currentBox->addItem("Argument wave");
    ui->doaction_button->setEnabled(false);
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
    connect(ui->analysis_button, SIGNAL(clicked()),
            this, SLOT(wavelet_analysis()));
    connect(ui->actionBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(change_action(QString)));
    connect(ui->currentBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(change_wave(int)));

    ui->plot->addGraph();
    colorMap = new QCPColorMap(ui->wavelet->xAxis, ui->wavelet->yAxis);
    ui->wavelet->addPlottable(colorMap);
    // give the axes some labels:
    ui->plot->xAxis->setLabel("Time(ms)");
    ui->plot->yAxis->setLabel("Voise(amp)");
    ui->wavelet->xAxis->setLabel("Time(ms)");
    ui->wavelet->yAxis->setLabel("Frequency(Hz)");

    colorMap->data()->setSize(50, 50);
    colorMap->data()->setRange(QCPRange(0, 2), QCPRange(0, 2));
    for (int x=0; x<50; ++x)
      for (int y=0; y<50; ++y)
        colorMap->data()->setCell(x, y, qCos(x/10.0)+qSin(y/10.0));
    colorMap->setGradient(QCPColorGradient::gpThermal);//gpThermal
    colorMap->rescaleDataRange(true);
    colorScale = new QCPColorScale(ui->wavelet);
    colorScale->setGradient(QCPColorGradient::gpThermal);//gpThermal
    colorScale->setDataRange(QCPRange(-2, 2));
    ui->wavelet->plotLayout()->addElement(0, 1, colorScale);
    //colorScale->setLabel("Some Label Text");

    ui->wavelet->rescaleAxes();
    ui->wavelet->replot();
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
    else if (ui->actionBox->currentText() == "Load")
    {
        ui->progress_voice->setValue(100);
        waves[current_wave]->read_wave(ui->action_line->text());
    } else if (ui->actionBox->currentText() == "Calculate diff")
        calculate_diff();
}

void MainWindow::change_action(QString action)
{
    if (action == "Save" && ui->progress_voice->value() != 100)
        ui->doaction_button->setEnabled(false);
    else
        ui->doaction_button->setEnabled(true);
}

void MainWindow::change_wave(int wave)
{
    current_wave = wave;
    waves[current_wave]->load_wave();
}

void MainWindow::wavelet_analysis()
{
    if (ui->progress_wavelet->value() == 100)
    {
        waves[current_wave]->wavelet_analysis();
        ui->progress_wavelet->setValue(100);
    }
}

void MainWindow::calculate_diff()
{
    double scale = ui->spinBox_scale->value();
    double lowerScale = ui->value1->value();
    double upperScale = ui->value2->value();
    double diff = 0;
    double max = 0, min = 0;
    int last_time = qMin(waves[0]->voice_y.size(), waves[1]->voice_y.size());

    for (int freq = 0; freq < (upperScale - lowerScale)*scale; freq++)
    for (int time = 0; time < last_time; time++)
    {
        diff = waves[0]->local_colorMap->data()->cell(time, freq) -\
               waves[1]->local_colorMap->data()->cell(time, freq);
        min = qMin(min, diff);
        max = qMax(max, diff);
        colorMap->data()->setCell(time, freq, diff);
    }
    colorMap->data()->setRange(QCPRange(0, last_time), QCPRange(70, 1500));
    colorMap->rescaleDataRange(true);
    colorScale->setDataRange(QCPRange(min, max));
    ui->wavelet->rescaleAxes();
    ui->wavelet->replot();
}
