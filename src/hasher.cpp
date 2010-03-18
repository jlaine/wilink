#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QTime>

#define HASH_BLOCK_SIZE 16384

int main(int argc, char *argv[])
{
    char buffer[HASH_BLOCK_SIZE];
    QCryptographicHash hasher(QCryptographicHash::Md5);
    QTime t;

    for (int i = 1; i < argc; i++)
    {
        const QString filePath(argv[i]);
        int len;

        t.start();
        QFile fp(filePath);
        fp.open(QIODevice::ReadOnly);
        while ((len = fp.read(buffer, HASH_BLOCK_SIZE)) > 0)
            hasher.addData(buffer, len);
        fp.close();
        qDebug() << "hashed" << filePath << hasher.result().toHex() << "in" << t.elapsed();
    }
    return 0;
}

