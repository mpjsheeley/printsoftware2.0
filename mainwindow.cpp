#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include "settingsdialog.h"
#include "printdialog.h"
#include "simpleprintdialog.h"
#include "downloaddialog.h"
#include <QSettings>
#include <QCloseEvent>
#include "aboutdialog.h"
#include <QGraphicsView>
#include <QMessageBox>
#include <QDesktopServices>


// Convert a string segment to int and check for errors.
static long long my_atoi(const QString &str, int from, int to) {
    long long acc = 0;
    while(from > to) {
        QChar c = str[from];
        if (!c.isDigit())
            return -1;
        acc = acc * 10 + c.digitValue();
        from++;
    }
    return acc;
}

// Identify and parsee the the string of the following format:
// John D. Zhmur_666_777.ext[.lnk]
// if not, just chop off .lnk
static bool parse_zhmur_fname(const QString &str, int *name_len, long long *id1, long long *id2) {
    int len = int(str.length());
    *id1 = *id2 = 0;
    if (len > 4) {
        if (str.endsWith(".lnk"))
            len -= 4;
        int i = len-1;
        int und1 = -1, und2=-1, dot=-1;
        while(--i >= 0) {
             QChar c = str[i];
             if (c == '.') {
                 if (dot < 0)
                     dot = i;
             } else if (c == '_') {
                 if (und2 < 0)
                     und2 = i;
                 else if (und1 < 0) {
                     und1 = i;
                     break;
                 }
             }
        }
        // If all parts exist and non-empty
        if (und1 >= 0 &&  dot >= 0 && dot > und2 + 1 && und1 + 1 > und2) {
            long long a = my_atoi(str, und1+1, und2);
            long long b = my_atoi(str, und2+1, dot);
            if (a >= 0 && b >= 0) {
                *id1 = a;
                *id2 = b;
                *name_len = und1;
                return true;
            }
        }
    }
    *name_len = len;
    return false;
}

bool MainWindow::is_file_listed(const QString &fname) {
    return !ui->job_list->findItems(fname, Qt::MatchExactly).isEmpty();
}

