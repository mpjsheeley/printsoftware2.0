#ifndef SIMPLEPRINTDIALOG_H
#define SIMPLEPRINTDIALOG_H

class MainWindow;

#include <QDialog>

namespace Ui {
class SimplePrintDialog;
}

class SimplePrintDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SimplePrintDialog(QWidget *parent = nullptr);
    ~SimplePrintDialog();

private slots:
    void on_help_btn_clicked();

    void on_print_btn_clicked();

private:
    Ui::SimplePrintDialog *ui;
    MainWindow *wnd;
    QList<QList<QRect> > pages;

    bool check_intersect(const QRect &arg);
    int find_best_rect(int *pic_cnt, QList<QPoint> &bases, int *prev_height, int64_t *prev_area, int media_width_pt);
    void init_new_page_rects(int w, int h);
    bool repack(int media_width_pt, int media_height_pt);

};

#endif // SIMPLEPRINTDIALOG_H
