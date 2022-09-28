#include "printdialog.h"
#include "ui_printdialog.h"
#include <QSettings>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPageSize>
#include <QGraphicsItem>
#include <QDesktopServices>
#include <QPrintDialog>
#include <QTimer>
#include "mainwindow.h"

static const int papers[4][2] = {
    {16*72, 20*72}, {11*72, 14*72}, {8*72, 10*72}, {5*72, 7*72}
};

PrintDialog::PrintDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrintDialog)
{
    in_progress = true;
    ui->setupUi(this);
    ui->errors_browser->hide();
    current_page = 0;
    wnd = qobject_cast<MainWindow * >(parent);

    QSettings settings;
    settings.beginGroup("printer");
    resize(settings.value("size", size()).toSize());
    printer_model = settings.value("model").toString();
    paper_format = settings.value("paper_format", "Letter").toString();
    media_width_pt  = settings.value("media_width", 612).toInt();
    media_height_pt = settings.value("media_height", 792).toInt();
    units = settings.value("units").toInt();
    is_roll = settings.value("roll").toInt();
    horz_alignment = settings.value("horz_alignment").toInt();
    vert_alignment = settings.value("vert_alignment").toInt();
    settings.endGroup();

    ui->media_width_spin->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    ui->media_height_spin->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    ui->media_unit_combo->setCurrentIndex(units);

    ui->horiz_alignment_combo->setCurrentIndex(horz_alignment);
    ui->vert_alignment_combo->setCurrentIndex(vert_alignment);
    ui->feed_type_combo->setCurrentIndex(is_roll);

    ui->print_16_20_spin->setValue(wnd->print_16_20);
    ui->print_11_14_spin->setValue(wnd->print_11_14);
    ui->print_8_10_spin->setValue(wnd->print_8_10);
    ui->print_5_7_spin->setValue(wnd->print_5_7);

    ui->vert_alignment_combo->setEnabled(!is_roll);
    ui->vert_alignment_lbl->setEnabled(!is_roll);
    ui->media_height_lbl->setEnabled(!is_roll);
    ui->media_height_spin->setEnabled(!is_roll);

    printer_list = QPrinterInfo::availablePrinters();
    for (const QPrinterInfo &info : printer_list) {
        ui->printer_combo->addItem(info.printerName());
    }
    printer_index = printer_model.isEmpty() ? 0 : ui->printer_combo->findText(printer_model);
    ui->printer_combo->setCurrentIndex(printer_index);
    printer_combo_currentIndexChanged(printer_index);
    printer_model = ui->printer_combo->currentText();

    int media_index = ui->media_name_combo->findText(paper_format);
    if (media_index < 0 || media_index >= ui->media_name_combo->count())
        media_index = 0; // This should be a default page size or the closest sise.
    ui->media_name_combo->setCurrentIndex(media_index);
    on_media_name_combo_currentIndexChanged(media_index);

    ui->graphics_view->setScene(&scene);
    ui->graphics_view->setInteractive(false);
    in_progress = false;
    last_error_is_media_error = false;
    QTimer::singleShot(42, Qt::CoarseTimer, this, &PrintDialog::repacl_and_layout_preview);
}

PrintDialog::~PrintDialog() {
    delete ui;
}

void PrintDialog::save_params() {
    wnd->print_16_20 = ui->print_16_20_spin->value();
    wnd->print_11_14 = ui->print_11_14_spin->value();
    wnd->print_8_10 = ui->print_8_10_spin->value();
    wnd->print_5_7 = ui->print_5_7_spin->value();

    QSettings settings;
    settings.beginGroup("printer");
    settings.setValue("size", QVariant(size()));
    settings.setValue("model", QVariant(ui->printer_combo->currentText()));
    settings.value("paper_format", ui->media_name_combo->currentText());
    settings.setValue("media_width", QVariant(media_width_pt));
    settings.setValue("media_height", QVariant(media_height_pt));
    settings.setValue("units", QVariant(units));
    settings.setValue("roll", QVariant(is_roll));
    settings.setValue("horz_alignment", QVariant(horz_alignment));
    settings.setValue("vert_alignment", QVariant(vert_alignment));
    settings.endGroup();
}

void PrintDialog::update_page_controls() {
    int cnt = pages.length();
    if (cnt == 0)
        ui->page_num_label->setText("No pages");
    else
        ui->page_num_label->setText(QStringLiteral("Page %1 of %2").arg(current_page + 1).arg(cnt));
    ui->prev_page_btn->setEnabled(current_page > 0);
    ui->next_page_btn->setEnabled(current_page < cnt - 1);
    ui->print_dialog_btn->setEnabled(cnt > 0);
    ui->print_now_btn->setEnabled(cnt > 0);
}

