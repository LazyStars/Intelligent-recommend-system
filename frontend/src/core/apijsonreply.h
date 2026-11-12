#pragma once

#include <qnetworkreply.h>
#include <qobject.h>
#include <qpointer.h>

#include "apierror.h"

class ApiJsonReply final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ApiJsonReply)

public:
    explicit ApiJsonReply(QNetworkReply *networkReply, QObject *parent = nullptr);
    ~ApiJsonReply() override;
    void cancel();

signals:
    void succeeded(const QJsonObject &object);
    void failed(const ApiError &error);

private slots:
    void consumeAvailableData();
    void handleFinished();

private:
    void completeWithError(const ApiError& error);

    QPointer<QNetworkReply> m_networkReply;
    QByteArray m_responseBody;

    bool m_completed = false;
    bool m_cancelRequested = false;
    bool m_responseTooLarge = false;
};
