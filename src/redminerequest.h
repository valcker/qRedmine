#ifndef REDMINEREQUEST_H
#define REDMINEREQUEST_H

#include <QNetworkRequest>

class RedmineRequest : public QNetworkRequest
{
public:
    RedmineRequest(const QUrl & url = QUrl(), const QString &api_key = "");
    void addBasicAuthHeader(const QString &ba_login, const QString &ba_pass);
};

#endif // REDMINEREQUEST_H
