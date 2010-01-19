#ifndef GrowableTextEdit_H
#define GrowableTextEdit_H

#include <QKeyEvent>
#include <QTextEdit>

class growableTextEdit : public QTextEdit
{
  Q_OBJECT

public:
  growableTextEdit(int maxheight = 80, QWidget* parent = NULL);
  virtual QSize minimumSizeHint() const;
  virtual QSize sizeHint() const;

signals:
  void returnPressed();

protected:
  virtual void keyPressEvent(QKeyEvent* e);
  virtual void resizeEvent(QResizeEvent *e);

public slots:
  void onTextChanged();

private:
  int maxHeight;
};
#endif
