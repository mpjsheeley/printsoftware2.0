#include "downloaddialog.h"
#include "ui_downloaddialog.h"
#include <QTableWidget>
#include "mainwindow.h"
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QByteArray>
#include <QDate>
#include <QDir>
#include "mainwindow.h"
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <stdio.h>
#include <stdlib.h>

DownloadDialog::DownloadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DownloadDialog)
{
    ui->setupUi(this);
    wnd = qobject_cast<MainWindow * >(parent);

    QTableWidget *t = ui->table_widget;
    QHeaderView *hh = t->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::QHeaderView::Stretch);
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    show_files = false; // to force an update in the next line
    switch_file_message_widgets(true);

    MainWindow *wnd = qobject_cast<MainWindow * >(parentWidget());
    account_id = wnd->account_id;
    password = wnd->password;

    network_access_mgr = new QNetworkAccessManager(this);
    dir.setPath(wnd->portrait_dir);

    QSettings settings;
    settings.beginGroup("download");
    resize(settings.value("size", size()).toSize());
    settings.endGroup();

    request_file_list();
}

DownloadDialog::~DownloadDialog()
{
    network_access_mgr->deleteLater();
    delete ui;
}

static const struct {
    enum QNetworkReply::NetworkError num;
    const char * const txt;
} einfo[] = {
    {QNetworkReply::ConnectionRefusedError, "the remote server refused the connection (the server is not accepting requests)"},
    {QNetworkReply::RemoteHostClosedError, "the remote server closed the connection prematurely, before the entire reply was received and processed"},
    {QNetworkReply::HostNotFoundError, "the remote host name was not found (invalid hostname)"},
    {QNetworkReply::TimeoutError, "the connection to the remote server timed out"},
    {QNetworkReply::OperationCanceledError, "the operation was canceled via calls to abort() or close() before it was finished."},
    {QNetworkReply::SslHandshakeFailedError, "the SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() signal should have been emitted."},
    {QNetworkReply::TemporaryNetworkFailureError, "the connection was broken due to disconnection from the network, however the system has initiated roaming to another access point. The request should be resubmitted and will be processed as soon as the connection is re-established."},
    {QNetworkReply::NetworkSessionFailedError, "the connection was broken due to disconnection from the network or failure to start the network."},
    {QNetworkReply::BackgroundRequestNotAllowedError, "the background request is not currently allowed due to platform policy."},
    {QNetworkReply::TooManyRedirectsError, "while following redirects, the maximum limit was reached. The limit is by default set to 50 or as set by QNetworkRequest::setMaxRedirectsAllowed(). (This value was introduced in 5.6.)"},
    {QNetworkReply::InsecureRedirectError, "while following redirects, the network access API detected a redirect from a encrypted protocol (https) to an unencrypted one (http). (This value was introduced in 5.6.)"},
    {QNetworkReply::ProxyConnectionRefusedError, "the connection to the proxy server was refused (the proxy server is not accepting requests)"},
    {QNetworkReply::ProxyConnectionClosedError, "the proxy server closed the connection prematurely, before the entire reply was received and processed"},
    {QNetworkReply::ProxyNotFoundError, "the proxy host name was not found (invalid proxy hostname)"},
    {QNetworkReply::ProxyTimeoutError, "the connection to the proxy timed out or the proxy did not reply in time to the request sent"},
    {QNetworkReply::ProxyAuthenticationRequiredError, "the proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)"},
    {QNetworkReply::ContentAccessDenied, "the access to the remote content was denied (similar to HTTP error 403)"},
    {QNetworkReply::ContentOperationNotPermittedError, "the operation requested on the remote content is not permitted"},
    {QNetworkReply::ContentNotFoundError, "the remote content was not found at the server (similar to HTTP error 404)"},
    {QNetworkReply::AuthenticationRequiredError, "the remote server requires authentication to serve the content but the credentials provided were not accepted (if any)"},
    {QNetworkReply::ContentReSendError, "the request needed to be sent again, but this failed for example because the upload data could not be read a second time."},
    {QNetworkReply::ContentConflictError, "the request could not be completed due to a conflict with the current state of the resource."},
    {QNetworkReply::ContentGoneError, "the requested resource is no longer available at the server."},
    {QNetworkReply::InternalServerError, "the server encountered an unexpected condition which prevented it from fulfilling the request."},
    {QNetworkReply::OperationNotImplementedError, "the server does not support the functionality required to fulfill the request."},
    {QNetworkReply::ServiceUnavailableError, "the server is unable to handle the request at this time."},
    {QNetworkReply::ProtocolUnknownError, "the Network Access API cannot honor the request because the protocol is not known"},
    {QNetworkReply::ProtocolInvalidOperationError, "the requested operation is invalid for this protocol"},
    {QNetworkReply::UnknownNetworkError, "an unknown network-related error was detected"},
    {QNetworkReply::UnknownProxyError, "an unknown proxy-related error was detected"},
    {QNetworkReply::UnknownContentError, "an unknown error related to the remote content was detected"},
    {QNetworkReply::ProtocolFailure, "a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)"},
    {QNetworkReply::UnknownServerError, "an unknown error related to the server response was detected"}
};


