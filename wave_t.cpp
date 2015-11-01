#include "wave_t.h"
#include "ui_mainwindow.h"
#include "fftw3.h"
#include <cmath>
#include <QDebug>
#include <QMessageBox>
#include <QFile>

wave_t::wave_t(Ui::MainWindow *init_ui, QCPColorMap* init_colorMap,\
               QCPColorScale* init_colorScale, QObject *parent):
    QObject(parent),
    ui(init_ui),
    max_y(0),
    colorMap(init_colorMap),
    colorScale(init_colorScale)
{
    qDebug() << "CREATE NEW WAVE";
    local_colorMap = new QCPColorMap(ui->wavelet->xAxis, ui->wavelet->yAxis);
}

void wave_t::wave_clearing()
{
    qDebug() << "WAVE CLEARING";
    double noise_level = ui->noise_spinbox->value();
    for (int y = 0; y < voice_y.size(); y++)
        if (noise_level > voice_y[y])
            voice_y[y] = 0;
    ui->plot->graph(0)->setData(voice_x, voice_y);
    ui->plot->replot();
}

void wave_t::start()
{
    timer.start();
    max_y = 0;
    voice_x.clear();
    voice_y.clear();
    last_time = 0;
}

void wave_t::get_output(quint16 *output, int bundle_size, int output_size)
{
    double time = (double)timer.elapsed() - last_time;
    for (int i = 0; i < bundle_size; i++)
    {
        voice_x.push_back(last_time + time*i/bundle_size);
        voice_y.push_back(output[bundle_size - i - 1]);
        max_y = qMax(max_y, output[bundle_size - i - 1]);
    }
    ui->plot->graph(0)->setData(voice_x, voice_y);

    last_time += time;

    // set axes ranges, so we see all data:
    ui->plot->xAxis->setRange(0, last_time);
    ui->plot->yAxis->setRange(0, (double) max_y);
    ui->plot->replot();

    delete output;
    ui->lcd_time->display(((double)timer.elapsed())/1000);
    ui->progress_voice->setValue((int)(100*voice_x.size()/(voice_x.size() + output_size)));
}