void PrintDialog::update_media_sizes() {
    in_progress = true  ;
    QDoubleSpinBox *wdsb = ui->media_width_spin;
    QDoubleSpinBox *hdsb = ui->media_height_spin;
    const int maxw = 3048;   // 3048mm == 120 in == 8640 pt
    const int maxh = 3*maxw;
    switch(units) {
    case 1: // mm
        wdsb->setDecimals(1);
        wdsb->setMaximum(maxw);
        wdsb->setValue(media_width_pt / 72. * 25.4);
        hdsb->setDecimals(1);
        hdsb->setMaximum(maxh);
        hdsb->setValue(media_height_pt / 72. * 25.4);
        break;
    case 2: // inch
        wdsb->setDecimals(2);
        wdsb->setMaximum(maxw*10/254);
        wdsb->setValue(media_width_pt / 72.);
        hdsb->setDecimals(2);
        hdsb->setMaximum(maxh*10/254);
        hdsb->setValue(media_height_pt / 72.);
        break;
    default: // point
        wdsb->setDecimals(0);
        wdsb->setMaximum(maxw*720/254);
        wdsb->setValue(media_width_pt);
        hdsb->setDecimals(0);
        hdsb->setMaximum(maxh*720/254);
        hdsb->setValue(media_height_pt);
        break;
    }
    in_progress = false;
}
void PrintDialog::select_best_media_format(const QList<QPageSize> &page_sizes) {
    int count = page_sizes.count();
    int best_i = 0;
    int diff = 1000000000; //infinity
    QPageSize pgsz;
    QSize ppsz;
    for (int i = 0; i < count && diff; i++) {
        pgsz = page_sizes[i];
        ppsz = pgsz.sizePoints();
        int pdiff = abs(ppsz.width() - media_width_pt) + abs(ppsz.height() - media_height_pt);
        if (pdiff < diff) {
            diff = pdiff;
            best_i = i;
        }
    }
    ui->media_name_combo->setCurrentIndex(best_i);
    on_media_name_combo_currentIndexChanged(best_i);
}

void PrintDialog::printer_combo_currentIndexChanged(int index) {
    if (index < 0 || index >= printer_list.length())
        return;
    QPrinterInfo info = printer_list[index];

    in_progress = true;
    ui->media_name_combo->clear();
    const QList<QPageSize> &page_sizes = info.supportedPageSizes();
    for (const QPageSize &p : page_sizes)
        ui->media_name_combo->addItem(p.name());
    if (info.supportsCustomPageSizes() || true) { // fixme: debug
        ui->media_name_combo->addItem("Custom size");
    }
    printer_model = info.printerName();
    printer_index = index;
    in_progress = false;
    select_best_media_format(page_sizes);
}

void PrintDialog::on_printer_combo_currentIndexChanged(int index) {
    if (in_progress)
        return;
    printer_combo_currentIndexChanged(index);
}


void PrintDialog::on_media_name_combo_currentIndexChanged(int index) {
    if (index < 0 || in_progress)
        return;
    QPrinterInfo info = printer_list[printer_index];
    const QList<QPageSize> &page_sizes = info.supportedPageSizes();
    if (index >= page_sizes.count()) {
        // custom page
        ui->feed_type_combo->setEnabled(true);
        ui->feed_type_lbl->setEnabled(true);
        ui->media_width_spin->setEnabled(true);
        ui->media_width_lbl->setEnabled(true);
        bool is_height_enabled = ui->feed_type_combo->currentIndex() == 0;
        ui->media_height_spin->setEnabled(is_height_enabled);
        ui->media_height_lbl->setEnabled(is_height_enabled);
    } else {
        // standard page
        ui->feed_type_combo->setEnabled(false);
        ui->feed_type_lbl->setEnabled(false);
        ui->feed_type_combo->setCurrentIndex(0);
        ui->media_width_spin->setEnabled(false);
        ui->media_width_lbl->setEnabled(false);
        ui->media_height_spin->setEnabled(false);
        ui->media_height_lbl->setEnabled(false);
        const QPageSize& page_size = page_sizes[index];
        QSize sz;
        sz = page_size.sizePoints();
        media_width_pt = sz.width();
        media_height_pt = sz.height();
    }
    update_media_sizes();
    repack();
}

void PrintDialog::on_print_now_btn_clicked() {
    do_print(false);
}

void PrintDialog::on_print_dialog_btn_clicked() {
    do_print(true);
}

