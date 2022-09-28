#include "portraitview.h"

PortraitView::PortraitView(QWidget *parent)
  : QGraphicsView(parent)
{

}

PortraitView::~PortraitView() {
}

void PortraitView::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    QRectF rect = this->scene()->itemsBoundingRect();
    fitInView(rect, Qt::KeepAspectRatio);
}
