#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class MainWindow;
class QLineEdit;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    virtual ~SettingsDialog() override;

private slots:
    void on_portrait_dir_btn_clicked();

    void on_helpt_dir_btn_clicked();

    void on_ok_btn_clicked();

    void on_import_btn_clicked();

    void on_export_btn_clicked();

    void on_help_btn_clicked();

private:
    Ui::SettingsDialog *ui;
    MainWindow *wnd;

    void get_dir_param(QLineEdit *ed, const QString &dir, const QString &msg);
};

#endif // SETTINGSDIALOG_H