static const char xml_header[] = "xml=<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
static const char mem_url[] = "https://www.memoryportraits.com/mp_printer/mp_printer.php";
//static const char mem_url[] = "http://192.168.1.195:8880/some/path";

QString DownloadDialog::gen_login_xml() {
  return QStringLiteral("%1<MPXML><LOGINREQUEST><ACCOUNTID>%2</ACCOUNTID><PASSWORD>%3</PASSWORD></LOGINREQUEST></MPXML>\n").arg(xml_header, account_id, password);
}

QString DownloadDialog::gen_file_list_xml() {
  return QStringLiteral("%1\n<MPXML><FILELISTREQUEST><ACCOUNTID>%2</ACCOUNTID><PASSWORD>%3</PASSWORD></FILELISTREQUEST></MPXML>\n").arg(xml_header, account_id, password);
}

QString DownloadDialog::gen_close_xml(const QString &token) {
  return QStringLiteral("%1\n<MPXML><CLOSELOGIN><ACCOUNTID>%2</ACCOUNTID><TOKEN>%3</TOKEN></CLOSELOGIN></MPXML>\n").arg(xml_header, account_id, token);
}

QString DownloadDialog::gen_file_xml(const QString &file_id) {
  return QStringLiteral("%1\n<MPXML><MARKFILEREQUEST><ACCOUNTID>%2</ACCOUNTID><PASSWORD>%3</PASSWORD><FILEID>%4</FILEID></MARKFILEREQUEST></MPXML>\n").arg(xml_header, account_id, password, file_id);
}


static const char * find_tag(bool *is_empty, const char *buf, const char *tag) {
    size_t len = strlen(tag);
    if (len > 2 && tag[0] == '<' && tag[len-1] == '>' && tag[1] != '/') {
        char *nop = (char *)alloca(len);
        memcpy(nop, tag, len-1);
        nop[len-1] = 0;
        const char *p = strstr(buf, nop);
        if (!p)
            return nullptr;
        p += len-1;
        if (p[0] == '>') {
            *is_empty =  false;
            return p + 1;
        } else if (p[0] == '/' && p[1] == '>') {
            *is_empty =  true;
            return p + 2;
        } else {
            return nullptr;
        }
    } else {
        const char *p = strstr(buf, tag);
        if (!p)
            return nullptr;
        *is_empty =  false;
        return p + len;
    }
}

// Length of the segment that follows the first '.' after last '/' .
static int ext_len(const QString &s) {
    int dot = int(s.length()); // virtual dot after the end of the string
    for (int i = dot - 1; i >= 0; i--) {
        if (s[i] == '/')
            break;
        if (s[i] == '.')
            dot = i;
    }
    return int(s.length()) - dot;
}

static QString make_fname(const QString &subj_name, const QString &url_str, const QString &order_id, const QString &file_id) {
    int ext = ext_len(url_str);
    return QString("%1_%2_%3%4").arg(subj_name, order_id, file_id, url_str.right(ext));
}

void DownloadDialog::parsing_error(const QByteArray &buf, const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    char temp[160];
    vsprintf(temp, fmt, v);
    QString text(temp);
    text.append("\n");
    text.append(buf.constData());
    ui->debug_text_ed->setPlainText(text);
    va_end(v);
}

