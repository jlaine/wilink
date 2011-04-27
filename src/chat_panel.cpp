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

#include <QApplication>
#include <QDebug>
#include <QDropEvent>
#include <QGraphicsLinearLayout>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QLayout>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QTextDocument>
#include <QTimer>

#include "chat_panel.h"

#define BORDER_RADIUS 8

class ChatPanelPrivate
{
public:
    void updateTitle();

    QVBoxLayout *header;
    QHBoxLayout *hbox;
    QHBoxLayout *widgets;
    QPushButton *attachButton;
    QPushButton *closeButton;
    QLabel *helpLabel;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QString windowExtra;
    QString windowStatus;
    QList< QPair<QString, int> > notificationQueue;

    ChatPanel *q;
};

void ChatPanelPrivate::updateTitle()
{
    nameLabel->setText(QString("<b>%1</b> %2<br/>%3").arg(q->windowTitle(),
        windowStatus, windowExtra));
}

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent),
    d(new ChatPanelPrivate)
{
    bool check;
    d->q = this;

    d->attachButton = new QPushButton;
    d->attachButton->setFlat(true);
    d->attachButton->setMaximumWidth(32);
    d->attachButton->setIcon(QIcon(":/add.png"));
    d->attachButton->hide();
    check = connect(d->attachButton, SIGNAL(clicked()),
                    this, SIGNAL(attachPanel()));
    Q_ASSERT(check);

    d->closeButton = new QPushButton;
    d->closeButton->setFlat(true);
    d->closeButton->setMaximumWidth(32);
    d->closeButton->setIcon(QIcon(":/close.png"));
    check = connect(d->closeButton, SIGNAL(clicked()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    d->iconLabel = new QLabel;
    d->nameLabel = new QLabel;

    d->hbox = new QHBoxLayout;
    d->hbox->addSpacing(16);
    d->hbox->addWidget(d->nameLabel);
    d->hbox->addStretch();
    d->hbox->addWidget(d->iconLabel);
    d->hbox->addWidget(d->attachButton);
    d->hbox->addWidget(d->closeButton);

    // assemble header
    d->header = new QVBoxLayout;
    d->header->setMargin(0);
    d->header->setSpacing(10);
    d->header->addLayout(d->hbox);

    d->helpLabel = new QLabel;
    d->helpLabel->setWordWrap(true);
    d->helpLabel->setOpenExternalLinks(true);
    d->helpLabel->hide();
    d->header->addWidget(d->helpLabel);

    d->widgets = new QHBoxLayout;
    d->widgets->addStretch();
    d->header->addLayout(d->widgets);

    setMinimumWidth(300);
}

ChatPanel::~ChatPanel()
{
    delete d;
}

void ChatPanel::addWidget(QGraphicsWidget *widget)
{
    Q_UNUSED(widget);
}

/** Return the type of entry to add to the roster.
 */
ChatRosterModel::Type ChatPanel::objectType() const
{
    return ChatRosterModel::Other;
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowExtra(const QString &extra)
{
    d->windowExtra = extra;
    d->updateTitle();
}

/** Sets the window's help text.
 *
 * @param help
 */
void ChatPanel::setWindowHelp(const QString &help)
{
    d->helpLabel->setText(help);
#ifndef WILINK_EMBEDDED
    if (help.isEmpty())
        d->helpLabel->hide();
    else
        d->helpLabel->show();
#endif
}

/** When the window icon is set, update the header icon.
 *
 * @param icon
 */
void ChatPanel::setWindowIcon(const QIcon &icon)
{
    QWidget::setWindowIcon(icon);
    const QSize actualSize = icon.actualSize(QSize(64, 64));
    d->iconLabel->setPixmap(icon.pixmap(actualSize));
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowStatus(const QString &status)
{
    d->windowStatus = status;
    d->updateTitle();
}

/** When the window title is set, update the header text.
 *
 * @param title
 */
void ChatPanel::setWindowTitle(const QString &title)
{
    QWidget::setWindowTitle(title);
    d->updateTitle();
}

/** Return a layout object for the panel header.
 */
QLayout* ChatPanel::headerLayout()
{
    return d->header;
}

void ChatPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ParentChange)
    {
        if (parent())
        {
            layout()->setMargin(0);
            d->attachButton->hide();
            d->closeButton->show();
        } else {
            layout()->setMargin(6);
            d->attachButton->show();
            d->closeButton->hide();
        }
    }
    QWidget::changeEvent(event);
}

void ChatPanel::closeEvent(QCloseEvent *event)
{
    emit hidePanel();
    QWidget::closeEvent(event);
}

bool ChatPanel::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);

    if (e->type() == QEvent::DragEnter)
    {
        QDragEnterEvent *event = static_cast<QDragEnterEvent*>(e);
        event->acceptProposedAction();
        return true;
    }
    else if (e->type() == QEvent::DragLeave)
    {
        return true;
    }
    else if (e->type() == QEvent::DragMove || e->type() == QEvent::Drop)
    {
        QDropEvent *event = static_cast<QDropEvent*>(e);
        event->ignore();
        emit dropPanel(event);
        return true;
    }
    return false;
}

