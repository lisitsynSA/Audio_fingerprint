#ifndef WAVE_T_H
#define WAVE_T_H

#include "qcustomplot.h"
#include <QObject>
#include <QTime>

namespace Ui {
class MainWindow;
}

class wave_t : public QObject
{
    Q_OBJECT

public:
    wave_t(Ui::MainWindow *init_ui, QCPColorMap* init_colorMap,\
           QCPColorScale* init_colorScale, QObject *parent);

    Ui::MainWindow *ui;

    quint16 max_y;
    QCPColorMap* colorMap;
    QCPColorMap* local_colorMap;
    QCPColorScale* colorScale;
    QVector<double> voice_x, voice_y;
    QTime timer;
    double last_time;

    void start();
    void get_output(quint16 *output, int bundle_size, int output_size);
    void print_output_csv(QString file_name);
    void read_wave(QString file_name);
    void write_wave(QString file_name);
    void load_wave();

    double scale;
    double freq_noise;
    double lowerScale;
    double upperScale;
    double max;
    double min;

    //WAVELET:
    float FTWavelet( float value, float scale, float f0 );
    void run_wavelet();
    void wavelet_analysis();
public slots:
    void wave_clearing();
};

#endif // WAVE_T_H