void DownloadDialog::process_post_data() {
    /* // expected response if a file is available
    <?xml version="1.0" encoding="utf-8"?>
    <MPXML>
      <FILELISTRESPONSE>
        <FILE>
          <ORDERID>754260</ORDERID>
          <SUBJECTNAME><![CDATA[Printing Software Test Image]]></SUBJECTNAME>
          <IMAGEURL>https://orderform.memoryportraits.com/customer_files/904772.tif</IMAGEURL>
          <FILESIZE>5.27</FILESIZE>
          <FILEID>904772</FILEID>
          <DATEUPLOADED>06/01/2022</DATEUPLOADED>
          <DATEEXPIRES>06/15/2022</DATEEXPIRES>
          <DATEDOWNLOADED>06/11/2022</DATEDOWNLOADED>
        </FILE>
      </FILELISTRESPONSE>
    </MPXML>
    */

    /* // expected response if a file was not downloaded
    <?xml version="1.0" encoding="utf-8"?>
    <MPXML>
      <FILELISTRESPONSE>
        <FILE>
          <ORDERID>754260</ORDERID>
          <SUBJECTNAME><![CDATA[Printing Software Test Image]]></SUBJECTNAME>
          <IMAGEURL>https://orderform.memoryportraits.com/customer_files/911412.tif</IMAGEURL>
          <FILESIZE>5.27</FILESIZE>
          <FILEID>911412</FILEID>
          <DATEUPLOADED>07/18/2022</DATEUPLOADED>
          <DATEEXPIRES>08/01/2022</DATEEXPIRES>
          <DATEDOWNLOADED/>
        </FILE>
      </FILELISTRESPONSE>
    </MPXML>
    */

    /* // expected response if no files are available
    <?xml version="1.0" encoding="utf-8"?>
    <MPXML>
      <FILELISTRESPONSE/>
    </MPXML>
    */

    /* // expected response if login is incorrect
    <?xml version="1.0" encoding="utf-8"?>
    <MPXML>
      <ERROR msg="Login Incorrect"/>
    </MPXML>
    */

    const char *p = post_data.constData();
    bool is_empty;
    p = find_tag(&is_empty, p, "<FILELISTRESPONSE>");
    if (!p) {
        if (strstr(post_data.constData(), "<ERROR msg=\"Login Incorrect\"/>")) {
            append_msg("The account number or password is incorrect.");
        } else {
            parsing_error(post_data, "Cannot find <FILELISTRESPONSE>");
        }
        return;
    }
    if (is_empty) {
        append_msg("There is no files available for download.");
        return;
    }
    for(;;) {
        long long order_id;
        int file_cnt = 1; // one-based for error messages
        double filesize;
        long long fileid;
        int uploaded_month, uploaded_day, uploaded_year;
        int expires_month, expires_day, expires_year;
        int downloaded_month, downloaded_day, downloaded_year;

        static const char file_has_no_tag[] = "The file entry %d has no %s tag.";
        static const char file_has_bad_value[] = "The file entry %d has a malformed value under %s tag.";
        const char *file_p = find_tag(&is_empty, p, "<FILE>");
        if (!file_p)
            break;
        if (is_empty)
            continue;
        const char *xfile_p = find_tag(&is_empty, file_p, "</FILE>");
        if (!xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "</FILE>");
            return;
        }

        const char *orderid_p = find_tag(&is_empty, file_p, "<ORDERID>");
        if (!orderid_p || orderid_p > xfile_p ) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<ORDERID>");
            return;
        }
        if (sscanf(orderid_p, "%lld", &order_id) != 1) {
            parsing_error(post_data, file_has_bad_value, file_cnt, "<ORDERID>");
            return;
        }

        const char *subject_name_p = find_tag(&is_empty, file_p, "<SUBJECTNAME>");
        if (!subject_name_p || subject_name_p > xfile_p ) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<SUBJECTNAME>");
            return;
        }
        const char *cdata_p = find_tag(&is_empty, subject_name_p, "<![CDATA[");
        if (!cdata_p || cdata_p > xfile_p) {
            parsing_error(post_data, "The file entry %d has <SUBJECTNAME> without CDATA tag but this parser requites it.", file_cnt);
            return;
        }
        const char *xcdata_p = strstr(cdata_p, "]]");
        if (!xcdata_p || xcdata_p > xfile_p) {
            parsing_error(post_data, "The file entry %d has CDATA tag but but no ]]", file_cnt);
            return;
        }
        QString subject_name(QByteArray(cdata_p, xcdata_p-cdata_p));

        const char *imageurl_p = find_tag(&is_empty, file_p, "<IMAGEURL>");
        if (!imageurl_p || imageurl_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<IMAGEURL>");
            return;
        }
        const char *ximageurl_p = strstr(imageurl_p, "</IMAGEURL>");
        if (!ximageurl_p || ximageurl_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "</IMAGEURL>");
            return;
        }
        QString image_url(QByteArray(imageurl_p, ximageurl_p-imageurl_p));

        const char *filesize_p = find_tag(&is_empty, file_p, "<FILESIZE>");
        if (!filesize_p || filesize_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<FILESIZE>");
            return;
        }
        if (sscanf(filesize_p, "%lf", &filesize) != 1) {
            parsing_error(post_data, file_has_bad_value, file_cnt, "<FILESIZE>");
            return;
        }

        const char *fileid_p = find_tag(&is_empty, file_p, "<FILEID>");
        if (!fileid_p || fileid_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<FILEID>");
            return;
        }
        if (sscanf(fileid_p, "%lld", &fileid) != 1) {
            parsing_error(post_data, file_has_bad_value, file_cnt, "<FILEID>");
            return;
        }

        const char *dateuploaded_p = find_tag(&is_empty, file_p, "<DATEUPLOADED>");
        if (!dateuploaded_p || dateuploaded_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<DATEUPLOADED>");
            return;
        }
        if (sscanf(dateuploaded_p, "%d/%d/%d", &uploaded_month, &uploaded_day, &uploaded_year) != 3
                || uploaded_month > 12 || uploaded_day > 31 || uploaded_year > 2050 || uploaded_month <= 0 || uploaded_day  <= 0 || uploaded_year <= 2000) {
            parsing_error(post_data, file_has_bad_value, file_cnt, "<DATEUPLOADED>");
            return;
        }

        const char *dateexpires_p = find_tag(&is_empty, file_p, "<DATEEXPIRES>");
        if (!dateexpires_p || dateexpires_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<DATEEXPIRES>");
            return;
        }
        if (sscanf(dateexpires_p, "%d/%d/%d", &expires_month, &expires_day, &expires_year) != 3
               || expires_month > 12 || expires_day > 31 || expires_year > 2050 || expires_month <= 0 || expires_day  <= 0 || expires_year <= 2000) {
            parsing_error(post_data, file_has_bad_value, file_cnt, "<DATEEXPIRES>");
            return;
        }

        const char *datedownloaded_p = find_tag(&is_empty, file_p, "<DATEDOWNLOADED>");
        if (!datedownloaded_p || datedownloaded_p > xfile_p) {
            parsing_error(post_data, file_has_no_tag, file_cnt, "<DATEDOWNLOADED>");
            return;
        }
        if (is_empty)
            downloaded_month = downloaded_day  = downloaded_year = 0;
        else {
            if (sscanf(datedownloaded_p, "%d/%d/%d", &downloaded_month, &downloaded_day, &downloaded_year) != 3
                   || downloaded_month > 12 || downloaded_day > 31 || downloaded_year > 2050 || downloaded_month <= 0 || downloaded_day  <= 0 || downloaded_year <= 2000) {
                parsing_error(post_data, file_has_bad_value, file_cnt, "<DATEDOWNLOADED>");
                return;
            }
        }
        p = xfile_p;
        QTableWidget *t = ui->table_widget;
        int row = t->rowCount();
        t->setRowCount(row + 1);
        QTableWidgetItem *twi;

        QString fname = make_fname(subject_name, image_url, QString::number(order_id), QString::number(fileid));
        twi = new QTableWidgetItem(subject_name);
        twi->setData(Qt::UserRole, image_url);
        twi->setCheckState(wnd->is_file_listed(fname) ? Qt::Unchecked : Qt::Checked);
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 0, twi);

        twi = new QTableWidgetItem(QString::number(order_id));
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 1, twi);

        twi = new QTableWidgetItem(QStringLiteral("%1 MB").arg(filesize));
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 2, twi);

        twi = new QTableWidgetItem(QString::number(fileid));
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 3, twi);

        QDate date;
        date.setDate(uploaded_year, uploaded_month, uploaded_day);
        twi = new QTableWidgetItem(date.toString("MM/dd/yyyy"));
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 4, twi);

        date.setDate(downloaded_year, downloaded_month, downloaded_day);
        twi = new QTableWidgetItem(date.toString("MM/dd/yyyy"));
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 5, twi);

        date.setDate(expires_year, expires_month, expires_day);
        twi = new QTableWidgetItem(date.toString("MM/dd/yyyy"));
        twi->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        t->setItem(row, 6, twi);
    }
    switch_file_message_widgets(true);
}


