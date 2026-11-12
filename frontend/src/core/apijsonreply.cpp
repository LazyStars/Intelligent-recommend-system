#include "apijsonreply.h"

#include <iso646.h>
#include <qjsondocument.h>
#include <qjsonobject.h>

namespace
{
    constexpr qsizetype kMaximumResponseSize = 4 * 1024 * 1024; // 单次读取最大4MB

    bool isRetryableHttpStatus(int status)
    {
        return status == 408 || // 超时
           status == 429 ||     // 请求过多，触发限流
           status == 500 ||     // 服务器内部错误，无法完成请求
           status == 502 ||     // 作为网关或者代理工作的服务器尝试执行请求时，从远程服务器接收到了一个无效的响应
           status == 503 ||     // 超载或系统维护，服务器暂时的无法处理客户端的请求
           status == 504;       // 充当网关或代理的服务器，未及时从远端服务器获取请求
    }

    bool isRetryableNetworkError(QNetworkReply::NetworkError error)
    {
        switch (error)
        {
            case QNetworkReply::ConnectionRefusedError:
            case QNetworkReply::RemoteHostClosedError:
            case QNetworkReply::HostNotFoundError:
            case QNetworkReply::TimeoutError:
            case QNetworkReply::TemporaryNetworkFailureError:
            case QNetworkReply::NetworkSessionFailedError:
                return true;
            default:
                return false;
        }
    }
}

ApiJsonReply::ApiJsonReply(QNetworkReply* networkReply, QObject* parent)
    : QObject(parent)
    , m_networkReply(networkReply)
{
    connect(networkReply, &QNetworkReply::readyRead, this, &ApiJsonReply::consumeAvailableData);
    connect(networkReply, &QNetworkReply::finished, this, &ApiJsonReply::handleFinished);
}

ApiJsonReply::~ApiJsonReply()
{
    if (m_networkReply && !m_networkReply->isFinished())
    {
        disconnect(m_networkReply, nullptr, this, nullptr);
        m_networkReply->abort();
        m_networkReply->deleteLater();
    }
}

void ApiJsonReply::cancel()
{
    if (!m_completed || m_responseTooLarge) return;
    m_cancelRequested = true;
    m_networkReply->abort();
}

void ApiJsonReply::consumeAvailableData()
{
    if (!m_networkReply || m_responseTooLarge) return;

    const auto chunk = m_networkReply->readAll();
    const auto remainingCapacity = kMaximumResponseSize - m_responseBody.size();
    if (chunk.size() > remainingCapacity)
    {
        m_responseTooLarge = true;
        m_networkReply->abort();
        return;
    }
    m_responseBody.append(chunk);
}

void ApiJsonReply::handleFinished()
{
    if (m_completed || !m_networkReply) return;
    consumeAvailableData();
    auto *reply = m_networkReply.data();
    const auto networkError = reply->error();
    const auto networkErrorString  = reply->errorString();

    const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    m_networkReply = nullptr;

    if (m_responseTooLarge)
    {
        ApiError err;
        err.kind = ApiErrorKind::ResponseTooLarge;
        err.code = QStringLiteral("RESPONSE_TOO_LARGE");
        err.diagnosticMessage = QStringLiteral("json响应超过允许的最大大小（4MiB）");
        err.httpStatus = httpStatus;
        err.retryable = false;
        completeWithError(err);
        return;
    }

    if (m_cancelRequested)
    {
        ApiError error;
        error.kind = ApiErrorKind::Cancelled;
        error.code = QStringLiteral("REQUEST_CANCELLED");
        error.diagnosticMessage = QStringLiteral("请求已被取消");
        error.httpStatus = httpStatus;
        error.retryable = false;
        completeWithError(error);
        return;
    }

    if (httpStatus >= 400)
    {
        ApiError error;
        error.kind = ApiErrorKind::Http;
        error.httpStatus = httpStatus;
        error.retryable = isRetryableHttpStatus(httpStatus);

        const auto errorDocument = QJsonDocument::fromJson(m_responseBody);
        if (errorDocument.isObject())
        {
            const auto obj = errorDocument.object();
            error.code = obj.value("code").toString();
            error.diagnosticMessage = obj.value("message").toString();
        }
        if (error.code.isEmpty())
        {
            error.code = QStringLiteral("HTTP_ERROR");
        }
        if (error.diagnosticMessage.isEmpty())
        {
            error.diagnosticMessage = QStringLiteral("HTTP %1 %2").arg(httpStatus).arg(networkErrorString);
        }
        completeWithError(error);
        return;
    }

    if (networkError != QNetworkReply::NoError)
    {
        ApiError error;
        error.code = QStringLiteral("NETWORK_ERROR");
        error.diagnosticMessage = networkErrorString;
        error.httpStatus = httpStatus;
        error.retryable = isRetryableNetworkError(networkError);

        if (networkError == QNetworkReply::TimeoutError)
        {
            error.kind = ApiErrorKind::Timeout;
            error.code = QStringLiteral("REQUEST_TIMEOUT");
        }
        else if (networkError == QNetworkReply::SslHandshakeFailedError)    // SSL/TLS握手失败
        {
            error.kind = ApiErrorKind::Tls;
            error.code = QStringLiteral("TLS_ERROR");
        }
        else
        {
            error.kind = ApiErrorKind::Network;
        }
        completeWithError(error);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(m_responseBody, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        ApiError error;
        error.kind = ApiErrorKind::InvalidJson;
        error.code = QStringLiteral("INVALID_JSON");
        error.diagnosticMessage = QStringLiteral("JSON解析失败，位置 %1：%2").arg(parseError.offset).arg(parseError.errorString());
        error.httpStatus = httpStatus;
        error.retryable = false;
        completeWithError(error);
        return;
    }
    if (!document.isObject())
    {
        ApiError error;
        error.kind = ApiErrorKind::InvalidPayload;
        error.code = QStringLiteral("INVALID_PAYLOAD");
        error.diagnosticMessage = QStringLiteral("后端响应不是JSON对象");
        error.httpStatus = httpStatus;
        error.retryable = false;
        completeWithError(error);
        return;
    }
    m_completed = true;
    emit succeeded(document.object());
    deleteLater();
}

void ApiJsonReply::completeWithError(const ApiError& error)
{
    if (m_completed) return;
    m_completed = true;
    emit failed(error);
    deleteLater();
}