void PrintDialog::do_print(bool show_dialog) {
    QPrinter printer;
    printer.setPrinterName(printer_model);

    QSettings settings;
    settings.beginGroup("printer");

    if (settings.contains("color_mode"))
        printer.setColorMode(QPrinter::ColorMode(settings.value("color_mode").toInt()));
    else
        show_dialog = true;

    if (settings.contains("paper_source"))
        printer.setPaperSource(QPrinter::PaperSource(settings.value("paper_source").toInt()));
    else
        show_dialog = true;

    if (settings.contains("output_format"))
        printer.setOutputFormat(QPrinter::OutputFormat(settings.value("output_format").toInt()));
    else
        show_dialog = true;

    if (settings.contains("output_file_name"))
        printer.setOutputFileName(settings.value("output_file_name").toString());
    else
        show_dialog = true;

    if (settings.contains("resolution"))
        printer.setResolution(settings.value("resolution").toInt());
    else
        show_dialog = true;
    settings.endGroup();

    // This is to display correct paper size in the dialog box, it doesn't help.
    QPageSize initial_size(QSize(media_width_pt, media_height_pt), QPageSize::Point);
    printer.setPageSize(initial_size);

    if (show_dialog) {
        QPrintDialog print_dialog(&printer);
        print_dialog.setOption(QAbstractPrintDialog::PrintShowPageSize);
        if (print_dialog.exec() != QDialog::Accepted)
            return;
    }

    settings.beginGroup("printer");
    settings.setValue("color_mode", printer.colorMode());
    settings.setValue("paper_source", printer.paperSource());
    settings.setValue("output_format", printer.outputFormat());
    settings.setValue("output_file_name", printer.outputFileName());
    settings.setValue("resolution", printer.resolution());
    settings.endGroup();

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
        QRect media_box(0, 0, media_width_pt, is_roll ? bbox.height() : media_height_pt);
        int w_offset = (media_box.width() - bbox.width()) * horz_alignment / 2;
        int h_offset = (media_box.height() - bbox.height()) * vert_alignment / 2;

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
    save_params();
}

void PrintDialog::on_next_page_btn_clicked() {
    int n = pages.length();
    if (current_page < n - 1) {
        current_page++;
        update_page_controls();
        layout_preview();
    }
}

void PrintDialog::on_prev_page_btn_clicked() {
    if (current_page > 0) {
        current_page--;
        update_page_controls();
        layout_preview();
    }
}

bool PrintDialog::check_intersect(const QRect &arg) {
    for (const QRect &r : pages.last()) {
        if (r.intersects(arg))
            return true;
    }
    return false;
}

// Return value: -1 or 0..3 | (landscape << 3)
int PrintDialog::find_best_rect(int *pic_cnt, QList<QPoint> &bases, int *prev_height, int64_t *prev_area, int media_width_pt) {
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

void PrintDialog::init_new_page_rects(int w, int h) {
    pages.append(QList<QRect>());
    pages.last().append(QRect(w, 0, 1, h + 1)); // guard rectangles
    pages.last().append(QRect(0, h, w, 1));
}

void PrintDialog::repack() {

    if (wnd->pixmap.isNull())
        return;

    int pic_cnt[4];

    pic_cnt[0] = ui->print_16_20_spin->value();
    pic_cnt[1] = ui->print_11_14_spin->value();
    pic_cnt[2] = ui->print_8_10_spin->value();
    pic_cnt[3] = ui->print_5_7_spin->value();
    int tot = pic_cnt[0] + pic_cnt[1] + pic_cnt[2] + pic_cnt[3];
    int roll_height = ui->feed_type_combo->currentIndex() == 1 ? 10000 : media_height_pt;
//    int roll_height = media_height_pt; // for now

    QList<QPoint> bases;
    pages.clear();
    init_new_page_rects(media_width_pt, roll_height);

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
                media_error();
                return;
            }
            bases.clear();
            bases.append(QPoint(0,0));
            current_height = 0;
            current_area = 0;
            init_new_page_rects(media_width_pt, roll_height);
        }
    }
    current_page = 0;
    update_page_controls();
    layout_preview();
}

