/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include <cstdlib>
#include <csignal>

#include <QApplication>
#include <QBuffer>
#include <QFile>
#include <QTimer>

#include "QSoundFile.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"
#include "echo.h"

#define L 8

class EchoFilter
{
public:
    EchoFilter();
    void played(qint16 s);
    qint16 recorded(qint16 x);

private:
    double m_step;
    double m_W[L];
    double m_X[L];
    int m_Xpos;
};

EchoFilter::EchoFilter()
    : m_Xpos(-1)
{
    m_step = 1 / 1000000000.0;
    for (int i = 0; i < L; ++i) {
        m_W[i] = 0.0;
        m_X[i] = 0;
    }
}

void EchoFilter::played(qint16 x)
{
    m_Xpos = (m_Xpos + 1) % L;
    m_X[m_Xpos] = x;

#if 0
    double pow = 0;
    for (int k = 0; k < L; ++k)
        pow += m_X[k] * m_X[k];
    qDebug("pow %f", pow/L);
#endif
}

qint16 EchoFilter::recorded(qint16 r)
{
    // estimate echo
    double y = 0;
    for (int k = 0; k < L; ++k) {
        y += m_W[k] * m_X[(m_Xpos - k) % L];
    }
    //qDebug("y %f", y);

    // remove echo
    //const double e = qMax(-32768.0, qMin(double(r) - y, 32767.0));
    const double e = double(r) - y;

    // update filter
    for (int k = 0; k < L; ++k) {
        m_W[k] += m_step * e * m_X[(m_Xpos - k) % L];
        //qDebug("W[%i] %f", k, m_W[k]);
    }

    // return filtered value
    return e;
}

EchoTester::EchoTester(QObject *parent)
    : QObject(parent)
{
    QList<QPair<QSoundFile::MetaData, QString> > metaData;
    metaData << qMakePair(QSoundFile::ArtistMetaData, QString::fromLatin1("TestArtist"));
    metaData << qMakePair(QSoundFile::AlbumMetaData, QString::fromLatin1("TestAlbum"));
    metaData << qMakePair(QSoundFile::TitleMetaData, QString::fromLatin1("TestTitle"));

    m_file = new QSoundFile("recorded.wav", this);

    m_player = new QSoundPlayer(this);
    m_stream = new QSoundStream(m_player);
    m_stream->setFormat(1, 8000);

    m_file->setFormat(m_stream->format());
    m_file->setMetaData(metaData);
    m_file->open(QIODevice::WriteOnly);

    m_stream->setDevice(m_file);
    m_stream->startInput();

    QTimer::singleShot(5000, qApp, SLOT(quit()));
}

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

