#include "audioinfo_t.h"

#include <stdlib.h>
#include <math.h>
#include <QtCore/qendian.h>
#include <QDebug>

audioinfo_t::audioinfo_t(const QAudioFormat &init_format, QObject *parent):
    QIODevice(parent),
    format(init_format),
    maxAmplitude(32767),
    value(0),
    level(0.0),
    int_output_size(0),
    int_output(NULL),
    output_size(0),
    output(NULL)
{

}

audioinfo_t::~audioinfo_t()
{
}

void audioinfo_t::start()
{
    open(QIODevice::WriteOnly);
}

void audioinfo_t::stop()
{
    close();
}

qint64 audioinfo_t::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data)
    Q_UNUSED(maxlen)

    return 0;
}

void audioinfo_t::set_int_output_size(quint32 init_size)
{
    int_output_size = init_size;
    int_output = new quint16[init_size];
}
void audioinfo_t::set_output_size(quint32 init_size)
{
    output_size = init_size;
    output = new qreal[init_size];
}

qint64 audioinfo_t::writeData(const char *data, qint64 len)
{
    if (maxAmplitude) {
        Q_ASSERT(format.sampleSize() % 8 == 0);
        const int channelBytes = format.sampleSize() / 8;
        const int sampleBytes = format.channels() * channelBytes;
        Q_ASSERT(len % sampleBytes == 0);
        const int numSamples = len / sampleBytes;

        quint16 maxValue = 0;
        const unsigned char *ptr = reinterpret_cast<const unsigned char *>(data);

        for (int i = 0; i < numSamples; ++i) {
            /*format.setChannels(1);
            format.setSampleSize(16);
            format.setSampleType(QAudioFormat::SignedInt);
            format.setByteOrder(QAudioFormat::LittleEndian);*/
            value = qAbs(qFromLittleEndian<qint16>(ptr));
            if (int_output_size)
            {
                int_output_size--;
                int_output[int_output_size] = value;
                if (!int_output_size)
                    emit int_output_ready(int_output);
            }
            maxValue = qMax(value, maxValue);
            ptr += channelBytes;
        }

        maxValue = qMin(maxValue, maxAmplitude);
        level = qreal(maxValue) / maxAmplitude;
        if (output_size)
        {
            output_size--;
            output[output_size] = level*100;
            if (!output_size)
                emit output_ready(output);
        }
    }

    emit update();
    return len;
}
