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

#include <QAudioInput>
#include <QAudioOutput>
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>

#include "QSoundMeter.h"

#include "application.h"
#include "preferences.h"

static QLayout *aboutBox()
{
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/wiLink-64.png"));
    hbox->addWidget(icon);
    hbox->addSpacing(20);
    hbox->addWidget(new QLabel(QString("<p style=\"font-size: xx-large;\">%1</p>"
        "<p style=\"font-size: large;\">%2</p>")
        .arg(qApp->applicationName(),
            QString("version %1").arg(qApp->applicationVersion()))));
    return hbox;
}

class ChatPreferencesList : public QListWidget
{
public:
    QSize sizeHint() const;
};

QSize ChatPreferencesList::sizeHint() const
{
    QSize hint(150, minimumHeight());
    int rowCount = model()->rowCount();
    int rowHeight = rowCount * sizeHintForRow(0);
    if (rowHeight > hint.height())
        hint.setHeight(rowHeight);
    return hint;
}

class ChatPreferencesPrivate
{
public:
    QListWidget *tabList;
    QStackedWidget *tabStack;
};

/** Constructs a new preferences dialog.
 *
 * @param parent
 */
ChatPreferences::ChatPreferences(QWidget *parent)
    : QDialog(parent),
      d(new ChatPreferencesPrivate)
{
    QVBoxLayout *layout = new QVBoxLayout;

    // splitter
    QSplitter *splitter = new QSplitter;
    
    d->tabList = new ChatPreferencesList;
    d->tabList->setIconSize(QSize(32, 32));
    splitter->addWidget(d->tabList);
    splitter->setStretchFactor(0, 0);

    d->tabStack = new QStackedWidget;
    splitter->addWidget(d->tabStack);
    splitter->setStretchFactor(1, 1);

    connect(d->tabList, SIGNAL(currentRowChanged(int)), d->tabStack, SLOT(setCurrentIndex(int)));
    layout->addWidget(splitter);

    // buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(validate()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);

    setLayout(layout);
    setWindowTitle(tr("Preferences"));
}

/** Destroys a preferences dialog.
 */
ChatPreferences::~ChatPreferences()
{
    delete d;
}

void ChatPreferences::addTab(ChatPreferencesTab *tab)
{
    QListWidgetItem *item = new QListWidgetItem(tab->windowIcon(), tab->windowTitle());
    d->tabList->addItem(item);
    d->tabStack->addWidget(tab);
}

void ChatPreferences::setCurrentTab(const QString &tabName)
{
    for (int i = 0; i < d->tabStack->count(); ++i) {
        if (d->tabStack->widget(i)->objectName() == tabName) {
            d->tabList->setCurrentRow(i);
            break;
        }
    }
}

/** Validates and applies the new preferences.
 */
void ChatPreferences::validate()
{
    for (int i = 0; i < d->tabStack->count(); ++i) {
        ChatPreferencesTab *tab = qobject_cast<ChatPreferencesTab*>(d->tabStack->widget(i));
        tab->save();
    }
    accept();
}

ChatPreferencesTab::ChatPreferencesTab()
{
}

ChatOptions::ChatOptions()
{
    QVBoxLayout *layout = new QVBoxLayout;

    // GENERAL OPTIONS
    QGroupBox *group = new QGroupBox(tr("General options"));
    QVBoxLayout *vbox = new QVBoxLayout;
    group->setLayout(vbox);
    layout->addWidget(group);

    if (wApp->isInstalled())
    {
        openAtLogin = new QCheckBox(tr("Open at login"));
        openAtLogin->setChecked(wApp->openAtLogin());
        vbox->addWidget(openAtLogin);
    } else {
        openAtLogin = 0;
    }

    showOfflineContacts = new QCheckBox(tr("Show offline contacts"));
    showOfflineContacts->setChecked(wApp->showOfflineContacts());
    vbox->addWidget(showOfflineContacts);

    // ABOUT
    group = new QGroupBox(tr("About %1").arg(wApp->applicationName()));
    group->setLayout(aboutBox());
    layout->addWidget(group);

    setLayout(layout);
    setWindowIcon(QIcon(":/options.png"));
    setWindowTitle(tr("General"));
}

bool ChatOptions::save()
{
    if (openAtLogin)
        wApp->setOpenAtLogin(openAtLogin->isChecked());
    wApp->setShowOfflineContacts(showOfflineContacts->isChecked());
    return true;
}

#define SOUND_TEST_SECONDS 5

