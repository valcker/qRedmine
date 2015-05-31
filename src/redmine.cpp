#include "redmine.h"
#include "redminerequest.h"

#include <QtNetwork>
#include <QtCore>

const QString Redmine::REDMINE_TIME_ENTRIES_URL = "/time_entries.json";

Redmine::Redmine(const QString &base_url, const QString &api_key, QObject *parent) :
    QObject(parent)
{
    this->base_url = base_url;
    this->api_key = api_key;
    network = new QNetworkAccessManager(this);
}

void Redmine::fetchActivities()
{
    QUrl url = base_url + "/enumerations/time_entry_activities.json";
    RedmineRequest request(url, api_key);
    if (ba_enabled) {
        request.addBasicAuthHeader(ba_login, ba_pass);
    }

    fetchActivitiesReply = network->get(request);
    fetchActivitiesReply->ignoreSslErrors();
    connect(fetchActivitiesReply, SIGNAL(finished()), this, SLOT(fetchActivitiesFinished()));
}

void Redmine::fetchMyIssues()
{
    retrieve_issues_state = Assigned;
    retrieve_issues_buff.clear();
    qDebug() << "Redmine: getMyIssues with watcher_enabled == " << watcher_enabled;
    retrieveIssues();
}

void Redmine::fetchAllIssues()
{
    retrieve_issues_state = All;
    retrieve_issues_buff.clear();
    qDebug() << "Redmine: getAllIssues";
    retrieveIssues(0, true);
}

void Redmine::fetchTimeEntries()
{
    retrieve_time_entries_buff.clear();
    qDebug() << "Redmine: fetchLoggedHours";
    retrieveTimeEntries(0);
}

void Redmine::postTimeEntry(const unsigned int &issue_id, const double &time, const int &activity_id)
{
    qDebug() << "Redmine: posting time entry for issue:" << issue_id << " and entry: " << time;

    QUrl url = base_url + REDMINE_TIME_ENTRIES_URL;
    RedmineRequest request(url, api_key);
    request.setRawHeader("Content-Type", "application/json");

    if (ba_enabled) {
        request.addBasicAuthHeader(ba_login, ba_pass);
    }

    // preparing json array
    QJsonObject entry;
    QJsonObject entry_value;
    QJsonDocument document;
    entry_value.insert("issue_id", QJsonValue((int)issue_id));
    entry_value.insert("activity_id", QJsonValue(activity_id));
    entry_value.insert("hours", QJsonValue(time));
    entry.insert("time_entry", QJsonValue(entry_value));
    document.setObject(entry);

    qDebug() << "Redmine: posting time entry: " << document.toJson();

    postTimeEntryReply = network->post(request, document.toJson());
    postTimeEntryReply->ignoreSslErrors();

    QObject::connect(postTimeEntryReply, SIGNAL(finished()), SLOT(postTimeEntryFinished()));
}

void Redmine::retrieveIssuesFinished()
{
    qDebug() << "Redmine: retrieveIssuesFinished";
    if (retrieveIssuesReply->error() > 0) {
        emit networkError(retrieveIssuesReply->errorString());
        retrieveIssuesReply->deleteLater();
    }
    else {
        QJsonDocument json = QJsonDocument::fromJson(retrieveIssuesReply->readAll());
        int offset = json.object().value("offset").toInt(0);
        int total_count = json.object().value("total_count").toInt(0);

        qDebug() << "Redmine: offset - total_count: " << offset << " - " << total_count;
        emit issuesFetchProgress(total_count, offset);

        foreach (const QJsonValue &value, json.object().value("issues").toArray()) {
            retrieve_issues_buff << value;
        }

        retrieveIssuesReply->deleteLater();

        if (offset < (total_count-RedmineIssueRequestLimit)) {
            qDebug() << "Redmine: calling retrieveIssues with offset: " << offset+RedmineIssueRequestLimit;

            retrieveIssues(offset+RedmineIssueRequestLimit, retrieve_issues_state==All ? true : false);
        }
        else {
            qDebug() << "Redmine: watcher_enabled" << watcher_enabled;
            if (retrieve_issues_state == Assigned && watcher_enabled) {
                retrieve_issues_state = Watcher;
                qDebug() << "Redmine: calling WATCHER retrieveIssues";
                retrieveIssues();
            }
            else {
                emit issuesFetched(retrieve_issues_buff);
            }
        }
    }
}

