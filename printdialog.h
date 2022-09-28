    #ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#include <QDialog>
#include <QList>
#include <QPrinterInfo>
#include <QGraphicsScene>

namespace Ui {
class PrintDialog;
}

class MainWindow;

typedef enum {k_no_size=-1, k_16_20=0, k_11_14=1, k_8_10=2, k_5_7=3} PaperSize;

class PrintDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrintDialog(QWidget *parent = nullptr);
    ~PrintDialog();

private slots:
    void on_printer_combo_currentIndexChanged(int index);

    void on_print_now_btn_clicked();

    void on_print_dialog_btn_clicked();

    void on_print_guide_btn_clicked();

    void on_next_page_btn_clicked();

    void on_prev_page_btn_clicked();

    void on_media_unit_combo_currentIndexChanged(int index);

    void on_horiz_alignment_combo_currentIndexChanged(int index);

    void on_vert_alignment_combo_currentIndexChanged(int index);

    void on_feed_type_combo_currentIndexChanged(int index);

    void on_media_height_spin_valueChanged(double arg1);

    void on_media_width_spin_valueChanged(double arg1);

    void on_help_btn_clicked();

    void on_print_16_20_spin_textChanged(const QString &arg1);

    void on_print_11_14_spin_textChanged(const QString &arg1);

    void on_print_8_10_spin_textChanged(const QString &arg1);

    void on_print_5_7_spin_textChanged(const QString &arg1);

    void on_media_name_combo_currentIndexChanged(int index);

private:
    Ui::PrintDialog *ui;
    QList<QPrinterInfo> printer_list;
    int current_page;
    QList<QList<QRect> > pages;
    QGraphicsScene scene;
    int media_width_pt, media_height_pt;
    int units, is_roll;
    int horz_alignment, vert_alignment;
    bool in_progress; // ignore signals when update is in progress
    bool last_error_is_media_error;
    int printer_index;
    QString printer_model;
    QString paper_format;
    MainWindow *wnd;

    void save_params();
    void update_page_controls();
    void update_media_sizes();
    int find_best_rect(int *pic_cnt, QList<QPoint> &bases, int *base_height, int64_t *base_area, int media_width_pt);
    void init_new_page_rects(int w, int h);
    bool check_intersect(const QRect &arg);
    int convert2points(double arg1);
    void error(const QString &msg);
    void unerror();
    void media_error();
    void layout_preview();
    void repack();
    void do_print(bool show_dialog);
    void repacl_and_layout_preview();
    void printer_combo_currentIndexChanged(int index);
    void select_best_media_format(const QList<QPageSize> &page_sizes);

};

#endif // PRINTDIALOG_H
