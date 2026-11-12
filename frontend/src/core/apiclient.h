#pragma once

#include <qobject.h>
#include <qnetworkrequest.h>
#include <qnetworkreply.h>

class ApiJsonReply;
class ApiClient final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ApiClient)

public:
    explicit ApiClient(QUrl baseUrl, QObject *parent = nullptr);
    const QUrl &baseUrl() const noexcept;
    ApiJsonReply *getJson(const QString &relativePath);
    ApiJsonReply *postJson(const QString &relativePath, const QJsonObject &body);

    bool isConfigured() const;
    const QString &configurationError() const noexcept;

private:
    QUrl resolveUrl(const QString& url) const;
    QNetworkRequest createJsonRequest(const QUrl &url) const;

    QNetworkAccessManager m_networkManager;
    QUrl m_baseUrl;
    QString m_configurationError;
};
