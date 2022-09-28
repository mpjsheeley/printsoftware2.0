#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include "mainwindow.h"
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QSettings settings;
    settings.beginGroup("settings");
    resize(settings.value("size", size()).toSize());
    settings.endGroup();

    wnd = qobject_cast<MainWindow * >(parentWidget());

    ui->account_id_ed->setText(wnd->account_id);
    ui->password_ed->setText(wnd->password);
    ui->portrait_dir_ed->setText(wnd->portrait_dir);
    ui->help_dir_ed->setText(wnd->help_dir);

#if defined(Q_OS_MAC)
    ui->show_menu_check->setCheckState(Qt::Checked);
    ui->show_menu_check->setEnabled(false);
#else
    ui->show_menu_check->setCheckState(wnd->show_menu ? Qt::Checked : Qt::Unchecked);
#endif
    ui->show_instructions_check->setCheckState(wnd->show_instructions ? Qt::Checked : Qt::Unchecked);
    ui->confirm_file_deletion->setCheckState(wnd->confirm_file_deletion? Qt::Checked : Qt::Unchecked);
    ui->show_crop_frame->setCheckState(wnd->show_crop_frame? Qt::Checked : Qt::Unchecked);
    ui->advanced_print_check->setCheckState(wnd->advanced_print_dialog? Qt::Checked : Qt::Unchecked);
    ui->print_frames_check->setCheckState(wnd->print_frames? Qt::Checked : Qt::Unchecked);
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}

void SettingsDialog::get_dir_param(QLineEdit *ed, const QString &dir, const QString &msg) {
    QFileDialog file_dialog(this, msg, dir + "/..");
    file_dialog.setFileMode(QFileDialog::Directory);
    QString fname;
    int n = dir.lastIndexOf('/');
    if (n >= 0)
        fname = dir.last(dir.length() - n - 1);
    else
        fname = dir;
    file_dialog.selectFile(fname);
    if (file_dialog.exec()) {
        const QStringList sel = file_dialog.selectedFiles();
        ed->setText(sel[0]);
    }
}

void SettingsDialog::on_portrait_dir_btn_clicked() {
    get_dir_param(ui->portrait_dir_ed, wnd->portrait_dir, QStringLiteral("Select a Portrait directory"));
}

void SettingsDialog::on_helpt_dir_btn_clicked() {
    get_dir_param(ui->help_dir_ed, wnd->help_dir, QStringLiteral("Select a help file directory"));
}

void SettingsDialog::on_ok_btn_clicked() {
    wnd->account_id = ui->account_id_ed->text();
    wnd->password = ui->password_ed->text();
    wnd->portrait_dir = ui->portrait_dir_ed->text();
    wnd->help_dir = ui->help_dir_ed->text();
    wnd->show_menu = ui->show_menu_check->checkState() == Qt::Checked;
    wnd->show_instructions = ui->show_instructions_check->checkState() == Qt::Checked;
    wnd->confirm_file_deletion = ui->confirm_file_deletion->checkState() == Qt::Checked;
    wnd->show_crop_frame = ui->show_crop_frame->checkState() == Qt::Checked;
    wnd->advanced_print_dialog = ui->advanced_print_check->checkState() == Qt::Checked;
    wnd->print_frames = ui->print_frames_check->checkState() == Qt::Checked;

    QSettings settings;
    settings.beginGroup("settings");
    settings.setValue("size", QVariant(size()));
    settings.endGroup();
    accept();
}


void SettingsDialog::on_import_btn_clicked() {
    QString sel = QFileDialog::getOpenFileName(this, "Open configuration", ".", "All files (*.*)" );
    if (sel.isEmpty())
        return;
    QFile f(sel);
    if (!f.open(QIODeviceBase::ReadOnly)) {
        ui->status_lbl->setText(QStringLiteral("Can't open for reading: %1").arg(sel));
        return;
    }
    QDataStream d(&f);
    QMap<QString, QVariant> m;
    d >> m;

    if (d.status() == QDataStream::Ok)
        ui->status_lbl->setText("Configuration imported");
    else {
        ui->status_lbl->setText("Import failed");
        return;
    }
    QSettings s;
    QMapIterator<QString, QVariant> i(m);
    while (i.hasNext()) {
        i.next();
        s.setValue(i.key(), i.value());
    }
}

void SettingsDialog::on_export_btn_clicked() {
    QString sel = QFileDialog::getSaveFileName(this, "Save configuration as", ".", "All files (*.*)" );
    if (sel.isEmpty())
        return;
    QFile f(sel);
    if (!f.open(QIODeviceBase::WriteOnly)) {
        ui->status_lbl->setText(QStringLiteral("Can't open for writing: %1").arg(sel));
        return;
    }
    QDataStream d(&f);
    QSettings s;
    QStringList all_keys = s.allKeys();
    QMap<QString, QVariant> m;
    for (const QString &key : all_keys) {
        m[key] = s.value(key);
    }
    d << m;
    if (d.status() == QDataStream::Ok)
        ui->status_lbl->setText("Configuration exportrted");
    else
        ui->status_lbl->setText("Export failed");
}

void SettingsDialog::on_help_btn_clicked() {
    wnd->show_help("help.pdf");
}
