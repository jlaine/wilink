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

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>

#include "chat_form.h"

ChatForm::ChatForm(const QXmppDataForm &form, QWidget *parent)
    : QDialog(parent), chatForm(form)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    foreach (const QXmppDataForm::Field &field, chatForm.fields())
    {
        const QString key = field.key();
        if (key.isEmpty())
            continue;

        if (field.type() == QXmppDataForm::Field::BooleanField)
        {
            QCheckBox *checkbox = new QCheckBox(field.label());
            checkbox->setChecked(field.value().toBool());
            checkbox->setObjectName(key);
            vbox->addWidget(checkbox);
        }
        else if (field.type() == QXmppDataForm::Field::TextSingleField)
        {
            QHBoxLayout *hbox = new QHBoxLayout;
            hbox->addWidget(new QLabel(field.label()));
            QLineEdit *edit = new QLineEdit(field.value().toString());
            edit->setObjectName(key);
            hbox->addWidget(edit);
            vbox->addItem(hbox);
        }
        else if (field.type() == QXmppDataForm::Field::ListSingleField)
        {
            QHBoxLayout *hbox = new QHBoxLayout;
            hbox->addWidget(new QLabel(field.label()));
            QComboBox *combo = new QComboBox;
            combo->setObjectName(key);
            int currentIndex = 0;
            QList<QPair<QString,QString> > options = field.options();
            for (int i = 0; i < options.size(); i++)
            {
                combo->addItem(options[i].first, options[i].second);
                if (options[i].first == field.value().toString())
                    currentIndex = i;
            }
            combo->setCurrentIndex(currentIndex);
            hbox->addWidget(combo);
            vbox->addItem(hbox);
        }
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vbox->addWidget(buttonBox);

    setLayout(vbox);
    setWindowTitle(chatForm.title());

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ChatForm::submit()
{
    chatForm.setType(QXmppDataForm::Submit);
    for (int i = 0; i < chatForm.fields().size(); i++)
    {
        QXmppDataForm::Field &field = chatForm.fields()[i];
        const QString key = field.key();
        if (key.isEmpty())
            continue;

        if (field.type() == QXmppDataForm::Field::BooleanField)
        {
            QCheckBox *checkbox = findChild<QCheckBox*>(key);
            field.setValue(checkbox->checkState() == Qt::Checked);
        }
        else if (field.type() == QXmppDataForm::Field::TextSingleField)
        {
            QLineEdit *edit = findChild<QLineEdit*>(key);
            field.setValue(edit->text());
        }
        else if (field.type() == QXmppDataForm::Field::ListSingleField)
        {
            QComboBox *combo = findChild<QComboBox*>(key);
            field.setValue(combo->itemData(combo->currentIndex()).toString());
        }
    }
    accept();
}

QXmppDataForm ChatForm::form() const
{
    return chatForm;
}
