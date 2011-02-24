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

#include <QAudioOutput>
#include <QIODevice>
#include <QWidget>

class QComboBox;
class QSoundPlayer;

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
    void startSound();
    void stopSound();

private:
    ToneGenerator *generator;
    QSoundPlayer *player;
    QList<int> soundIds;
    QComboBox *soundCombo;
};

