#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
    class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();

    void reject();
    void show();
    void setLabel(const QString &label);
    void setMaxValue(const int &value);
    void setValue(const int &value);

private:
    Ui::ProgressDialog *ui;
};

#endif // PROGRESSDIALOG_H
