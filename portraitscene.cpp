#include "portraitscene.h"
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QBrush>
#include "mainwindow.h"
#include "portraitrectitem.h"

PortraitScene::PortraitScene(MainWindow *main_window) {
    this->main_window = main_window;  // to show status messages
    dash_pen.setBrush(QBrush(QImage(":/images/checkerboard.png")));
    dash_pen.setCosmetic(true);
    dash_pen.setWidth(3);
    pixmap_item = nullptr;
    rect_item = nullptr;
}

PortraitScene::~PortraitScene() {
}

void PortraitScene::set_portrait_file(  const QPixmap &pixmap) {
    int image_width = pixmap.width();
    int image_height = pixmap.height();
    QRectF crop_rect;
    if (image_width * 5 >= image_height * 4) {
        int width = image_height * 4 / 5;
        crop_rect.setRect((image_width - width)/2, 0, width, image_height);
    } else {
        int height = image_width * 5 / 4;
        crop_rect.setRect(0, (image_height - height)/2, image_width, height);
    }
    clear();
    pixmap_item = addPixmap(pixmap);
    if (main_window->show_crop_frame) {
        rect_item = new PortraitRectItem(pixmap_item);
        rect_item->setRect(crop_rect);
        rect_item->setPen(dash_pen);
        rect_item->setCursor(Qt::OpenHandCursor);
        addItem(rect_item);
    }
    setSceneRect(QRectF(0, 0, image_width, image_height));
}

QPixmap PortraitScene::get_selection() {
    if (!pixmap_item)
        return QPixmap();
    else if (rect_item && main_window->show_crop_frame) {
        QRectF rf = rect_item->rect();
        QRect  ri(int(rf.x() + 0.5), int(rf.y() + 0.5), int(rf.width() + 0.5), int(rf.height() + 0.5));
        return QPixmap::fromImage(pixmap_item->pixmap().toImage().copy(ri));
    } else {
        QRect crop;
        QRect img = pixmap_item->pixmap().rect();
        int w = img.width();
        int h = img.height();
        if (w * 5 > h * 4) {
            int nw = (h * 4 + 2) / 5;
            int dw = (5 * w - 4 * h + 5) / 10;
            crop.setRect(dw, 0, nw, h); // crop horizontally
        } else {
            int nh = (w * 5 + 2) / 4;
            int dh = (4 * h - 5 * w + 4) / 8;
            crop.setRect(0, dh, w, nh); // crop vertically
        }
        return QPixmap::fromImage(pixmap_item->pixmap().toImage().copy(crop));
    }
}

void PortraitScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mousePressEvent(mouseEvent);
}

void PortraitScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void PortraitScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}
