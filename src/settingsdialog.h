#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    void accept();
    void reject();
    int exec();
    
private slots:
    void on_basicAuthCheckBox_toggled(bool checked);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