void DownloadDialog::onReadyRead()
{
    // This slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file_count < 0) {
        post_data.append(reply->readAll());
    } else {
        if (file.isOpen())
            file.write(reply->readAll());
    }
}

void DownloadDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (file_count >= 0) {
        if (bytesTotal <= 0) {
            QTableWidget *t = ui->table_widget;
            QStringList file_size_s = t->item(file_count, 2)->text().split(' ', Qt::SkipEmptyParts);
            if (file_size_s.length() >= 1) {
                double file_size_f = file_size_s[0].toDouble();
                if (file_size_s.length() >= 2) {
                    if (file_size_s[1] == "MB")
                        bytesTotal = qint64(file_size_f * 1024 * 1924 + 0.5);
                    else if (file_size_s[1] == "KB")
                        bytesTotal = qint64(file_size_f * 1924 + 0.5);
                }
            }
        }
        if (bytesTotal > 0) {
            int percent = int((bytesReceived * 100) / bytesTotal);
            amend_msg(QStringLiteral("Downloaded... %1%").arg(percent));
        }
    }
}

void DownloadDialog::onFinish()
{
    QNetworkReply::NetworkError error = reply->error();
    if (error) {
        QString prev;
        const char *msg = "unlisted error";
        for (unsigned i=0; i < sizeof(einfo)/sizeof(*einfo); i++) {
            if (einfo[i].num == error)
                msg = einfo[i].txt;
        }
        prev += QStringLiteral("\nError #%1, %2.\n").arg(error).arg(msg);
        prev += reply->errorString();
        ui->debug_text_ed->setPlainText(prev);
    } else {
        onReadyRead();
        if (file_count < 0) {
            append_msg(QStringLiteral("Received... 100%"));
            process_post_data();
        } else {
            QTimer::singleShot(451, Qt::CoarseTimer, this, &DownloadDialog::on_download_btn_clicked);
        }
    }
    reply->deleteLater();
}

