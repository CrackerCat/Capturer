#include "textedit.h"
#include <QDebug>

TextEdit::TextEdit(QWidget * parent)
    : QTextEdit(parent)
{
   setFrameShape(QFrame::NoFrame);
   setAttribute(Qt::WA_TranslucentBackground);
   viewport()->setAutoFillBackground(false);

   setTextColor(QColor(0, 0, 0, 0));
   document()->setDocumentMargin(2);

   setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

   setLineWrapMode(QTextEdit::NoWrap);

   setFixedSize(document()->size().toSize());

   auto resize_fuctor = [=](){
       setFixedSize(document()->size().toSize());
   };

   connect(this, &TextEdit::documentSizeChanged, resize_fuctor);
   connect(this, &TextEdit::textChanged, resize_fuctor);
   connect(document(), &QTextDocument::modificationChanged, resize_fuctor);
}

void TextEdit::setFont(const QFont& font)
{
    QTextEdit::setFont(font);
    emit documentSizeChanged();
}

void TextEdit::focusInEvent(QFocusEvent *e)
{
    emit focus(true);
    QTextEdit::focusInEvent(e);
}
void TextEdit::focusOutEvent(QFocusEvent *e)
{
    emit focus(false);
    hide();
    QTextEdit::focusOutEvent(e);
}
