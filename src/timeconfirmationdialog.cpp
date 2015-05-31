#include "timeconfirmationdialog.h"
#include "ui_timeconfirmation.h"

#include <QMapIterator>
#include <QDebug>

TimeConfirmationDialog::TimeConfirmationDialog(const double &time, const QString &issue_link, const QString &project_title, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TimeConfirmationDialog)
{
    ui->setupUi(this);
    ui->loggedTimeSpinBox->setValue(time);
    ui->issueLinkLabel->setText(issue_link);
    ui->projectDisplayLabel->setText(project_title);
}

TimeConfirmationDialog::~TimeConfirmationDialog()
{
    delete ui;
}

void TimeConfirmationDialog::setActivitiesComboValues(const QList<Redmine::Activities> &values, const int &default_activity)
{
    ui->activityComboBox->clear();

    foreach(const Redmine::Activities &a, values) {
        ui->activityComboBox->addItem(a.name, a.id);
        if (a.id == default_activity) {
            ui->activityComboBox->setCurrentText(a.name);
        }
    }
}

int TimeConfirmationDialog::getActivity()
{
    return ui->activityComboBox->currentData().toInt();
}

double TimeConfirmationDialog::getTime()
{
    return ui->loggedTimeSpinBox->value();
}
