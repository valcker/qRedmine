#ifndef REDMINE_H
#define REDMINE_H

#include <QObject>
#include <QNetworkRequest>
#include <QList>
#include <QMap>
#include <QJsonObject>

class QNetworkAccessManager;
class QNetworkReply;

class Redmine : public QObject
{
    Q_OBJECT
public:
    static int const RedmineIssueRequestLimit = 100;
    static int const RedmineTimeEntriesRequestLimit = 5;
    struct Activities {
        int id;
        QString name;
        bool is_default;
    };

    explicit Redmine(const QString &base_url, const QString &api_key, QObject *parent = 0);

    void setBasicAuth(const QString &login, const QString &pass) { ba_login = login; ba_pass = pass; }
    void setBasicAuthEnabled(bool enabled) { ba_enabled = enabled; }
    void setWatcherEnabled(bool watcher) { watcher_enabled = watcher; }
    void fetchActivities();
    void fetchMyIssues();
    void fetchAllIssues();
    void fetchTimeEntries();
    void postTimeEntry(const unsigned int &issue_id, const double &time, const int &activity_id);
    const QString & getBaseURL() { return base_url; }
    const QList<Activities> & getActivities() { return activities; }

signals:
    void issuesFetched(const QList<QJsonValue> &issues);
    void timeEntriesFetched(const QList<QJsonValue> &entries);
    void networkError(const QString &error);
    void issuesFetchProgress(const int &total, const int &offset);
    void timeEntriesFetchProgress(const int &total, const int &offset);
    void postTimeEntrySuccess();

private slots:
    void retrieveIssuesFinished();
    void retrieveTimeEntriesFinished();
    void postTimeEntryFinished();
    void fetchActivitiesFinished();
    void retrieveTimeEntries(int offset);

private:
    enum RetrieveIssuesStates {
        Assigned = 0,
        Watcher = 1,
        All = 2
    };

    static const QString REDMINE_TIME_ENTRIES_URL;

    QString getMyIssuesURL(bool watcher = false);
    QString getAllIssuesURL();
    void retrieveIssues(int offset = 0, bool all = false);

    QString base_url;
    QString api_key;
    QString ba_login;
    QString ba_pass;
    bool ba_enabled;
    bool watcher_enabled;
    QList<QJsonValue> retrieve_issues_buff;
    QList<QJsonValue> retrieve_time_entries_buff;
    int retrieve_issues_state;
    QList<Activities> activities;

    QNetworkAccessManager *network;
    QNetworkReply *retrieveIssuesReply;
    QNetworkReply *retrieveTimeEntriesReply;
    QNetworkReply *postTimeEntryReply;
    QNetworkReply *fetchActivitiesReply;
};

#endif // REDMINE_H
