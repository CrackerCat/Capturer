#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>
#include <QPainter>

class TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = nullptr);

    void setFont(const QFont& font);

signals:
    void documentSizeChanged();
    void focus(bool);

protected:
      virtual void focusInEvent(QFocusEvent *e);
      virtual void focusOutEvent(QFocusEvent *e);

private:
    QPainter painter_;
};

#endif // TEXTEDIT_H
