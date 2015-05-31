#include "redminerequest.h"

RedmineRequest::RedmineRequest(const QUrl &url, const QString &api_key) :
    QNetworkRequest(url)
{
    setRawHeader("X-Redmine-API-Key", api_key.toLocal8Bit());
}

void RedmineRequest::addBasicAuthHeader(const QString &ba_login, const QString &ba_pass)
{
    setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2").arg(ba_login).arg(ba_pass).toLocal8Bit()).toBase64());
}
