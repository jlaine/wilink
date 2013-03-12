/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

#ifndef __QXMPP_DECLARATIVE_H__
#define __QXMPP_DECLARATIVE_H__

#include "QXmppCallManager.h"
#include "QXmppClient.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMessage.h"
#include "QXmppMucManager.h"
#include "QXmppPresence.h"
#include "QXmppRtpChannel.h"
#include "QXmppRosterManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

class QXmppDeclarativeDataForm : public QObject
{
    Q_OBJECT
    Q_ENUMS(Field)

public:
    enum Field
    {
        BooleanField = QXmppDataForm::Field::BooleanField,
        FixedField = QXmppDataForm::Field::FixedField,
        HiddenField = QXmppDataForm::Field::HiddenField,
        JidMultiField = QXmppDataForm::Field::JidMultiField,
        JidSingleField = QXmppDataForm::Field::JidSingleField,
        ListMultiField = QXmppDataForm::Field::ListMultiField,
        ListSingleField = QXmppDataForm::Field::ListSingleField,
        TextMultiField = QXmppDataForm::Field::TextMultiField,
        TextPrivateField = QXmppDataForm::Field::TextPrivateField,
        TextSingleField = QXmppDataForm::Field::TextSingleField,
    };
};

class QXmppDeclarativeMessage : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    enum State {
        None = QXmppMessage::None,
        Active = QXmppMessage::Active,
        Inactive = QXmppMessage::Inactive,
        Gone = QXmppMessage::Gone,
        Composing = QXmppMessage::Composing,
        Paused = QXmppMessage::Paused,
    };
};

class QXmppDeclarativeMucItem : public QObject
{
    Q_OBJECT
    Q_ENUMS(Affiliation)

public:
    enum Affiliation {
        UnspecifiedAffiliation = QXmppMucItem::UnspecifiedAffiliation,
        OutcastAffiliation = QXmppMucItem::OutcastAffiliation,
        NoAffiliation = QXmppMucItem::NoAffiliation,
        MemberAffiliation = QXmppMucItem::MemberAffiliation,
        AdminAffiliation = QXmppMucItem::AdminAffiliation,
        OwnerAffiliation = QXmppMucItem::OwnerAffiliation,
    };
};

#endif
