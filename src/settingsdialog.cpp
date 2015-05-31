#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QUrl>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    if (!QUrl::fromUserInput(ui->redmineUrlEdit->text()).isValid() || ui->redmineApiKeyEdit->text().isEmpty()) {
        QMessageBox::critical(this, tr("Validation error"), tr("Please enter correct Redmine URL and authentication credentials"), QMessageBox::Ok);
    }
    else {
        QSettings settings;
        settings.beginGroup("redmine connection");

        settings.setValue("url", QUrl::fromUserInput(ui->redmineUrlEdit->text()).toString(QUrl::RemovePath | QUrl::StripTrailingSlash));
        settings.setValue("api_key", ui->redmineApiKeyEdit->text());

        settings.setValue("basic_auth_enabled", ui->basicAuthCheckBox->isChecked());
        settings.setValue("basic_auth_login", ui->basicAuthLoginEdit->text());
        settings.setValue("basic_auth_pass", ui->basicAuthPassEdit->text());

        settings.endGroup();

        QDialog::accept();
    }
}

void SettingsDialog::reject()
{
    // TODO: replace that with something more clever
    QDialog::reject();
}

int SettingsDialog::exec()
{
    QSettings settings;
    settings.beginGroup("redmine connection");

    // api key and url fields
    ui->redmineUrlEdit->setText(settings.value("url", "https://your.redmine.com").toString());
    ui->redmineApiKeyEdit->setText(settings.value("api_key", "").toString());

    // basic auth fields
    ui->basicAuthCheckBox->setChecked(settings.value("basic_auth_enabled", false).toBool());
    ui->basicAuthLoginEdit->setText(settings.value("basic_auth_login").toString());
    ui->basicAuthPassEdit->setText(settings.value("basic_auth_pass").toString());

    settings.endGroup();

    return QDialog::exec();
}

void SettingsDialog::on_basicAuthCheckBox_toggled(bool checked)
{
    ui->basicAuthLoginEdit->setEnabled(checked);
    ui->basicAuthPassEdit->setEnabled(checked);
}