void DownloadDialog::onSslErrors(const QList<QSslError> &errors) {
    for (const QSslError &e : errors) {
        QString es = e.errorString();
        if (!es.isEmpty())
            append_msg(es);
    }
}

// This is a query captured from a [hacked] MemoryPortraits v. 1.1:
//POST /sssom/mp_printer/mp_printer.php HTTP/1.1
//User-Agent: Java/1.8.0_333
//Host: 192.168.1.195:8880
//Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2
//Connection: keep-alive
//Content-type: application/x-www-form-urlencoded
//Content-Length: 139
//
//xml=<?xml version="1.0" encoding="UTF-8"?><MPXML><LOGINREQUEST><ACCOUNTID>9000</ACCOUNTID><PASSWORD>9000</PASSWORD></LOGINREQUEST></MPXML>


// FYI: SSL is implemented in libeay32.dll and ssleay32.dll . They must be loadable.

void DownloadDialog::request_file_list() {

    ui->table_widget->hide();
    ui->debug_text_ed->show();
    if (!QSslSocket::supportsSsl()) {
        QString s("Connection to the server is not possible because SSL is not available on the client.\n"
                  "Please contact technical support.\n");
        s.append(QStringLiteral("SSL library build version: %1\n").arg(QSslSocket::sslLibraryBuildVersionString()));
        s.append(QStringLiteral("SSL library run-time version: %1\n").arg(QSslSocket::sslLibraryVersionString()));
        ui->debug_text_ed->setPlainText(s);
        return;
    }

    if (!dir.exists() || !dir.isReadable()) { // WTF there's no isWritable()
        ui->debug_text_ed->setPlainText(QStringLiteral("Portrait directory: %1\n"
                                                "does not exist or isn't accessible.\n"
                                                "Open the Settings dialog and select an existing directory.\n").arg(dir.path()));
        return;
    }

    ui->debug_text_ed->setPlainText("Requesting the list of available files...");
    post_data.clear();
    file_count = -1;

    QUrl url = QUrl(mem_url);
    QNetworkRequest request(url);
    // The server wants this type, although the message body is just plain text.
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    reply = network_access_mgr->post(request, gen_login_xml().toUtf8());
    connect(reply, &QNetworkReply::finished, this, &DownloadDialog::onFinish);
    connect(reply, &QIODevice::readyRead, this, &DownloadDialog::onReadyRead);
    connect(reply, &QNetworkReply::downloadProgress, this, &DownloadDialog::onDownloadProgress);
    connect(reply, &QNetworkReply::sslErrors, this, &DownloadDialog::onSslErrors);
}

