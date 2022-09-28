#include <QSettings>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QMessageBox>
#include "simpleprintdialog.h"
#include "ui_simpleprintdialog.h"
#include "mainwindow.h"

static const int papers[4][2] = {
    {16*72, 20*72}, {11*72, 14*72}, {8*72, 10*72}, {5*72, 7*72}
};

SimplePrintDialog::SimplePrintDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimplePrintDialog)
{
    ui->setupUi(this);
    wnd = qobject_cast<MainWindow * >(parent);

    QSettings settings;
    settings.beginGroup("printer");
    resize(settings.value("size", size()).toSize());
    settings.endGroup();

    ui->print_16_20_spin->setValue(wnd->print_16_20);
    ui->print_11_14_spin->setValue(wnd->print_11_14);
    ui->print_8_10_spin->setValue(wnd->print_8_10);
    ui->print_5_7_spin->setValue(wnd->print_5_7);

}

SimplePrintDialog::~SimplePrintDialog() {
    delete ui;
}

void SimplePrintDialog::on_help_btn_clicked() {
    wnd->show_help("help.pdf");
}



bool SimplePrintDialog::check_intersect(const QRect &arg) {
    for (const QRect &r : pages.last()) {
        if (r.intersects(arg))
            return true;
    }
    return false;
}

// Return value: -1 or 0..3 | (landscape << 3)
int SimplePrintDialog::find_best_rect(int *pic_cnt, QList<QPoint> &bases, int *prev_height, int64_t *prev_area, int media_width_pt) {
    int pic_num = -1;
    QList<QPoint> new_bases;
    int    best_base = -1;
    QRect  best_rect;
    int    best_height;
    double best_quality = 0; // Start with 0 to add at least  one rectangle.
    int64_t best_area;

    new_bases.reserve(bases.length() + 2);

    for (int j=bases.length() - 1; j >= 0; j--) {
        bool usable_base = false;

        for (int i=0; i<4; i++) {
            if (!pic_cnt[i])
                continue;

            for (int p = 0; p < 2; p++) {
                QRect rect(bases[j].x(), bases[j].y(), papers[i][p], papers[i][p ^ 1]);
                if (!check_intersect(rect)) {
                    int new_height = qMax(prev_height[0], bases[j].y() + rect.height());
                    int64_t new_area = *prev_area + papers[i][1] * papers[i][0];
                    double new_quality = new_area / (double(new_height) * media_width_pt);
                    if (new_quality > best_quality) {
                        best_quality = new_quality;
                        pic_num = i | (p << 3);
                        best_base = new_bases.length();
                        best_rect = rect;
                        best_height = new_height;
                        best_area = new_area;
                    }
                    usable_base = true;
                }
            }

        }

        if (usable_base) {
            new_bases.append(bases[j]);
        }
    }
    if (pic_num >= 0) {
        new_bases.removeAt(best_base);
        new_bases.append(QPoint(best_rect.x() + best_rect.width(), best_rect.y()));
        new_bases.append(QPoint(best_rect.x(), best_rect.y() + best_rect.height()));
        bases = new_bases;
        *prev_height = best_height;
        *prev_area   = best_area;
        pages.last().append(best_rect);
    }
    return pic_num;
}

void SimplePrintDialog::init_new_page_rects(int w, int h) {
    pages.append(QList<QRect>());
    pages.last().append(QRect(w, 0, 1, h + 1)); // guard rectangles
    pages.last().append(QRect(0, h, w, 1));
}

