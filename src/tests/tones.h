#include <QAudioOutput>
#include <QIODevice>
#include <QWidget>

class QFile;

class ToneGenerator : public QIODevice
{
    Q_OBJECT

public:
    ToneGenerator(QObject *parent = 0);
    int clockrate() const;

public slots:
    void startTone(int tone);
    void stopTone(int tone);

protected:
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);

private:
    int m_clockrate;
    int m_clocktick;
    int m_tone;
    int m_tonetick;
};

class ToneGui : public QWidget
{
    Q_OBJECT

public:
    ToneGui();

private slots:
    void keyPressed();
    void keyReleased();

private:
    ToneGenerator *generator;
};

class WavePlayer : public QIODevice
{
    Q_OBJECT

public:
    WavePlayer(const QString &name, QObject *parent = 0);
    QAudioFormat format() const;
    bool open(QIODevice::OpenMode mode);

protected:
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);

private:
    QFile *m_file;
    QAudioFormat m_format;
    qint64 m_beginPos;
    qint64 m_endPos;
};

