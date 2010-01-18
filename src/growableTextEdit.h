#ifndef GrowableTextEdit_H
#define GrowableTextEdit_H

#include <QTextEdit>
#include <QKeyEvent>

class growableTextEdit : public QTextEdit
{
  Q_OBJECT

public:
  growableTextEdit(int maxheight = 80, QWidget* parent = NULL);
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

signals:
  void returnPressed();

protected:
  virtual void resizeEvent(QResizeEvent *e);
  virtual void keyPressEvent(QKeyEvent* e);

public slots:
  void onTextChanged();

private:
  int maxHeight;
};
#endif
