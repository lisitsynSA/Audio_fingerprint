#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QTime>
#include "audioinfo_t.h"

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
    void print_output_csv(quint16* output);
    void start_button();
    void start_wavelet();

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
    QTime timer;

    quint16 max_y;
    double last_time;
    QVector<double> x, y;
};

#endif // MAINWINDOW_H
