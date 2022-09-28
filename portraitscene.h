#ifndef PORTRAITSCENE_H
#define PORTRAITSCENE_H
#include <QGraphicsScene>
class MainWindow;
class PortraitRectItem;

class PortraitScene : public QGraphicsScene
{
public:
    PortraitScene(MainWindow *mw);
    virtual ~PortraitScene() override;
    void set_portrait_file(const QPixmap &pixmap);
    QPixmap get_selection();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
private:
    MainWindow *main_window;
    QPen dash_pen;
    PortraitRectItem *rect_item;
    QGraphicsPixmapItem *pixmap_item;
};

#endif // PORTRAITSCENE_H
