#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fftw3.h"
#include <cmath>

float MainWindow::FTWavelet( float value, float scale, float f0 )
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

void MainWindow::run_wavelet()
{
    double scale = ui->spinBox_scale->value();
    /// The smallest scale to render.
    double lowerScale = 20;//20

    /// The largest scale to render.
    double upperScale = 50;//50

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

    int row = 0;
    double max = 0, min = 1000;

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
            colorMap->data()->setCell(y, (upperScale - lowerScale)*scale - row, value);
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
