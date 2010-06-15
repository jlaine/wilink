#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTime>

/** Fingerprint a file.
 */
static QByteArray hashFile(const QString &path)
{
    QFile fp(path);

    if (!fp.open(QIODevice::ReadOnly) || fp.isSequential())
        return QByteArray();

    QCryptographicHash hasher(QCryptographicHash::Md5);
    char buffer[16384];
    int hashed = 0;
    while (fp.bytesAvailable())
    {
        int len = fp.read(buffer, sizeof(buffer));
        if (len < 0)
            return QByteArray();
        hasher.addData(buffer, len);
        hashed += len;
    }
    return hasher.result();
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        QString path = QString::fromLocal8Bit(argv[i]);
        QFileInfo info(path);
        QTime t;
        t.start();
        QByteArray result = hashFile(path);
        int ms = t.elapsed();
        if (ms > 0)
            qDebug() << info.fileName() << result.toHex() << ms/1000.0 << "s" << (double(info.size()) / (1000.0 * ms)) << "MB/s";
    }
    return 0;
}

