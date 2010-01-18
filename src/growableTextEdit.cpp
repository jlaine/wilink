#include "growableTextEdit.h"
#include <QDebug>

void growableTextEdit::onTextChanged()
{
    static int oldHeight = 0;
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    if (myHeight != oldHeight)
        updateGeometry();
    oldHeight = myHeight;
}

void growableTextEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    updateGeometry();
}

void growableTextEdit::keyPressEvent(QKeyEvent* e)
{
    if(e->key()==Qt::Key_Return)
        emit returnPressed();
    else
        QTextEdit::keyPressEvent(e);
}

growableTextEdit::growableTextEdit(int maxheight, QWidget* parent) : QTextEdit(parent), maxHeight(maxheight)
{
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

QSize growableTextEdit::sizeHint() const
{
    QSize sizeHint = QTextEdit::sizeHint();
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    sizeHint.setHeight(qMin(myHeight, maxHeight));
    return sizeHint;
}

QSize growableTextEdit::minimumSizeHint() const
{
    QSize sizeHint = QTextEdit::minimumSizeHint();
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    sizeHint.setHeight(qMin(myHeight, maxHeight));
    return sizeHint;
}
