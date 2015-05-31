#include "progressdialog.h"
#include "ui_progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::reject()
{
    // we don't treat reject event during issue fetch operations. yet.
}

void ProgressDialog::show()
{
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);

    QDialog::show();
}

void ProgressDialog::setLabel(const QString &label) { ui->label->setText(label); }

void ProgressDialog::setMaxValue(const int &value) { ui->progressBar->setMaximum(value); }

void ProgressDialog::setValue(const int &value) { ui->progressBar->setValue(value); }