int main(int argc, char *argv[])
{
    bool check;

    QApplication app(argc, argv);
    /* Install signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

#if 0
    EchoTester tester;
    return app.exec();
#endif

    QSoundFile local(":/local.ogg");
    check = local.open(QIODevice::ReadOnly);
    Q_ASSERT(check);
    QDataStream localStream(&local);
    localStream.setByteOrder(QDataStream::LittleEndian);

    QSoundFile remote(":/remote.ogg");
    check = remote.open(QIODevice::ReadOnly);
    Q_ASSERT(check);
    QDataStream remoteStream(&remote);
    remoteStream.setByteOrder(QDataStream::LittleEndian);

    Q_ASSERT(local.format() == remote.format());

    // simulate echo
    const size_t N = local.format().frequency() * 4;

    QSoundFile buffer("mixed.wav");
    buffer.setFormat(local.format());
    check = buffer.open(QBuffer::WriteOnly);
    Q_ASSERT(check);
    QDataStream bufferStream(&buffer);
    bufferStream.setByteOrder(QDataStream::LittleEndian);
    qint16 val = 0;
    const int delay = 0;
    for (int i = 0; i < N; ++i) {
        localStream >> val;
        if (i >= delay) {
            qint16 echo;
            remoteStream >> echo;
            val += (echo / 2);
        }
        bufferStream << val;
    }
    buffer.close();
    buffer.open(QIODevice::ReadOnly);
    remote.close();
    remote.open(QIODevice::ReadOnly);

    // cancel echo
    QSoundFile output("fixed.wav");
    output.setFormat(local.format());
    check = output.open(QIODevice::WriteOnly);
    Q_ASSERT(check);
    QDataStream outputStream(&output);
    outputStream.setByteOrder(QDataStream::LittleEndian);

    EchoFilter filter;
    for (int i = 0; i < N; ++i) {
        remoteStream >> val;
        filter.played(val);
        bufferStream >> val;
        val = filter.recorded(val);
        outputStream << val;
    }

    return 0;
}

#if 0
    #include <fftw3.h>

    // allocate memory
    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *played_fft = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *recorded_fft = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *auto_correlation = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *cross_correlation = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_plan p;

    // FFT of recorded signal
    p = fftw_plan_dft_1d(N, in, recorded_fft, FFTW_FORWARD, FFTW_ESTIMATE);
    QDataStream recorded_stream(&recorded);
    recorded_stream.setByteOrder(QDataStream::LittleEndian);
    qint16 val;
    for (int i = 0; i < N; ++i) {
        recorded_stream >> val;
        in[i][0] = (i < real_N) ? val : 0;
        in[i][1] = 0;
        //qDebug("%i %f", i, in[i][0] * in[i][0]);
    }
    fftw_execute(p);
    fftw_destroy_plan(p);

    // FFT of played signal
    p = fftw_plan_dft_1d(N, in, played_fft, FFTW_FORWARD, FFTW_ESTIMATE);
    QDataStream played_stream(&played);
    played_stream.setByteOrder(QDataStream::LittleEndian);
    for (int i = 0; i < N; ++i) {
        played_stream >> val;
        in[i][0] = (i < real_N) ? val : 0;
        in[i][1] = 0;
    }
    fftw_execute(p);
    fftw_destroy_plan(p);

    // auto correlation
    p = fftw_plan_dft_1d(N, in, auto_correlation, FFTW_BACKWARD, FFTW_ESTIMATE);
    for (int i = 0; i < N; ++i) {
        in[i][0] = played_fft[i][0] * played_fft[i][0] + played_fft[i][1] * played_fft[i][1];
        in[i][1] = 0;
    }
    fftw_execute(p);
    fftw_destroy_plan(p);

    // cross-correlation
    p = fftw_plan_dft_1d(N, in, cross_correlation, FFTW_BACKWARD, FFTW_ESTIMATE);
    for (int i = 0; i < N; ++i) {
        in[i][0] = recorded_fft[i][0] * played_fft[i][0] + recorded_fft[i][1] * played_fft[i][1];
        in[i][1] = -recorded_fft[i][0] * played_fft[i][1] + recorded_fft[i][1] * played_fft[i][0];
    }
    fftw_execute(p);
    fftw_destroy_plan(p);

    int max_pos = -1;
    double max_val = 0;
    for (int i = 0; i < N; ++i) {
        const double corr = cross_correlation[i][0] / N;
        //qDebug("%f", corr);
        if (max_pos < 0 || corr > max_val) {
            max_pos = i;
            max_val = corr;
        }
    }
    if (max_pos > N/2)
        max_pos = max_pos - N;
    qDebug("auto correlation %f", cross_correlation[0][0] / N);
    qDebug("max correlation %i: %f", max_pos, max_val);

    fftw_free(auto_correlation);
    fftw_free(cross_correlation);
    fftw_free(played_fft);
    fftw_free(recorded_fft);
    fftw_free(in);

    // cancel echo
    played.close();
    played.open(QIODevice::ReadOnly);

    recorded.close();
    recorded.open(QIODevice::ReadOnly);

    QFile file("fixed.raw");
    file.open(QIODevice::WriteOnly);

    QDataStream output_stream(&file);
    output_stream.setByteOrder(QDataStream::LittleEndian);
    qint16 echo;
    for (int i = 0; i < real_N; ++i) {
        recorded_stream >> val;
        if (i >= max_pos) {
            played_stream >> echo;
            val -= echo;
        }
        output_stream << val;
    }
#endif

