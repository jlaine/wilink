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

#include <QAction>
#include <QCoreApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDomDocument>
#include <QLayout>
#include <QMessageBox>
#include <QTimer>
#include <QUrl>

#include "QXmppUtils.h"

#include "application.h"
#include "chat.h"

#include "phone/models.h"
#include "phone/sip.h"
#include "phone.h"
 
PhonePanel::PhonePanel(Chat *chatWindow, QWidget *parent)
    : ChatPanel(parent),
    m_callsModel(0),
    m_window(chatWindow)
{
    bool check;

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // declarative
    declarativeView = new QDeclarativeView;
    QDeclarativeContext *context = declarativeView->rootContext();
    context->setContextProperty("window", m_window);

    declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    declarativeView->setSource(QUrl("qrc:/PhonePanel.qml"));

    QObject *obj = declarativeView->rootObject()->property("historyModel").value<QObject*>();
    m_callsModel = qobject_cast<PhoneCallsModel*>(obj);
    Q_ASSERT(m_callsModel);
    layout->addWidget(declarativeView, 1);

    setFocusProxy(declarativeView);

    // connect signals
    check = connect(declarativeView->rootObject(), SIGNAL(close()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    check = connect(m_callsModel->client(), SIGNAL(callReceived(SipCall*)),
                    this, SLOT(callReceived(SipCall*)));
    Q_ASSERT(check);

    check = connect(m_callsModel->client(), SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                    m_window->client(), SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    Q_ASSERT(check);

    check = connect(m_window->client(), SIGNAL(connected()),
                    m_callsModel, SLOT(getSettings()));
    Q_ASSERT(check);
}

void PhonePanel::callButtonClicked(QAbstractButton *button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    SipCall *call = qobject_cast<SipCall*>(box->property("call").value<QObject*>());
    if (!call)
        return;

    if (box->standardButton(button) == QMessageBox::Yes)
        QMetaObject::invokeMethod(call, "accept");
    else
        QMetaObject::invokeMethod(call, "hangup");
    box->deleteLater();
}

void PhonePanel::callReceived(SipCall *call)
{
    m_callsModel->addCall(call);
    const QString contactName = sipAddressToName(call->recipient());

    QMessageBox *box = new QMessageBox(QMessageBox::Question,
        tr("Call from %1").arg(contactName),
        tr("%1 wants to talk to you.\n\nDo you accept?").arg(contactName),
        QMessageBox::Yes | QMessageBox::No, m_window);
    box->setDefaultButton(QMessageBox::NoButton);
    box->setProperty("call", qVariantFromValue(qobject_cast<QObject*>(call)));

    /* connect signals */
    connect(call, SIGNAL(destroyed(QObject*)), box, SLOT(deleteLater()));
    connect(box, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(callButtonClicked(QAbstractButton*)));
    box->show();
}