void PrintDialog::layout_preview() {
    if (wnd->pixmap.isNull() || pages.length() == 0)
        return;
    unerror();

    const QList<QRect> &page = pages[current_page];
    QRect bbox;
    for (int i=2; i < page.length(); i++) {
        if (i == 2)
            bbox = page[i];
        else
            bbox = bbox.united(page[i]);
    }
    QRect media_box(0, 0, media_width_pt, is_roll ? bbox.height() : media_height_pt);
    int w_offset = (media_box.width() - bbox.width()) * horz_alignment / 2;
    int h_offset = (media_box.height() - bbox.height()) * vert_alignment / 2;

    scene.clear();
    if (is_roll) {
        scene.setSceneRect(0, 0, media_width_pt, bbox.height());
        scene.addRect(0,0, media_width_pt, bbox.height(), QPen(Qt::transparent), QBrush(Qt::white));
    } else {
        scene.setSceneRect(0, 0, media_width_pt, media_height_pt);
        scene.addRect(0,0, media_width_pt, media_height_pt, QPen(Qt::transparent), QBrush(Qt::white));
    }

    QPixmap sel = wnd->scene.get_selection();
    QGraphicsPixmapItem *pm = new QGraphicsPixmapItem(sel);
    QRectF br = pm->boundingRect();

    for (int i=2; i < page.length(); i++) {
        QRect r = page[i];
        QGraphicsPixmapItem *pm = new QGraphicsPixmapItem(sel);
        int w = r.width();
        int h = r.height();
        if (w <= h) { // portrait
            QTransform t(w/br.width(), 0, 0, h/br.height(), r.x() + w_offset, r.y() + h_offset);
            pm->setTransform(t, true);
        } else { // landscape
            QTransform t(0, h/br.width(), w/br.height(),  0, r.x() + w_offset, r.y() + h_offset);
            pm->setTransform(t, true);
        }
        scene.addItem(pm);
        if (wnd->print_frames)
            scene.addRect(r.x() + w_offset, r.y() + h_offset, r.width(), r.height(), QPen(Qt::black), QBrush(Qt::transparent));
    }
    QColor background = QApplication::palette().color(QPalette::Window);
    ui->graphics_view->setBackgroundBrush(QBrush(background));
    ui->graphics_view->fitInView(0, 0, media_width_pt, media_height_pt, Qt::KeepAspectRatio);
    ui->graphics_view->show();
}


void PrintDialog::on_media_unit_combo_currentIndexChanged(int index) {
    if (in_progress)
        return;
    units = qMax(0, index);
    update_media_sizes();
}

void PrintDialog::on_horiz_alignment_combo_currentIndexChanged(int index) {
    if (in_progress)
        return;
    horz_alignment = qMax(0, index);
    layout_preview();
}

void PrintDialog::on_vert_alignment_combo_currentIndexChanged(int index) {
    if (in_progress)
        return;
    vert_alignment = qMax(0, index);
    layout_preview();
}

void PrintDialog::repacl_and_layout_preview() {
    repack();
    layout_preview();
}

void PrintDialog::on_feed_type_combo_currentIndexChanged(int index) {
    if (in_progress)
        return;
    is_roll = index == 1;
    ui->vert_alignment_combo->setEnabled(!is_roll);
    ui->vert_alignment_lbl->setEnabled(!is_roll);
    ui->media_height_lbl->setEnabled(!is_roll);
    ui->media_height_spin->setEnabled(!is_roll);
    repacl_and_layout_preview();
}

int PrintDialog::convert2points(double arg1) {
    double k;
    switch(units) {
    case 1: // mm
        k =  72. / 25.4;
        break;
    case 2:
        k = 72.;
        break;
    default:
        k = 1;
    }
    return int(arg1*k + 0.5);
}

void PrintDialog::on_media_width_spin_valueChanged(double arg1) {
    if (in_progress)
        return;
    media_width_pt = convert2points(arg1);
    repack();
}

void PrintDialog::on_media_height_spin_valueChanged(double arg1) {
    if (in_progress)
        return;
    media_height_pt = convert2points(arg1);
    repack();
}

void PrintDialog::media_error() {
    ui->errors_browser->show();
    ui->graphics_view->hide();
    if (!last_error_is_media_error) {
        ui->errors_browser->append("Media size is too small to fit the largest portrait.");
        last_error_is_media_error = true;
    }
}
void PrintDialog::error(const QString &msg) {
    ui->errors_browser->show();
    ui->graphics_view->hide();
    ui->errors_browser->append(msg);
    last_error_is_media_error = false;
}
void PrintDialog::unerror() {
    ui->errors_browser->hide();
    ui->graphics_view->show();
}

void PrintDialog::on_help_btn_clicked() {
    wnd->show_help("help.pdf");
}

void PrintDialog::on_print_guide_btn_clicked() {
    wnd->show_help("guide.pdf");
}

void PrintDialog::on_print_16_20_spin_textChanged(const QString &arg1) {
    Q_UNUSED(arg1)
    repack();
}

void PrintDialog::on_print_11_14_spin_textChanged(const QString &arg1) {
    Q_UNUSED(arg1)
    repack();
}

void PrintDialog::on_print_8_10_spin_textChanged(const QString &arg1) {
    Q_UNUSED(arg1)
    repack();
}

void PrintDialog::on_print_5_7_spin_textChanged(const QString &arg1) {
    Q_UNUSED(arg1)
    repack();
}
