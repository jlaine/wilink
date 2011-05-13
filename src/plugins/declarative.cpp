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

#include <QDeclarativeItem>
#include <QDeclarativeEngine>

#include "chat_plugin.h"
#include "declarative.h"

ListHelper::ListHelper(QObject *parent)
    : QObject(parent),
    m_model(0)
{
}

int ListHelper::count() const
{
    if (m_model)
        return m_model->rowCount();
    return 0;
}

QModelIndex ListHelper::get(int row) const
{
    return QModelIndex();
}

QAbstractItemModel *ListHelper::model() const
{
    return m_model;
}

void ListHelper::setModel(QAbstractItemModel *model)
{
    if (model != m_model) {
        m_model = model;
        emit modelChanged(model);
    }
}

// PLUGIN

class DeclarativePlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Declarative interface"; };
};

bool DeclarativePlugin::initialize(Chat *chat)
{
    qmlRegisterType<ListHelper>("wiLink", 1, 2, "ListHelper");
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(declarative, DeclarativePlugin)

