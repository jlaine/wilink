/*
 * wDesktop
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
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>

#include "chat_form.h"

ChatForm::ChatForm(const QXmppElement &form, QWidget *parent)
    : QDialog(parent), chatForm(form)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    QXmppElement field = chatForm.firstChildElement("field");
    while (!field.isNull())
    {
        const QString key = field.attribute("var");
        if (!key.isEmpty())
        {
            const QString label = field.attribute("label");
            const QString value = field.firstChildElement("value").value();
            if (field.attribute("type") == "boolean")
            {
                QCheckBox *checkbox = new QCheckBox(label);
                checkbox->setChecked(value == "1");
                checkbox->setObjectName(key);
                vbox->addWidget(checkbox);
            } else if (field.attribute("type") == "text-single") {
                QHBoxLayout *hbox = new QHBoxLayout;
                hbox->addWidget(new QLabel(label));
                QLineEdit *edit = new QLineEdit(value);
                edit->setObjectName(key);
                hbox->addWidget(edit);
                vbox->addItem(hbox);
            } else if (field.attribute("type") == "list-single") {
                QHBoxLayout *hbox = new QHBoxLayout;
                hbox->addWidget(new QLabel(label));
                QComboBox *combo = new QComboBox;
                combo->setObjectName(key);
                int currentIndex = 0;
                int index = 0;
                QXmppElement option = field.firstChildElement("option");
                while (!option.isNull())
                {
                    const QString optionValue = option.firstChildElement("value").value();
                    combo->addItem(option.attribute("label"), optionValue);
                    if (optionValue == value)
                        currentIndex = index;
                    index++;
                    option = option.nextSiblingElement("option");
                }
                combo->setCurrentIndex(currentIndex);
                hbox->addWidget(combo);
                vbox->addItem(hbox);
            }
        }
        field = field.nextSiblingElement("field");
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vbox->addWidget(buttonBox);

    setLayout(vbox);
    setWindowTitle(chatForm.firstChildElement("title").value());

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ChatForm::submit()
{
    chatForm.setAttribute("type", "submit");
    QXmppElement field = chatForm.firstChildElement("field");
    while (!field.isNull())
    {
        const QString key = field.attribute("var");
        if (!key.isEmpty())
        {
            QXmppElement value = field.firstChildElement("value");
            if (field.attribute("type") == "boolean")
            {
                QCheckBox *checkbox = findChild<QCheckBox*>(key);
                value.setValue(checkbox->checkState() == Qt::Checked ? "1" : "0");
            } else if (field.attribute("type") == "text-single") {
                QLineEdit *edit = findChild<QLineEdit*>(key);
                value.setValue(edit->text());
            } else if (field.attribute("type") == "list-single") {
                QComboBox *combo = findChild<QComboBox*>(key);
                value.setValue(combo->itemData(combo->currentIndex()).toString());
            }
        }
        field = field.nextSiblingElement("field");
    }

    accept();
}

QXmppElement ChatForm::form() const
{
    return chatForm;
}
