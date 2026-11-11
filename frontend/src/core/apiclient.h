#pragma once

#include <qobject.h>
#include <qnetworkrequest.h>

class ApiClient final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ApiClient)

public:
    explicit ApiClient(QUrl baseUrl, QObject *parent = nullptr);

    const QUrl &baseUrl() const noexcept;

private:
    QNetworkRequest createJsonRequest();

    QUrl m_baseUrl;
};
