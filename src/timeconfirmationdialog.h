#ifndef TIMECONFIRMATIONDIALOG_H
#define TIMECONFIRMATIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QMap>
#include "redmine.h"

namespace Ui {
class TimeConfirmationDialog;
}

class TimeConfirmationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TimeConfirmationDialog(const double &time, const QString &issue_link, const QString &project_title, QWidget *parent = 0);
    ~TimeConfirmationDialog();

    void setActivitiesComboValues(const QList<Redmine::Activities> &values, const int &default_activity);
    int getActivity();
    double getTime();

private:
    Ui::TimeConfirmationDialog *ui;
};

#endif // TIMECONFIRMATIONDIALOG_H
