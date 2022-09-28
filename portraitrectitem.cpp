#include "portraitrectitem.h"
#include <QGraphicsSceneHoverEvent>
#include <QCursor>

static bool near(double x, double y) {
    return  x - y < 5 && x - y > - 5;
}

static const Qt::CursorShape rhumb2cursor[] = {
    Qt::OpenHandCursor,  // kInside
    Qt::SizeHorCursor,   // kEast
    Qt::SizeBDiagCursor, // kNortEast
    Qt::SizeVerCursor,   // kNorth
    Qt::SizeFDiagCursor, // kNorthWest
    Qt::SizeHorCursor,   // kWest
    Qt::SizeBDiagCursor, // kSouthWest
    Qt::SizeVerCursor,   // kSouth
    Qt::SizeFDiagCursor, // kSouthEast
    Qt::ArrowCursor      // kOutside
};

Rhumb PortraitRectItem::getRhumb(qreal x, qreal y) {
    qreal x1, y1, x2, y2;
    this->rect().getCoords(&x1, &y1, &x2, &y2);
    if (near(x, x1)) {
        if (near(y, y1))
            return kNorthWest;
        if (near(y, y2))
            return kSouthWest;
        if (y > y1 && y < y2)
            return kWest;
        return kInside;
    }
    if (near(x, x2)) {
        if (near(y, y1))
            return kNorthEast;
        if (near(y, y2))
            return kSouthEast;
        if (y > y1 && y < y2)
            return kEast;
        return kInside;
    }
    if (near(y, y1))
        return kNorth;
    if (near(y, y2))
        return kSouth;
    if (x > x1 && x < x2 && y > y1 && y < y2)
        return kInside;
    return kOutside;
}

void PortraitRectItem::setRectChecked(qreal x1, qreal y1, qreal x2, qreal y2) {
    qreal ix1, iy1, ix2, iy2;
    pixmap->boundingRect().getCoords(&ix1, &iy1, &ix2, &iy2);
    qreal w = x2 - x1;
    qreal h = y2 - y1;
    if (w > ix2 - ix1 || h > iy2 - iy1)
        return;
    x1 = qMax(x1, ix1);
    y1 = qMax(y1, iy1);
    x1 = qMin(x1, ix2-w);
    y1 = qMin(y1, iy2-h);
    setRect(x1, y1, w, h);
}

PortraitRectItem::PortraitRectItem(QGraphicsPixmapItem *pixmap)
{
    this->pixmap = pixmap;
    setAcceptHoverEvents(true);
}



void PortraitRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPointF pos = event->scenePos();
    this->setCursor(QCursor(rhumb2cursor[getRhumb(pos.x(), pos.y())]));
}


void PortraitRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    qreal dx, dy; // mouse delta
    qreal sx, sy; // stationary point of scaling
    qreal mscale; // user request for scaling
    qreal ex, ey; // x and y extents of rect at mouse down event.
    QPointF pos = event->scenePos();
    dx = pos.x() - mx_at_mouse_down;
    dy = pos.y() - my_at_mouse_down;
    ex = rx2_at_mouse_down - rx1_at_mouse_down;
    ey = ry2_at_mouse_down - ry1_at_mouse_down;

    switch(rhumb_at_mouse_down) {
    case kInside:
        setRectChecked(rx1_at_mouse_down+dx, ry1_at_mouse_down+dy, rx2_at_mouse_down+dx, ry2_at_mouse_down+dy);
        break;
    case kEast:
        sx = rx1_at_mouse_down;
        sy = (ry1_at_mouse_down + ry2_at_mouse_down)/2;
        mscale = 1 + dx/ex;
        setRectChecked(sx, sy - mscale*ey/2, sx + mscale*ex, sy + mscale*ey/2);
        break;
    case kNorthEast:
        sx = rx1_at_mouse_down;
        sy = ry2_at_mouse_down;
        mscale = 1 + (dx-dy)/ex/2.25;
        setRectChecked(sx, sy-mscale*ey, sx + mscale*ex, sy);
        break;
    case kNorth:
        sx = (rx1_at_mouse_down + rx2_at_mouse_down)/2;
        sy = ry2_at_mouse_down;
        mscale = 1 - dy/ey;
        setRectChecked(sx-mscale*ex/2, sy-mscale*ey, sx + mscale*ex/2, sy);
        break;
    case kNorthWest:
        sx = rx2_at_mouse_down;
        sy = ry2_at_mouse_down;
        mscale = 1 - (dx+dy)/ex/2.25;
        setRectChecked(sx-mscale*ex, sy-mscale*ey, sx, sy);
        break;
    case kWest:
        sx = rx2_at_mouse_down;
        sy = (ry1_at_mouse_down + ry2_at_mouse_down)/2;
        mscale = 1 - dx/ex;
        setRectChecked(sx-mscale*ex, sy-mscale*ey/2, sx, sy+mscale*ey/2);
        break;
    case kSouthWest:
        sx = rx2_at_mouse_down;
        sy = ry1_at_mouse_down;
        mscale = 1 - (dx-dy)/ex/2.25;
        setRectChecked(sx-mscale*ex, sy, sx, sy+mscale*ey);
        break;
    case kSouth:
        sx = (rx1_at_mouse_down + rx2_at_mouse_down)/2;
        sy = ry1_at_mouse_down;
        mscale = 1 + dy/ey;
        setRectChecked(sx-mscale*ex/2, sy, sx + mscale*ex/2, sy+mscale*ey);
        break;
    case kSouthEast:
        sx = rx1_at_mouse_down;
        sy = ry1_at_mouse_down;
        mscale = 1 + (dx+dy)/ex/2.25;
        setRectChecked(sx, sy, sx+mscale*ex, sy+mscale*ey);
        break;
    case kOutside:
        break; // Should not happen; do nothing
    }

}
void PortraitRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->scenePos();
    mx_at_mouse_down = pos.x();
    my_at_mouse_down = pos.y();
    this->rect().getCoords(&rx1_at_mouse_down, &ry1_at_mouse_down, &rx2_at_mouse_down, &ry2_at_mouse_down);
    rhumb_at_mouse_down = getRhumb(mx_at_mouse_down, my_at_mouse_down);
    grabMouse();
}
void PortraitRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event)
    ungrabMouse();
}
