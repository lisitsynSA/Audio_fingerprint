#ifndef AUDIOINFO_T_H
#define AUDIOINFO_T_H

#include <QByteArray>
#include <qaudioinput.h>

class audioinfo_t : public QIODevice
{
    Q_OBJECT
public:
    audioinfo_t(const QAudioFormat &init_format, QObject *parent);
    ~audioinfo_t();

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qreal get_level() { return level; }
    qint16 get_value() { return value; }
    void set_int_output_size(quint32 init_size);
    void set_output_size(quint32 init_size);

private:
    const QAudioFormat format;
    quint16 maxAmplitude;
    quint16 value;
    qreal level; // 0.0 <= m_level <= 1.0

    quint32 int_output_size;
    quint16* int_output;
    quint32 output_size;
    qreal* output;

signals:
    void update();
    void int_output_ready(quint16* output);
    void output_ready(qreal* output);
};

#endif // AUDIOINFO_T_H
