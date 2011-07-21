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

import QtQuick 1.0

Panel {
    function trim(text)
    {
        return text.replace(/^\s+|\s+$/g, '');
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'peer.png'
        title: qsTr('News reader')
    }

    ListView {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }
}

/*
News.prototype.showItems = function(error, reply, redirect)
{
    if (redirect) {
        qDebug("Following redirect to " + redirect);
        this.client.get(redirect, "*");
        return;
    }

    if (error) {
        qWarning("News retrieval failed");
        if (!this.menus.length)
            this.quit();
        return;
    }

    // Create UI
    var widget = script.createWidget("BocChoice");
    widget.accept.connect(this, this.showDetails);
    widget.info.connect(this, this.info);
    if (this.widgets.length)
        widget.cancel.connect(this, this.goBack);
    else
        widget.cancel.connect(this, this.quit);
    this.widgets.push(widget);

    var doc = new QDomDocument();
    doc.setContent(reply);
    this.title = doc.elementsByTagName("title").at(0).firstChild().nodeValue();
    this.description = doc.elementsByTagName("description").at(0).firstChild().nodeValue();
    var items = doc.elementsByTagName("item");

    var entries = [];
    for (var i = 0; i < items.length(); i++) {
        var title = trim(items.at(i).firstChildElement("title").text());
        var description = trim(items.at(i).firstChildElement("description").text());
        var link = trim(items.at(i).firstChildElement("link").text());

        var inline = description.length < 16;
        var summarise = title.length < 32;
        var maxlen = 24;

        entries[i] = {
            'link': link,
            'description': description,
            'inline': inline
        };
        var label = script.createWidget("QLabel");
        label.alignment = Qt.AlignCenter;
        label.objectName = i;
        if (inline) {
            label.text = "<b>" + title + "</b><br/>" + cleanup_html(description);
        } else if (summarise) {
            var extract = strip_html(description);
            if (extract.length > maxlen)
                extract = extract.substring(0, maxlen) + "..";
            label.text = "<b>" + title + "</b><br/><small>" + extract + "</small>";
        } else {
            label.text = "<html>" + title + "</html>";
        }
        label.textInteractionFlags = Qt.NoTextInteraction;
        label.wordWrap = true;
        widget.addWidget(label);
    }
    if (!entries.length)
    {
        var label = script.createWidget("QLabel");
        label.alignment = Qt.AlignCenter;
        label.text = tr("No items");
        widget.addWidget(label);
    }
    this.menus.push(entries);

    script.addWindow(widget);
    script.removeWindow(this.splash);
}
*/