void MainWindow::parse_zhmur_dir() {
    QListWidget *lw = ui->job_list;
    lw->clear();
    QDir dir(portrait_dir);
    if (dir.exists() && dir.isReadable()) {
        QFileInfoList flist = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        // QIcon green_icon(":/images/downloadgreen.png"); // The icons were intended to show downloades status,
        // QIcon red_icon(":/images/downloadred.png");     // but all files in the list are already downloaded.
        for (const QFileInfo &info : flist) {              // So the icons serve no purpose.
            QString file_name = info.fileName();
            long long id1, id2;
            int zhmur_name_len;
            QListWidgetItem *item;
            if (parse_zhmur_fname(file_name, &zhmur_name_len, &id1, &id2))
                item = new QListWidgetItem(file_name.left(zhmur_name_len));
            else {
                //info.symLinkTarget()
                QFileInfo cinfo(info.canonicalFilePath());
                item = new QListWidgetItem(cinfo.fileName());
            }
            item->setData(Qt::UserRole, QVariant(info.canonicalFilePath()));  // file path
            item->setData(Qt::UserRole+1, QVariant(info.absoluteFilePath())); // symlink path
            item->setData(Qt::UserRole+2, QVariant(id1));
            item->setData(Qt::UserRole+3, QVariant(id2));
            lw->addItem(item);
        }
    }
    if (lw->count() > 0)
        lw->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , scene(this)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    readSettings();
    menuBar()->setVisible(show_menu);
    ui->graphics_view->setScene(&scene);
    ui->graphics_view->setInteractive(true);
    parse_zhmur_dir();
    setWindowIcon( QIcon(":/images/mplogo.png"));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    writeSettings();
    event->accept();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::readSettings() {
    QSettings settings;

    settings.beginGroup("main_window");
    resize(settings.value("size", size()).toSize());
    move(settings.value("pos", pos()).toPoint());
    ui->splitter->restoreState(settings.value("split").toByteArray());
    settings.endGroup();

    settings.beginGroup("settings");
    account_id = settings.value("account_id").toString();
    password = settings.value("password").toString(); // fixme: plaintext password
    portrait_dir = settings.value("portrait_dir", QString(".")).toString();
    help_dir = settings.value("help_dir", QString(".")).toString();

    show_menu = settings.value("show_menu").toBool();
    show_instructions = settings.value("show_instructions").toBool();
    integrated_calibration = settings.value("integrated_calibration").toBool();
    confirm_file_deletion = settings.value("confirm_del").toBool();
    show_crop_frame = settings.value("show_crop_frame").toBool();

    print_frames = settings.value("print_frames").toBool();
    advanced_print_dialog = settings.value("advanced_print_dialog").toBool();
    settings.endGroup();

    settings.beginGroup("printer");
    print_16_20 = settings.value("print_16x20").toInt();
    print_11_14 = settings.value("print_11x14").toInt();
    print_8_10 = settings.value("print_8x10").toInt();
    print_5_7 = settings.value("print_5x7").toInt();
    settings.endGroup();
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.beginGroup("main_window");
    settings.setValue("pos", QVariant(pos()));
    settings.setValue("size", QVariant(size()));
    settings.setValue("split", QVariant(ui->splitter->saveState()));
    settings.endGroup();

    settings.beginGroup("settings");
    settings.setValue("account_id", QVariant(account_id));
    settings.setValue("password", QVariant(password));
    settings.setValue("portrait_dir", QVariant(portrait_dir));
    settings.setValue("help_dir", QVariant(help_dir));
    settings.setValue("show_menu", QVariant(show_menu));
    settings.setValue("show_instructions", QVariant(show_instructions));
    settings.setValue("integrated_calibration", QVariant(integrated_calibration));
    settings.setValue("confirm_del", QVariant(confirm_file_deletion));
    settings.setValue("show_crop_frame", QVariant(show_crop_frame));
    settings.setValue("print_frames", print_frames);
    settings.setValue("advanced_print_dialog", advanced_print_dialog);
    settings.endGroup();

    settings.beginGroup("printer");
    settings.setValue("print_16x20", QVariant(print_16_20));
    settings.setValue("print_11x14", QVariant(print_11_14));
    settings.setValue("print_8x10", QVariant(print_8_10));
    settings.setValue("print_5x7", QVariant(print_5_7));
    settings.endGroup();
}

static bool is_removable(const QString &abs_path) {
    if (abs_path.startsWith("/media/")) // fixme: this works on Linux only
        return true;
    else
        return false;
}

void MainWindow::on_action_open_triggered()
{
    QSettings settings;
    settings.beginGroup("open");
    QString last_dir = settings.value("dir", ".").toString();
    settings.endGroup();
    QDir dir(portrait_dir);
    if (!dir.exists() || !dir.isReadable()) {
        QMessageBox msg(QMessageBox::Critical, QStringLiteral("Configuration problem")
                   ,QStringLiteral("The portrait directory does not exist or is not writable:\n")+portrait_dir
                   ,QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    QFileDialog file_dialog(this, "Select an Image File", last_dir, "Images (*.png *.bmp *.jpg);;All files (*.*)");
    file_dialog.setFileMode(QFileDialog::ExistingFiles);
    if (file_dialog.exec()) {
        const QStringList file_names = file_dialog.selectedFiles();
        int cnt = 0;
        for (const QString &file_name : file_names) {
            QFileInfo info(file_name);
            QFile file(file_name);
            if (is_removable(info.canonicalFilePath()))
                file.copy(dir.filePath(info.fileName()));  // fixme: error handling
            else {
#ifdef Q_OS_WIN
                file.link(dir.filePath(info.fileName()+".lnk"));
#else
                file.link(dir.filePath(info.fileName()));
#endif
            }

            if (cnt == 0) {
                settings.beginGroup("open");
                settings.setValue("dir", QVariant(info.absoluteDir().path()));
                settings.endGroup();
            }
            cnt++;
        }
        if (cnt) {
            parse_zhmur_dir();
        }
    }
}

void MainWindow::on_action_settings_triggered() {
    QString old_dir = portrait_dir;
    bool old_frame = show_crop_frame;

    SettingsDialog settings(this);
    settings.exec(); // everything is done in the dialog

    menuBar()->setVisible(show_menu);
    if (portrait_dir != old_dir)
        parse_zhmur_dir();
    else if (old_frame != show_crop_frame) {
        re_scene_current_job();
    }
}

void MainWindow::on_action_print_triggered() {
    if (advanced_print_dialog) {
        PrintDialog print(this);
        print.exec();
    } else {
        SimplePrintDialog print(this);
        print.exec();
    }
}

void MainWindow::on_open_btn_clicked() {
    on_action_open_triggered();
}

void MainWindow::on_settings_btn_clicked() {
    on_action_settings_triggered();
}

void MainWindow::on_print_btn_clicked() {
    on_action_print_triggered();
}


void MainWindow::on_exit_btn_clicked() {
    close();
}

void MainWindow::show_help(const char *fname) {
#ifdef Q_OS_WIN32
    const char *extra_slash = "/";
#else
    const char *extra_slash = "";
#endif
    QDesktopServices::openUrl(QUrl(QStringLiteral("file://%1%2/%3").arg(extra_slash, help_dir, fname)));
}

void MainWindow::on_action_help_help_triggered() {
    show_help("help.pdf");
}

void MainWindow::on_action_help_copying_triggered() {
    show_help("help.pdf");
}

void MainWindow::on_action_help_about_triggered() {
    AboutDialog about(this);
    about.exec();
}

void MainWindow::on_job_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
    Q_UNUSED(previous)
    if (current) {
        QString file_name = current->data(Qt::UserRole).toString();
        if (pixmap.load(file_name)) {
            scene.set_portrait_file(pixmap);
            ui->graphics_view->fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
            ui->graphics_view->show();
        } else {
            scene.clear(); // the file is unreadable
        }
    } else {
        scene.clear(); // no current item. the list is empty
    }
}

void MainWindow::re_scene_current_job() {
    QListWidgetItem *current = ui->job_list->currentItem();
    on_job_list_currentItemChanged(current, nullptr);
}


void MainWindow::show_status(const QString &str) {
    QStatusBar *bar = ui->statusbar;
    bar->showMessage(str, 1000);
}

void MainWindow::on_action_delete_triggered() {
    on_delete_btn_clicked();
}
void MainWindow::on_delete_btn_clicked() {
    QListWidget *lw = ui->job_list;
    QList<QListWidgetItem *> items = lw->selectedItems();
    if (items.length()) {
        if (confirm_file_deletion) {
            int len = items.length();
            QString msg = QStringLiteral("Please confirm deletion of the following file%1:\n").arg(len > 1 ? "s" : "");
            for (int i=0; i<len; i++) {
                const QListWidgetItem *item = items[i];
                QString fname = item->data(Qt::UserRole+1).toString();
                int last = fname.lastIndexOf('/');
                if (last > 0)
                    msg.append(fname.last(fname.length() - last - 1));
                else
                    msg.append(fname);
                if (i != len - 1)
                    msg.append(", ");
            }
            QMessageBox box(QMessageBox::Question, "Deleting files", msg, QMessageBox::Yes | QMessageBox::No, this);
            if (box.exec() == QMessageBox::No)
                return;
        }

        for (const QListWidgetItem *item : items ) {
            QString fname = item->data(Qt::UserRole+1).toString();
            if (!QFile::remove(fname))
                show_status(QString("Can't remove: ") + fname);
        }
        parse_zhmur_dir();
    }
}

void MainWindow::on_download_btn_clicked() {
    on_action_download_Center_triggered();
}
void MainWindow::on_action_download_Center_triggered() {
    DownloadDialog download(this);
    download.exec();
}