bool SimplePrintDialog::repack(int media_width_pt, int media_height_pt) {

    if (wnd->pixmap.isNull())
        return false;

    int pic_cnt[4];

    pic_cnt[0] = ui->print_16_20_spin->value();
    pic_cnt[1] = ui->print_11_14_spin->value();
    pic_cnt[2] = ui->print_8_10_spin->value();
    pic_cnt[3] = ui->print_5_7_spin->value();
    int tot = pic_cnt[0] + pic_cnt[1] + pic_cnt[2] + pic_cnt[3];

    QList<QPoint> bases;
    pages.clear();
    init_new_page_rects(media_width_pt, media_height_pt);

    bases.append(QPoint(0,0));
    int     current_height = 0;
    int64_t current_area = 0;
    while(tot > 0) {
        int psz = find_best_rect(pic_cnt, bases, &current_height, &current_area, media_width_pt);
        if (psz >= 0) {
            pic_cnt[psz & 3]--;
            tot--;
        } else {
            if (pages.last().length() <= 2) {
                QMessageBox::critical(this, "Page size mismatch", "The largest selected portrait does not fin on the selected media.");
                return false;
            }
            bases.clear();
            bases.append(QPoint(0,0));
            current_height = 0;
            current_area = 0;
            init_new_page_rects(media_width_pt, media_height_pt);
        }
    }
    return true;
}

void SimplePrintDialog::on_print_btn_clicked() {
    QSettings settings;
    settings.beginGroup("printer");
    settings.setValue("size", QVariant(size()));
    settings.endGroup();

    wnd->print_16_20 = ui->print_16_20_spin->value();
    wnd->print_11_14 = ui->print_11_14_spin->value();
    wnd->print_8_10 = ui->print_8_10_spin->value();
    wnd->print_5_7 = ui->print_5_7_spin->value();


    QPrinter printer;
    QPrintDialog print_dialog(&printer);
    print_dialog.setOption(QAbstractPrintDialog::PrintShowPageSize);
    if (print_dialog.exec() != QDialog::Accepted)
        return;
    QRectF paper_rect = printer.paperRect(QPrinter::Point);
    if (!repack(paper_rect.width(), paper_rect.height()))
        return;

    QPixmap sel = wnd->scene.get_selection();
    int sel_height = sel.height();
    QPainter painter;
    int page_count = pages.length();

    for (int page_no = 0; page_no < page_count; page_no++) {
        const QList<QRect> &page = pages[page_no];
        QRect bbox;
        for (int i=2; i < page.length(); i++) {
            if (i == 2)
                bbox = page[i];
            else
                bbox = bbox.united(page[i]);
        }
        QRect media_box(0, 0, paper_rect.width(), paper_rect.height());
        int w_offset = (media_box.width() - bbox.width()) / 2;
        int h_offset = (media_box.height() - bbox.height()) / 2;

        QPageSize page_size(QSizeF(qreal(media_box.width()), qreal(media_box.height())), QPageSize::Point);
        printer.setPageSize(page_size);
        printer.setPageMargins(QMarginsF());
        printer.setFullPage(true);
        printer.setPageOrientation(QPageLayout::Portrait);
        if (page_no == 0)
            painter.begin(&printer);
        else
            printer.newPage();
        painter.setWindow(media_box);

        for (int i=2; i < page.length(); i++) {
            QRect r = page[i];
            int w = r.width();
            int h = r.height();

            double aspect = double(qMin(w,h))/double(qMax(w,h));
            QRect crop_rect;
            if (aspect > 0.79)
                crop_rect = QRect(); // no cropping, aspect ratio = 4:5 is the same as aspect ratio of the selection
            else
                crop_rect = QRect(int((0.8 - aspect)/2 * sel_height + 0.5), 0, int(aspect*sel_height + 0.5), sel_height);

            if (w <= h) { // portrait
                QRect target(r.x() + w_offset, r.y() + h_offset, w, h);
                painter.drawPixmap(target, sel, crop_rect);
            } else { // landscape
                QTransform old_transform = painter.worldTransform();
                painter.translate(r.x() + w_offset, r.y() + h_offset + h);
                painter.rotate(-90);
                QRect target(0, 0, h, w);
                painter.drawPixmap(target, sel, crop_rect);
                painter.setWorldTransform(old_transform, false);
            }

            if (wnd->print_frames) {
                painter.setPen(Qt::black);
                painter.setBrush(Qt::transparent);
                painter.drawRect(r.x() + w_offset, r.y() + h_offset, r.width(), r.height());
            }
        }
    }
    painter.end();
}