void Redmine::retrieveTimeEntriesFinished()
{
    qDebug() << "Redmine: retrieveTimeEntriesFinished";
    if (retrieveTimeEntriesReply->error() > 0) {
        emit networkError(retrieveTimeEntriesReply->errorString());
        retrieveTimeEntriesReply->deleteLater();
    }
    else {
        QJsonDocument json = QJsonDocument::fromJson(retrieveTimeEntriesReply->readAll());
        int offset = json.object().value("offset").toInt(0);
        int total_count = json.object().value("total_count").toInt(0);

        qDebug() << "Redmine retrieveTimeEnriesFinished: offset - total_count: " << offset << " - " << total_count;
        emit timeEntriesFetchProgress(total_count, offset);

        foreach (const QJsonValue &value, json.object().value("time_entries").toArray()) {
            retrieve_time_entries_buff << value;
        }

        retrieveTimeEntriesReply->deleteLater();

        if (offset < (total_count-RedmineTimeEntriesRequestLimit)) {
            qDebug() << "Redmine: calling retrieveIssues with offset: " << offset+RedmineTimeEntriesRequestLimit;

            retrieveTimeEntries(offset+RedmineTimeEntriesRequestLimit);
        }
        else {
            emit timeEntriesFetched(retrieve_time_entries_buff);
        }
    }
}

void Redmine::postTimeEntryFinished()
{
    qDebug() << "Redmine: slotPostTimeEntryFinished";
    if (postTimeEntryReply->error() > 0) {
        emit networkError(postTimeEntryReply->errorString());
    }
    else {
        // TODO: add handling of response messages
        qDebug() << "Redmine: slotPostTimeEntryFinished response: " << postTimeEntryReply->readAll();
        emit postTimeEntrySuccess();
    }

    // TODO client won't know that slot was called unless we called postTimeEntrySuccess

    postTimeEntryReply->deleteLater();
}

void Redmine::fetchActivitiesFinished()
{
    qDebug() << "Redmine: fetchActivitiesFinished";
    if (fetchActivitiesReply->error() > 0) {
        emit networkError(fetchActivitiesReply->errorString());
    }
    else {
        QJsonDocument json = QJsonDocument::fromJson(fetchActivitiesReply->readAll());
        qDebug() << "Redmine: fetchActivitiesReply().count()" << json.object().value("time_entry_activities").toArray().count();

        foreach (const QJsonValue &value, json.object().value("time_entry_activities").toArray()) {
            Activities activity = {value.toObject().value("id").toInt(), value.toObject().value("name").toString(), value.toObject().value("is_default").toBool()};
            activities.append(activity);
            qDebug() << value.toObject().value("id").toInt() << value.toObject().value("name").toString() << value.toObject().value("is_default").toBool();
        }
    }

    fetchActivitiesReply->deleteLater();
}

void Redmine::retrieveTimeEntries(int offset)
{
    qDebug() << "Redmine: retrieveTimeEntries offset=" << offset;
    QString url_string = base_url + "/time_entries.json?user_id=me&spent_on=m&limit=" + QString::number(RedmineTimeEntriesRequestLimit);
    url_string = url_string + "&offset=" + QString::number(offset);

    RedmineRequest request(QUrl(url_string), api_key);

    if (ba_enabled) {
        request.addBasicAuthHeader(ba_login, ba_pass);
    }

    qDebug() << "Redmine: retrieveTimeEntries URL: " << request.url();

    retrieveTimeEntriesReply = network->get(request);
    QObject::connect(retrieveTimeEntriesReply, SIGNAL(finished()), this, SLOT(retrieveTimeEntriesFinished()));
}

QString Redmine::getMyIssuesURL(bool watcher)
{
    QString base = "/issues.json?status_id=open&limit=" + QString::number(RedmineIssueRequestLimit);
    if (!watcher) {
        base += "&assigned_to_id=me";
    }
    else {
        base += "&watcher_id=me";
    }

    return base;
}

QString Redmine::getAllIssuesURL()
{
    QString base = "/issues.json?status_id=open&limit=" + QString::number(RedmineIssueRequestLimit);

    return base;
}

void Redmine::retrieveIssues(int offset, bool all)
{
    qDebug() << "Redmine: retrieveIssues offset=" << offset << " all=" << all;
    QString url_string = base_url;

    if (!all) {
        url_string = url_string + getMyIssuesURL(retrieve_issues_state == Assigned ? false : true);
    }
    else {
        url_string = url_string + getAllIssuesURL();
    }

    url_string = url_string + "&offset=" + QString::number(offset);

    RedmineRequest request(QUrl(url_string), api_key);

    if (ba_enabled) {
        request.addBasicAuthHeader(ba_login, ba_pass);
    }

    qDebug() << "Redmine: retrieveIssues URL: " << request.url();

    retrieveIssuesReply = network->get(request);
    QObject::connect(retrieveIssuesReply, SIGNAL(finished()), this, SLOT(retrieveIssuesFinished()));
}
