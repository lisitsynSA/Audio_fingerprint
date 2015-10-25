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
    wave_t(Ui::MainWindow *init_ui, QObject *parent);

    Ui::MainWindow *ui;

    quint16 max_y;
    QCPColorMap* colorMap;
    QCPColorScale *colorScale;
    QVector<double> voice_x, voice_y;
    QTime timer;
    double last_time;

    void start();
    void get_output(quint16 *output, int bundle_size, int output_size);
    void print_output_csv(QString file_name);
    void read_wave(QString file_name);
    void write_wave(QString file_name);

    //WAVELET:
    float FTWavelet( float value, float scale, float f0 );
    void run_wavelet();
public slots:
    void wave_clearing();
};

#endif // WAVE_T_H
