#ifndef PORTRAITVIEW_H
#define PORTRAITVIEW_H
#include <QGraphicsView>


class PortraitView : public QGraphicsView
{

public:
    PortraitView(QWidget *parent = nullptr);
    virtual ~PortraitView()  override;
    virtual void resizeEvent(QResizeEvent *event) override;
};

#endif // PORTRAITVIEW_H
