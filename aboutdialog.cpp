#include <QSettings>
#include <QCloseEvent>
#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    QSettings settings;
    settings.beginGroup("about");
    if (settings.contains("size"))
        resize(settings.value("size", size()).toSize());
    if (settings.contains("pos"))
        move(settings.value("pos", pos()).toPoint());
    settings.endGroup();
}

void AboutDialog::closeEvent(QCloseEvent *event) {
    QSettings settings;
    settings.beginGroup("about");
    settings.setValue("pos", QVariant(pos()));
    settings.setValue("size", QVariant(size()));
    settings.endGroup();
    event->accept();
}

AboutDialog::~AboutDialog() {
    delete ui;
}
