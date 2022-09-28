#ifndef DOENLOADDIALOG_H
#define DOENLOADDIALOG_H

#include <QDialog>
#include <QNetworkReply>
#include <QDir>

namespace Ui {
class DownloadDialog;
}

class MainWindow;

class DownloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadDialog(QWidget *parent = nullptr);
    virtual ~DownloadDialog() override;

private slots:

    void onFinish();
    void onSslErrors(const QList<QSslError> &errors);
    void onReadyRead();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    void on_download_btn_clicked();

    void on_messages_btn_clicked();

    void on_help_btn_clicked();

private:
    QString gen_login_xml();
    QString gen_file_list_xml();
    QString gen_close_xml(const QString &token);
    QString gen_file_xml(const QString &file_id);
    void request_file_list ();
    void parsing_error(const QByteArray &buf, const char *msg, ...);
    void append_msg(const QString &msg);
    void amend_msg(const QString &msg);
    void process_post_data();
    void switch_file_message_widgets(bool show_files);

    Ui::DownloadDialog *ui;
    MainWindow *wnd;
    QString account_id;
    QString password;
    QNetworkAccessManager * network_access_mgr;
    QDir dir;
    QFile file;
    QNetworkReply *reply;
    QByteArray post_data;
    double approx_file_size;
    int file_count; // -1 == POST data
    bool show_files;
};

#endif // DOENLOADDIALOG_H