void ChatPanel::filterDrops(QWidget *widget)
{
    widget->setAcceptDrops(true);
    widget->installEventFilter(this);
}

void ChatPanel::queueNotification(const QString &message, int options)
{
    d->notificationQueue << qMakePair(message, options);
    QTimer::singleShot(0, this, SLOT(sendNotifications()));
}

void ChatPanel::sendNotifications()
{
    while (!d->notificationQueue.isEmpty())
    {
        QPair<QString, int> entry = d->notificationQueue.takeFirst();
        emit notifyPanel(entry.first, entry.second);
    }
}

/** Create a custom palette for chat panel */
QPalette ChatPanel::palette()
{
    QPalette palette;

    // colors for active and inactive content
    palette.setColor(QPalette::Light, QColor("#E7F4FE"));
    palette.setColor(QPalette::Midlight, QColor("#BFDDF4"));
    palette.setColor(QPalette::Button, QColor("#88BFE9"));
    palette.setColor(QPalette::Mid, QColor("#4A9DDD"));
    palette.setColor(QPalette::Dark, QColor("#2689D6"));
    palette.setColor(QPalette::Shadow, QColor("#aaaaaa"));
    palette.setColor(QPalette::ButtonText, QColor("#000000"));
    palette.setColor(QPalette::BrightText, QColor("#FFFFFF"));

    // colors for disabled content
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor("#F5FAFE"));
    palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor("#E5F1FA"));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor("#CFE5F6"));
    palette.setColor(QPalette::Disabled, QPalette::Mid, QColor("#B6D7F1"));
    palette.setColor(QPalette::Disabled, QPalette::Dark, QColor("#A8CFEE"));
    palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor("#D6D6D6"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#999999"));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor("#FFFFFF"));

    return palette;
}

/** Creates a new ChatPanelBar instance.
 *
 * @param view
 */
ChatPanelBar::ChatPanelBar(QGraphicsView *view)
    : m_view(view)
{
    const QPalette palette = ChatPanel::palette();
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setColor(palette.color(QPalette::Shadow));
    effect->setOffset(0, 3);
    effect->setBlurRadius(10);
    setGraphicsEffect(effect);
    m_view->viewport()->installEventFilter(this);

    m_delay = new QTimer(this);
    m_delay->setInterval(100);
    m_delay->setSingleShot(true);

    m_animation = new QPropertyAnimation(this, "geometry");
    m_animation->setEasingCurve(QEasingCurve::OutQuad);
    m_animation->setDuration(500);

    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_layout->setContentsMargins(8, 8, 8, 8);
    setLayout(m_layout);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    bool check;
    check = connect(m_view->scene(), SIGNAL(sceneRectChanged(QRectF)),
                    m_delay, SLOT(start()));
    Q_ASSERT(check);

    check = connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                    m_delay, SLOT(start()));
    Q_ASSERT(check);

    check = connect(m_delay, SIGNAL(timeout()),
                    this, SLOT(trackView()));
    Q_ASSERT(check);
}

void ChatPanelBar::addWidget(QGraphicsWidget *widget)
{
    ChatPanelWidget *wrapper = new ChatPanelWidget(widget, this);
    connect(widget, SIGNAL(finished()), wrapper, SLOT(disappear()));
    m_layout->addItem(wrapper);
    wrapper->appear();
}

bool ChatPanelBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Resize) {
        m_delay->start();
    }
    return false;
}

void ChatPanelBar::trackView()
{
    // determine new geometry
    QPointF newPos = m_view->mapToScene(QPoint(0, 0));
    QSizeF newSize = QGraphicsWidget::sizeHint(Qt::PreferredSize);
    newSize.setWidth(m_view->viewport()->width() - 1);
    QRectF newGeometry(newPos, newSize);

    // animate geometry
    m_animation->setStartValue(geometry());
    m_animation->setEndValue(newGeometry);
    m_animation->start();
}

void ChatPanelBar::updateGeometry()
{
    QGraphicsWidget::updateGeometry();
    trackView();
}

ChatPanelButton::ChatPanelButton(QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
    const QPalette palette = ChatPanel::palette();
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0.2, palette.color(QPalette::Light));
    gradient.setColorAt(0.3, palette.color(QPalette::Midlight));
    gradient.setColorAt(0.7, palette.color(QPalette::Midlight));
    gradient.setColorAt(1, palette.color(QPalette::Button));
    m_path = new QGraphicsPathItem(this);
    m_path->setBrush(gradient);
    m_path->setPen(QPen(palette.color(QPalette::Mid)));
    m_pixmap = new QGraphicsPixmapItem(this);
}

void ChatPanelButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (isEnabled() &&
        m_path->path().contains(event->pos()))
    {
        const QPalette palette = ChatPanel::palette();
        QLinearGradient gradient(0, 0, 0, 1);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setColorAt(0.2, palette.color(QPalette::Button));
        gradient.setColorAt(0.3, palette.color(QPalette::Midlight));
        gradient.setColorAt(0.7, palette.color(QPalette::Midlight));
        gradient.setColorAt(1, palette.color(QPalette::Light));
        m_path->setBrush(gradient);
        m_path->setPen(QPen(palette.color(QPalette::Dark), 0.5));
    }
    event->accept();
}

void ChatPanelButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    const QPalette palette = ChatPanel::palette();
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0.2, palette.color(QPalette::Light));
    gradient.setColorAt(0.3, palette.color(QPalette::Midlight));
    gradient.setColorAt(0.7, palette.color(QPalette::Midlight));
    gradient.setColorAt(1, palette.color(QPalette::Button));
    m_path->setBrush(gradient);
    m_path->setPen(QPen(palette.color(QPalette::Mid), 0.7));
    if (isEnabled() &&
        m_path->path().contains(event->pos()) &&
        m_path->path().contains(event->buttonDownPos(Qt::LeftButton)))
        emit clicked();
    event->accept();
}

/** Updates the widget geometry.
 */
void ChatPanelButton::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    QRectF rect(baseRect);
    rect.moveLeft(0);
    rect.moveTop(0);

    QPainterPath buttonPath;
    buttonPath.addRoundedRect(QRectF(0.5, 0.5, rect.width() - 1, rect.height() - 1),
                              BORDER_RADIUS/2, BORDER_RADIUS/2);
    m_path->setPath(buttonPath);

    const QSizeF pixmapSize = m_pixmap->pixmap().size();
    m_pixmap->setPos(
        (rect.width() - pixmapSize.width()) / 2,
        (rect.height() - pixmapSize.height()) / 2);
}

/** Sets the widget's button pixmap.
 */
void ChatPanelButton::setPixmap(const QPixmap &pixmap)
{
    m_pixmap->setPixmap(pixmap.scaledToWidth(24, Qt::SmoothTransformation));
    updateGeometry();
}

QSizeF ChatPanelButton::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    if (which == Qt::MinimumSize || which == Qt::PreferredSize) {
        QSizeF hint = m_pixmap->pixmap().size();
        hint.setHeight(hint.height() + 8);
        hint.setWidth(hint.width() + 8);
        return hint;
    } else {
        return constraint;
    }
}

ChatPanelText::ChatPanelText(const QString &text, QGraphicsItem *parent)
    : QGraphicsTextItem(text, parent)
{
    setGraphicsItem(this);
}

void ChatPanelText::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void ChatPanelText::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
    emit clicked();
}

void ChatPanelText::setGeometry(const QRectF &rect)
{
    QGraphicsLayoutItem::setGeometry(rect);
    setPos(rect.topLeft());
}

QSizeF ChatPanelText::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    if (which == Qt::MinimumSize || which == Qt::PreferredSize) {
        return document()->size();
    } else {
        return constraint;
    }
}

/** Creates a new ChatPanelWidget instance.
 *
 * @param parent
 */
ChatPanelWidget::ChatPanelWidget(QGraphicsWidget *contents, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    m_centralWidget(contents)
{
    // widget starts transparent, use appear() to show it
    setOpacity(0.0);

    const QPalette palette = ChatPanel::palette();

    m_border = new QGraphicsPathItem(this);
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0, palette.color(QPalette::Light));
    gradient.setColorAt(0.2, palette.color(QPalette::Midlight));
    gradient.setColorAt(0.8, palette.color(QPalette::Midlight));
    gradient.setColorAt(1, palette.color(QPalette::Light));
    m_border->setBrush(gradient);
    m_border->setPen(QPen(palette.color(QPalette::Dark), 1));

    m_centralWidget->setParentItem(this);
}

/** Makes the widget appear.
 */
void ChatPanelWidget::appear()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(500);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

/** Makes the widget disappear then deletes it.
 */
void ChatPanelWidget::disappear()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(500);
    animation->setEasingCurve(QEasingCurve::InQuad);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->start();
    connect(animation, SIGNAL(finished()), this, SLOT(deleteLater()));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

/** Updates the widget geometry.
 */
void ChatPanelWidget::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    QRectF rect(baseRect);
    rect.moveLeft(0);
    rect.moveTop(0);

    // position border
    QPainterPath path;
    path.addRoundedRect(QRectF(0.5, 0.5, rect.width() - 1, rect.height() - 1), BORDER_RADIUS, BORDER_RADIUS);
    m_border->setPath(path);

    // position contents
    rect.adjust(BORDER_RADIUS/2, BORDER_RADIUS/2, -BORDER_RADIUS/2, -BORDER_RADIUS/2);
    m_centralWidget->setGeometry(rect);
}

QSizeF ChatPanelWidget::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    if (which == Qt::MinimumSize || which == Qt::PreferredSize) {
        QSizeF hint = m_centralWidget->effectiveSizeHint(which);
        hint.setWidth(hint.width() + BORDER_RADIUS);
        hint.setHeight(hint.height() + BORDER_RADIUS);
        return hint;
    } else {
        return constraint;
    }
}

