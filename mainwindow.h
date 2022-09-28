#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QGraphicsScene>
#include "portraitscene.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow() override;
    void show_status(const QString &str);
    void parse_zhmur_dir();
    void show_help(const char *fname);
    bool is_file_listed(const QString &fname);
    void re_scene_current_job();

    QString account_id;
    QString password; // fixme: plaintext password
    QString portrait_dir;
    QString help_dir;

    bool show_menu;
    bool show_instructions;
    bool integrated_calibration;
    bool confirm_file_deletion;
    bool show_crop_frame;
    bool print_frames;
    bool advanced_print_dialog;

    int print_16_20;
    int print_11_14;
    int print_8_10;
    int print_5_7;

    PortraitScene scene;
    QPixmap pixmap;
protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_action_open_triggered();
    void on_action_settings_triggered();
    void on_open_btn_clicked();
    void on_settings_btn_clicked();
    void on_print_btn_clicked();
    void on_action_print_triggered();
    void on_exit_btn_clicked();
    void on_download_btn_clicked();

    void on_action_help_help_triggered();

    void on_action_help_about_triggered();

    void on_action_help_copying_triggered();

    void on_job_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_delete_btn_clicked();

    void on_action_download_Center_triggered();

    void on_action_delete_triggered();
private:
    void readSettings();
    void writeSettings();

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
