/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __WILINK_DECLARATIVE_H__
#define __WILINK_DECLARATIVE_H__

#include <QAbstractItemModel>
#include <QDeclarativeItem>
#include <QDeclarativeNetworkAccessManagerFactory>
#include <QDeclarativeExtensionPlugin>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QSortFilterProxyModel>

#include "QXmppMessage.h"
#include "QXmppPresence.h"

class QDeclarativeFileDialog : public QFileDialog
{
    Q_OBJECT
    Q_PROPERTY(QString directory READ directory WRITE setDirectory)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters)
    Q_PROPERTY(QStringList selectedFiles READ selectedFiles)

public:
    QDeclarativeFileDialog(QWidget *parent = 0) : QFileDialog(parent) {}

    QString directory() const;
    void setDirectory(const QString &directory);
};

class QDeclarativeSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)

public:
    QDeclarativeSortFilterProxyModel(QObject *parent = 0);
    void setSourceModel(QAbstractItemModel *model);

public slots:
    void sort(int column);

signals:
    void sourceModelChanged(QAbstractItemModel *sourceModel);
};

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

class QXmppDeclarativePresence : public QObject
{
    Q_OBJECT
    Q_ENUMS(Status)

public:
    enum Status {
        Offline = QXmppPresence::Status::Offline,
        Online = QXmppPresence::Status::Online,
        Away = QXmppPresence::Status::Away,
        XA = QXmppPresence::Status::XA,
        DND = QXmppPresence::Status::DND,
        Chat = QXmppPresence::Status::Chat,
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

class ListHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
    ListHelper(QObject *parent = 0);

    int count() const;
    Q_INVOKABLE QVariant get(int row) const;
    Q_INVOKABLE QVariant getProperty(int row, const QString &name) const;

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

signals:
    void countChanged();
    void modelChanged(QAbstractItemModel *model);

private:
    QAbstractItemModel *m_model;
    QList<QPersistentModelIndex> m_selection;
};

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    NetworkAccessManager(QObject *parent = 0);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

private slots:
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
};

class NetworkAccessManagerFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject * parent)
    {
        return new NetworkAccessManager(parent);
    }
};

class DropArea : public QDeclarativeItem
{
    Q_OBJECT

public:
    DropArea(QDeclarativeItem *parent = 0);

signals:
    void filesDropped(const QStringList &files);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);
};

class Plugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void initializeEngine(QDeclarativeEngine *engine, const char *uri);
    void registerTypes(const char *uri);
};

#endif