void wave_t::print_output_csv(QString file_name)
{
    qDebug() << "SAVE OUTPUT";
    QFile file(file_name + ".csv");
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(ui->doaction_button, tr("Audio recognition"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
    } else {
        QTextStream out(&file);
        out << (int) voice_y.size() << "\n";
        for (int i = 0; i < voice_y.size(); i++)
            out << (int) voice_y[i] << ";";
    }
}

void wave_t::write_wave(QString file_name)
{
    qDebug() << "WRITE WAVE";
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(ui->doaction_button, tr("Audio recognition"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
    } else {
        QDataStream out(&file);
        out << voice_x;
        out << last_time;
        out << voice_y;
        out << max_y;
        QMessageBox::information(ui->doaction_button, tr("Audio recognition"),
                                 tr("Wave has been saved to file %1.")
                                 .arg(file.fileName()));
    }
}

void wave_t::read_wave(QString file_name)
{
    qDebug() << "READ WAVE";
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(ui->doaction_button, tr("Audio recognition"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
    } else {
        QDataStream in(&file);
        voice_x.clear();
        voice_y.clear();
        in >> voice_x;
        in >> last_time;
        in >> voice_y;
        in >> max_y;
        ui->plot->xAxis->setRange(0, last_time);
        ui->plot->yAxis->setRange(0, (double) max_y);
        ui->plot->graph(0)->setData(voice_x, voice_y);
        ui->plot->replot();
        QMessageBox::information(ui->doaction_button, tr("Audio recognition"),
                                 tr("Wave has been loaded from file %1.")
                                 .arg(file.fileName()));
    }
}

void wave_t::load_wave()
{
    ui->plot->xAxis->setRange(0, last_time);
    ui->plot->yAxis->setRange(0, (double) max_y);
    ui->plot->graph(0)->setData(voice_x, voice_y);
    ui->plot->replot();

    colorMap->data()->setSize(voice_y.size(), (upperScale - lowerScale)*scale);
    for (int time = 0; time < voice_y.size(); time++)
    for (int freq = 0; freq < (upperScale - lowerScale)*scale; freq++)
        colorMap->data()->setCell(time, freq, local_colorMap->data()->cell(time, freq));

    colorMap->data()->setRange(QCPRange(0, last_time), QCPRange(70, 1500));
    colorMap->rescaleDataRange(true);
    colorScale->setDataRange(QCPRange(min, max));

    ui->wavelet->addPlottable(colorMap);
    ui->wavelet->rescaleAxes();
    ui->wavelet->replot();
}

void wave_t::wavelet_analysis()
{
    scale = ui->spinBox_scale->value();
    freq_noise = ui->freq_noise->value();
    /// The smallest scale to render.
    lowerScale = ui->value1->value();//20

    /// The largest scale to render.
    upperScale = ui->value2->value();//50

    for (int time = 0; time < voice_y.size(); time++)
    {
        double max_freq = freq_noise;
        int number_max_freq = -1;
        for (int freq = 0; freq < (upperScale - lowerScale)*scale; freq++)
            if (max_freq < local_colorMap->data()->cell(time, freq))
            {
                max_freq = local_colorMap->data()->cell(time, freq);
                number_max_freq = freq;
                //qDebug() << "ANALYSIS MAX: time = " << time << "; max_freq = " << max_freq << ";number = "<< number_max_freq;
            }

        if (number_max_freq != -1)
        {
            //qDebug() << "ANALYSIS: time = " << time << "; max_freq = " << max_freq << ";number = "<< number_max_freq;
            int barrier = 0;
            for (int freq = number_max_freq; freq < (upperScale - lowerScale)*scale; freq++)
                if (local_colorMap->data()->cell(time, freq) < freq_noise)
                {
                    barrier = 1;
                    local_colorMap->data()->setCell(time, freq, 0);
                } else if (barrier){
                    local_colorMap->data()->setCell(time, freq, 0);
                }
            barrier = 0;
            for (int freq = number_max_freq; freq >= 0; freq--)
                if (local_colorMap->data()->cell(time, freq) < freq_noise)
                {
                    barrier = 1;
                    local_colorMap->data()->setCell(time, freq, 0);
                } else if (barrier){
                    local_colorMap->data()->setCell(time, freq, 0);
                }
        } else{
            for (int freq = 0; freq < (upperScale - lowerScale)*scale; freq++)
                local_colorMap->data()->setCell(time, freq, 0);
        }
        ui->progress_wavelet->setValue(((100*time)/voice_y.size()));

    }
    load_wave();
}

float wave_t::FTWavelet( float value, float scale, float f0 )
{
    //if ( x < 0.9 / scale ||  x > 1.1 / scale ) {
    //    return (float)0.0;
    //}

    static const float pi = (float)3.14159265358979323846;
    static const double two_pi_f0 = 2.0 * pi * f0;
    static const double multiplier = 1.8827925275534296252520792527491;

    scale *= (float)f0;

    // 1.88279*exp(-0.5*(2*pi*x*10-2*pi*10)^2)

    float basic = (float)(multiplier *
            exp(-0.5*(2*pi*value*scale - two_pi_f0)*(2*pi*value*scale - two_pi_f0)));

    // pi^0.25*sqrt(2.0)*exp(-0.5*(2*pi*x*scale-2*pi*0.849)^2)
    return sqrt(scale)*basic;
}

void wave_t::run_wavelet()
{
    scale = ui->spinBox_scale->value();
    freq_noise = ui->freq_noise->value();
    /// The smallest scale to render.
    lowerScale = ui->value1->value();//20

    /// The largest scale to render.
    upperScale = ui->value2->value();//50

    /// "Wave number". Higher means more frequency localization. Smaller means
    /// more time localization.
    double f0 = 10;

    unsigned avoid_overlap = (unsigned)(upperScale * 20);
    double df = pow(upperScale/lowerScale, 1.0/(scale*(upperScale-lowerScale)));
    unsigned N = (int)pow(2, ceil(log((double)(voice_y.size() + avoid_overlap)*2)/log((double)2)));


    // Iniitalize the fast fourier transform.
    // See fftwf3 documentation online.
    fftwf_plan plan_forward;
    fftwf_plan plan_inverse;
    fftwf_complex* data = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_complex* ans = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);

    plan_inverse = fftwf_plan_dft_1d(N, ans, ans, FFTW_BACKWARD,  FFTW_ESTIMATE);
    plan_forward = fftwf_plan_dft_1d(N, data, data, FFTW_FORWARD, FFTW_ESTIMATE);

    memset(data, 0, sizeof(fftwf_complex)*N);
    memset(ans, 0, sizeof(fftwf_complex)*N);


    for( int i = 0; i < voice_y.size(); i++ ) {
        data[i][0] = (float)voice_y[i];
    }

    fftwf_execute(plan_forward);

    //prepare plot
    colorMap->data()->setSize(voice_y.size(), (upperScale - lowerScale)*scale);
    local_colorMap->data()->setSize(voice_y.size(), (upperScale - lowerScale)*scale);
    int row = 0;
    max = 0;
    min = 1000;

    for ( double period = lowerScale; period <= upperScale; period*= df, row += 1 )
    {

        // Multiply the fourier transform of the sound with the fourier
        // transform of the wavelet.
        memset( ans, 0, sizeof(fftwf_complex)*N);
        int start = (unsigned)(0.9 * N / period);
        int end = (unsigned)(1.1 * N / period);

        for( int value = start; value < end; value++ )
            ans[value][0] = FTWavelet( (float)value, (float)period/(N), (float)f0 )*data[value][0];

        // Perform inverse fourier transform of the result.
        fftwf_execute(plan_inverse);

        for (int y = 0; y < voice_y.size(); y++)
        {
            double value = sqrt(ans[y][0]*ans[y][0]+ans[y][1]*ans[y][1]);
            min = qMin(min, value);
            max = qMax(max, value);
            colorMap->data()->setCell(y, (upperScale - lowerScale)*scale - row, (value > freq_noise) ? value : 0);
            local_colorMap->data()->setCell(y, (upperScale - lowerScale)*scale - row, (value > freq_noise) ? value : 0);
        }
        //qDebug() << "Period : "<< period << "; Row : " << row << "; Max = " << max << "; Min = " << min;
        ui->progress_wavelet->setValue((int)(100*(period - lowerScale)/(upperScale - lowerScale)));
    }

    //prepare plot
    colorMap->data()->setRange(QCPRange(0, last_time), QCPRange(70, 1500));
    colorMap->rescaleDataRange(true);
    colorScale->setDataRange(QCPRange(min, max));
    ui->wavelet->rescaleAxes();
    ui->wavelet->replot();

    fftwf_destroy_plan(plan_forward);
    fftwf_destroy_plan(plan_inverse);
    fftwf_free(data);
    fftwf_free(ans);
}