SoundOptions::SoundOptions()
{
    QVBoxLayout *layout = new QVBoxLayout;

    // DEVICES
    QGroupBox *group = new QGroupBox(tr("Sound devices"));
    QGridLayout *devicesLayout = new QGridLayout;
    devicesLayout->setColumnStretch(2, 1);
    group->setLayout(devicesLayout);
    layout->addWidget(group);

    // output
    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/audio-output.png"));
    devicesLayout->addWidget(label, 0, 0);
    devicesLayout->addWidget(new QLabel(tr("Audio playback device")), 0, 1);
    outputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    outputCombo = new QComboBox;
    foreach (const QAudioDeviceInfo &info, outputDevices) {
        outputCombo->addItem(info.deviceName());
        if (info.deviceName() == wApp->audioOutputDevice().deviceName())
            outputCombo->setCurrentIndex(outputCombo->count() - 1);
    }
    devicesLayout->addWidget(outputCombo, 0, 2);

    // input
    label = new QLabel;
    label->setPixmap(QPixmap(":/audio-input.png"));
    devicesLayout->addWidget(label, 1, 0);
    devicesLayout->addWidget(new QLabel(tr("Audio capture device")), 1, 1);
    inputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    inputCombo = new QComboBox;
    foreach (const QAudioDeviceInfo &info, inputDevices) {
        inputCombo->addItem(info.deviceName());
        if (info.deviceName() == wApp->audioInputDevice().deviceName())
            inputCombo->setCurrentIndex(inputCombo->count() - 1);
    }
    devicesLayout->addWidget(inputCombo, 1, 2);

    // test
    testBuffer = new QBuffer(this);
    testBar = new QSoundMeterBar;
    testBar->hide();
    devicesLayout->addWidget(testBar, 2, 0, 1, 3);
    testLabel = new QLabel;
    testLabel->setWordWrap(true);
    devicesLayout->addWidget(testLabel, 3, 0, 1, 2);
    testButton = new QPushButton(tr("Test"));
    connect(testButton, SIGNAL(clicked()), this, SLOT(startInput()));
    devicesLayout->addWidget(testButton, 3, 2);

    // NOTIFICATIONS
    group = new QGroupBox(tr("Sound notifications"));
    QVBoxLayout *notificationsLayout = new QVBoxLayout;
    group->setLayout(notificationsLayout);
    layout->addWidget(group);

    incomingMessageSound = new QCheckBox(tr("Incoming message"));
    incomingMessageSound->setChecked(!wApp->incomingMessageSound().isEmpty());
    notificationsLayout->addWidget(incomingMessageSound);

    outgoingMessageSound = new QCheckBox(tr("Outgoing message"));
    outgoingMessageSound->setChecked(!wApp->outgoingMessageSound().isEmpty());
    notificationsLayout->addWidget(outgoingMessageSound);

    setLayout(layout);
    setWindowIcon(QIcon(":/audio-output.png"));
    setWindowTitle(tr("Sound"));
}

bool SoundOptions::save()
{
    // devices
    if (!inputDevices.isEmpty())
        wApp->setAudioInputDevice(inputDevices[inputCombo->currentIndex()]);
    if (!outputDevices.isEmpty())
        wApp->setAudioOutputDevice(outputDevices[outputCombo->currentIndex()]);

    // notifications
    wApp->setIncomingMessageSound(
        incomingMessageSound->isChecked() ? QLatin1String(":/message-incoming.ogg") : QString());
    wApp->setOutgoingMessageSound(
        outgoingMessageSound->isChecked() ? QLatin1String(":/message-outgoing.ogg") : QString());
    return true;
}

void SoundOptions::startInput()
{
    if (inputDevices.isEmpty() || outputDevices.isEmpty())
        return;
    testButton->setEnabled(false);

    QAudioFormat format;
    format.setFrequency(8000);
    format.setChannels(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // create audio input and output
#ifdef Q_OS_MAC
    // 128ms at 8kHz
    const int bufferSize = 2048 * format.channels();
#else
    // 160ms at 8kHz
    const int bufferSize = 2560 * format.channels();
#endif
    testInput = new QAudioInput(inputDevices[inputCombo->currentIndex()], format, this);
    testInput->setBufferSize(bufferSize);
    testOutput = new QAudioOutput(outputDevices[outputCombo->currentIndex()], format, this);
    testOutput->setBufferSize(bufferSize);

    // start input
    testBar->show();
    testLabel->setText(tr("Speak into the microphone for %1 seconds and check the sound level.").arg(QString::number(SOUND_TEST_SECONDS)));
    testBuffer->open(QIODevice::WriteOnly);
    testBuffer->reset();
    QSoundMeter *inputMeter = new QSoundMeter(testInput->format(), testBuffer, testInput);
    connect(inputMeter, SIGNAL(valueChanged(int)), testBar, SLOT(setValue(int)));
    testInput->start(inputMeter);
    QTimer::singleShot(SOUND_TEST_SECONDS * 1000, this, SLOT(startOutput()));
}

void SoundOptions::startOutput()
{
    testInput->stop();
    testBar->setValue(0);

    // start output
    testLabel->setText(tr("You should now hear the sound you recorded."));
    testBuffer->open(QIODevice::ReadOnly);
    testBuffer->reset();
    QSoundMeter *outputMeter = new QSoundMeter(testOutput->format(), testBuffer, testOutput);
    connect(outputMeter, SIGNAL(valueChanged(int)), testBar, SLOT(setValue(int)));
    testOutput->start(outputMeter);
    QTimer::singleShot(SOUND_TEST_SECONDS * 1000, this, SLOT(stopOutput()));
}

void SoundOptions::stopOutput()
{
    testOutput->stop();
    testBar->setValue(0);
    testBar->hide();

    // cleanup
    delete testInput;
    delete testOutput;
    testLabel->setText(QString());
    testButton->setEnabled(true);
}

