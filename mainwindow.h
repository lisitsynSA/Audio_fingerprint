#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QDebug>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include "audioinfo_t.h"
#include "qcustomplot.h"
#include "wave_t.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void initializeWindow();
    void initializeAudio();
    void createAudioInput();
    void continue_get_output();

private slots:
    void refreshDisplay();
    void notified();
    void readMore();
    void deviceChanged(int index);
    void get_output(quint16* output);
    void start_button();
    void start_wavelet();
    void wave_clearing();
    void do_button();

private:
    Ui::MainWindow *ui;
    audioinfo_t *audioinfo;
    QIODevice *device;
    QAudioDeviceInfo device_info;
    QByteArray m_buffer;
    QAudioFormat format;
    QAudioInput *input;

    int output_size;
    int bundle_size;
    QVector<wave_t*> waves;
    int current_wave;
};

#endif // MAINWINDOW_H
