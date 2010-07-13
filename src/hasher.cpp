/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTime>

#include "qxmpp/QXmppUtils.h"

static const quint32 MAGIC_COOKIE = 0x2112A442;

enum MessageType {
    BindingRequest  = 0x0001,
    BindingResponse = 0x0101,
    BindingError    = 0x0111,
    SharedSecretRequest  = 0x0002,
    SharedSecretResponse = 0x0102,
    SharedSecretError    = 0x0112,
};

enum AttributeType {
    Username         = 0x0006,
    MessageIntegrity = 0x0008,
    Priority         = 0x0024,
    Unknown          = 0x0029,
    Fingerprint      = 0x8028,
};

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

void stunHash()
{
    QString m_localUser = "XiSh";
    QString m_remoteUser = "B23U";
    QString m_remotePassword = "EXlVjyYgSRcC4OAxMSYDFF";

    QByteArray username = QString("%1:%2").arg(m_remoteUser, m_localUser).toUtf8();
    quint16 usernameSize = username.size();
    username += QByteArray::fromHex("e1c318");
//    username += QByteArray(4 * ((usernameSize + 3) / 4) - usernameSize, 0);
    quint32 priority = 1862270975;
    QByteArray unknown(8, 0);
    QByteArray key = m_remotePassword.toUtf8();

    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    quint16 type = BindingRequest;
    quint16 length = 0;
    QByteArray id = QByteArray::fromHex("93be62deb6e5418f9f87b68b");
    stream << type;
    stream << length;
    stream << MAGIC_COOKIE;
    stream.writeRawData(id.data(), id.size());

    stream << quint16(Priority);
    stream << quint16(sizeof(priority));
    stream << priority;

    stream << quint16(Unknown);
    stream << quint16(unknown.size());
    stream.writeRawData(unknown.data(), unknown.size());

    stream << quint16(Username);
    stream << usernameSize;
    stream.writeRawData(username.data(), username.size());

    // integrity
    length = buffer.size() - 20 + 24;
    qint64 pos = stream.device()->pos();
    stream.device()->seek(2);
    stream << length;
    stream.device()->seek(pos);

    QByteArray integrity = generateHmacSha1(key, buffer);
    qDebug() << "integrity" << integrity.toHex();
    stream << quint16(MessageIntegrity);
    stream << quint16(integrity.size());
    stream.writeRawData(integrity.data(), integrity.size());

    // fingerprint
    length = buffer.size() - 20 + 8;
    pos = stream.device()->pos();
    stream.device()->seek(2);
    stream << length;
    stream.device()->seek(pos);

    quint32 fingerprint = generateCrc32(buffer) ^ 0x5354554eL;
    qDebug() << "fingerprint" << fingerprint;
    stream << quint16(Fingerprint);
    stream << quint16(sizeof(fingerprint));
    stream << fingerprint;

    // output
    buffer.prepend(QByteArray(10, 0));
    for (int i = 0; i < buffer.size(); i += 16)
    {
        QByteArray chunk = buffer.mid(i, 16);
        QString str;
        for (int j = 0; j < 16; j += 1)
            str += chunk.mid(j, 1).toHex() + " ";
        qDebug() << str;
    }

}

int main(int argc, char *argv[])
{
    stunHash();

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

