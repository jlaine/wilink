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

#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QTimer>

#include "chat_tip.h"

/** Show a tip bubble attached to a widget.
 */
ChatTip::ChatTip(QWidget *widget, const QString &text)
    : m_widget(widget)
{
    QPoint anchor = m_widget->mapToGlobal(QPoint());

    /* draw halo */
    m_halo = new QWidget;
    m_halo->setAttribute(Qt::WA_TranslucentBackground, true);
    m_halo->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    QFrame *haloframe = new QFrame; //Label(text);
    haloframe->setStyleSheet(
        "QFrame {\n"
        "  background: transparent;"
        "  border: 1px solid red;"
        "}");
    layout->addWidget(haloframe);
    m_halo->setLayout(layout);
    m_halo->resize(m_widget->size());
    m_halo->move(anchor);
    m_halo->show();

    /* draw tip */
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    layout = new QVBoxLayout;
    layout->setMargin(0);
    QLabel *tip = new QLabel(text);
    tip->setStyleSheet(
        "QLabel {\n"
        "  background: white;"
        "  border: 1px solid red;"
        "  border-radius: 10px;"
        "  padding: 5px;"
        "  opacity: 200;"
        "}");
    tip->setWordWrap(true);
    layout->addWidget(tip);
    setLayout(layout);
    move(anchor);
    show();

    /* register */
    m_widget->installEventFilter(this);
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
}

ChatTip::~ChatTip()
{
    delete m_halo;
}

bool ChatTip::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize)
    {
        QPoint anchor = m_widget->mapToGlobal(QPoint());
        m_halo->resize(m_widget->size());
        m_halo->move(anchor);
    }
    return false;
}

