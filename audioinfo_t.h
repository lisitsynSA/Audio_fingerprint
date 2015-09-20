#ifndef AUDIOINFO_T_H
#define AUDIOINFO_T_H

#include <QByteArray>
#include <qaudioinput.h>

class audioinfo_t : public QIODevice
{
    Q_OBJECT
public:
    audioinfo_t(const QAudioFormat &format, QObject *parent);
    ~audioinfo_t();

    void start();
    void stop();

    qreal level() const { return m_level; }

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qreal get_level() {return m_level;}

private:
    const QAudioFormat m_format;
    quint16 m_maxAmplitude;
    qreal m_level; // 0.0 <= m_level <= 1.0

signals:
    void update();
};

#endif // AUDIOINFO_T_H
