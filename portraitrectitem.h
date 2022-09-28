#ifndef PORTRAITRECTITEM_H
#define PORTRAITRECTITEM_H
#include <QGraphicsRectItem>

typedef enum {kInside=0, kEast=1, kNorthEast=2, kNorth=3, kNorthWest=4, kWest=5, kSouthWest=6, kSouth=7, kSouthEast=8, kOutside=9} Rhumb;

class PortraitRectItem : public QGraphicsRectItem
{
public:
    PortraitRectItem(QGraphicsPixmapItem *pixmap);
    void setRectChecked(qreal x1, qreal y1, qreal x2, qreal y2);
protected:
//    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
//    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
private:
    QGraphicsPixmapItem *pixmap;
    Rhumb getRhumb(qreal x, qreal y);
    Rhumb rhumb_at_mouse_down;
    qreal mx_at_mouse_down, my_at_mouse_down; // mouse
    qreal rx1_at_mouse_down, ry1_at_mouse_down,  // rectangle box
          rx2_at_mouse_down, ry2_at_mouse_down;
};

#endif // PORTRAITRECTITEM_H
