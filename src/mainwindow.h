#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define APP_NAME "ProtoQt"

#include <QMainWindow>
#include <QString>
#include <QModelIndex>
#include <QSystemTrayIcon>
#include <QList>

#include "globals.h"

class QNetworkAccessManager;
class QNetworkReply;
class QSortFilterProxyModel;
class QStringListModel;
class QElapsedTimer;
class QTimer;
class QLabel;
class QAbstractItemModel;
class QAction;
class QMenu;
class QHideEvent;
class QShowEvent;
class QShortcut;
class QComboBox;
class QSignalMapper;
class QWidget;
class Redmine;
class ProgressDialog;

class IssuesModel;
class IssueSortFilterProxyModel;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *);

private slots:
    void slotGetIssuesFinished(const QList<QJsonValue> &issues);
    void slotGetIssuesProgress(const int &total, const int &offset);
    void slotFetchActivitiesFinished();
    void slotFetchTimeEntriesFinished(const QList<QJsonValue> &entries);
    void slotPostTimeEntrySuccess();
    void slotTimerTimeout();
    void slotOpenSettingsDialog();
    void slotTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void slotToggleVisibility();
    void slotShowComboboxPopup(QWidget *combo);
    void slotStartStopLogging();
    void slotOpenCurrentIssueUrl();
    void slotOpenSelectedIssueUrl();
    void slotAbout();
    void slotQuit();

    void on_comboFilterByProject_currentIndexChanged(const QString &arg1);
    void on_comboFilterByPriority_currentIndexChanged(const QString &arg1);
    void on_issueIdEdit_textChanged(const QString &arg1);
    void on_issueSubjectEdit_textChanged(const QString &arg1);
    void on_startStopLogButton_clicked(bool checked);

private:
    void createWidgets();
    void createActions();
    void createTrayIcon();
    void createShortcuts();
    void verifySettings();
    void writeSettings();
    void readSettings();
    void loadStyleSheet();

    void setHighlightCurrentItem(bool highlight);
    void setRecordLabelsVisible(bool visible);

    void startLogging(int id = 0);
    void stopLogging();
    void fetchIssues();

    void removeOldIssues(const QStringList &list);
    void updateProjectsFilter(QStringList &projects);
    int selectedIssueId() const;
    int currentIssueId() const;
    QModelIndex currentIssueIndex() const;
    QModelIndex safeCurrentViewIndex() const;

    struct IssueFetchStates {
        enum States {
            Idle = 0,
            Fetching = 1
        };
    };

    static const QString REDMINE_TIME_ENTRIES_URL;

    Ui::MainWindow *ui;
    Redmine *m_redmine;

    IssuesModel *m_issuesModel;
    IssueSortFilterProxyModel *m_proxyIssuesModel;
    QStringListModel *m_projectsModel;
    int m_currentIssueId;

    QElapsedTimer *m_loggedTimer;
    QTimer *m_timeLabelTimer;
    QLabel *m_issuesLoaderLabel;

    QTimer *m_refreshTimer;

    QAction *m_activeIssueAction;
    QAction *m_activeIssueTimeAction;
    QAction *m_refreshAction;
    QAction *m_restoreMinimizeAction;
    QAction *m_settingsAction;
    QAction *m_quitAction;

    QShortcut *m_quitShortcut;
    QShortcut *m_focusFilterProject;
    QShortcut *m_focusFilterPriority;
    QShortcut *m_focusFilterIssue;
    QShortcut *m_focusFilterSubject;
    QShortcut *m_focusIssueView;
    QShortcut *m_issuesViewKeyDown;
    QShortcut *m_issuesViewCtrlEnter;

    QSignalMapper *m_signalMapper;
    QMenu *m_trayIconMenu;
    QMenu *m_trayIconRecentIssuesMenu;
    QSystemTrayIcon *m_trayIcon;

    ProgressDialog *m_progress_dialog;

    int m_issuesFetchState;
};

#endif // MAINWINDOW_H