void DownloadDialog::append_msg(const QString &msg) {
    if (msg.isEmpty())
        return;
    switch_file_message_widgets(false);
    ui->debug_text_ed->appendPlainText(msg);
    if (msg[msg.length() - 1] != '\n')
        ui->debug_text_ed->appendPlainText("\n");
}

void DownloadDialog::amend_msg(const QString &msg) {
    switch_file_message_widgets(false);
    QString text = ui->debug_text_ed->toPlainText();
    int i = int(text.length()) - 1;
    while (i >=0) {
        if (text[i] == '\n')
            break;
        i--;
    }
    text.replace(i+1, text.length() - i - 1, msg);
    ui->debug_text_ed->setPlainText(text);
}

void DownloadDialog::on_download_btn_clicked() {
    QSettings settings;
    settings.beginGroup("download");
    settings.setValue("size", QVariant(size()));
    settings.endGroup();

    switch_file_message_widgets(false);

    QTableWidget *t = ui->table_widget;
    // Pick the 1st selected file and queue a re-run if there are more files.
    file_count = -1;
    for (int i=0; i < t->rowCount(); i++) {
        if (t->item(i, 0)->checkState() == Qt::Checked) {
            file_count = i;
            break;
        }
    }

    if (file_count >= 0) {
        int i = file_count;
        t->item(i, 0)->setCheckState(Qt::Unchecked);
        file_count = i;
        t->item(i, 5)->setText("Now");
        QString subj_name = t->item(i, 0)->text();
        QString url_str = t->item(i, 0)->data(Qt::UserRole).toString();
        QString order_id = t->item(i, 1)->text();
        QString file_id = t->item(i, 3)->text();
        QString fname = make_fname(subj_name, url_str, order_id, file_id);
        QString path_fname = dir.filePath(fname);
        file.setFileName(path_fname);
        if (!file.open(QIODeviceBase::WriteOnly)) {
            append_msg(QStringLiteral("Error opening file for writing: ") + path_fname);
            return;
        }
        QUrl url = QUrl(url_str);
        QNetworkRequest request(url);
        append_msg(QStringLiteral("Downloading %1").arg(fname));

        reply = network_access_mgr->get(request);
        connect(reply, &QNetworkReply::finished, this, &DownloadDialog::onFinish);
        connect(reply, &QIODevice::readyRead, this, &DownloadDialog::onReadyRead);
        connect(reply, &QNetworkReply::downloadProgress, this, &DownloadDialog::onDownloadProgress);
        connect(reply, &QNetworkReply::sslErrors, this, &DownloadDialog::onSslErrors);
    } else {
        switch_file_message_widgets(true);
        wnd->parse_zhmur_dir();
    }
}

void DownloadDialog::switch_file_message_widgets(bool show_files) {
    if (show_files == this->show_files)
        return;
    this->show_files = show_files;
    if (show_files) {
        ui->table_widget->show();
        ui->debug_text_ed->hide();
        ui->messages_btn->setText("Messages");
    } else {
        ui->table_widget->hide();
        ui->debug_text_ed->show();
        ui->messages_btn->setText("Files");
    }
}

void DownloadDialog::on_messages_btn_clicked() {
    switch_file_message_widgets(!show_files);
}

void DownloadDialog::on_help_btn_clicked() {
    wnd->show_help("help.pdf");
}
