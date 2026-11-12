/**
 *  baseUrl必须以 / 结尾
 *  relativePath不能以 / 开头
 **/

#include "apiclient.h"

#include <utility>
#include <qjsondocument.h>

ApiClient::ApiClient(QUrl baseUrl, QObject* parent)
    : QObject(parent)
    , m_baseUrl(std::move(baseUrl))
{
    if (!m_baseUrl.isValid() || m_baseUrl.host().isEmpty())
    {
        m_configurationError = QStringLiteral("后端基础地址无效");
        return;
    }

    const auto scheme = m_baseUrl.scheme();
    if (scheme != "http" && scheme != "https")
    {
        m_configurationError = QStringLiteral("后端地址必须使用http或者https");
        return;
    }

    auto path = m_baseUrl.path();   // 例http://www.example.com/test/123返回/test/123
    if (!path.endsWith('/'))
    {
        path.append('/');
        m_baseUrl.setPath(path);
    }

    m_networkManager.setTransferTimeout(10);
    m_networkManager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);  // 默认重定向，允许https跳转到https，但不会自动接受https降级到http
}

const QUrl& ApiClient::baseUrl() const noexcept
{
    return m_baseUrl;
}

QNetworkReply* ApiClient::getJson(const QString& relativePath)
{
    const auto url = resolveUrl(relativePath);
    if (url.isEmpty()) return nullptr;
    return m_networkManager.get(createJsonRequest(url));
}

QNetworkReply* ApiClient::postJson(const QString& relativePath, const QJsonObject& body)
{
    const auto url = resolveUrl(relativePath);
    if (url.isEmpty()) return nullptr;

    const auto payload = QJsonDocument(body).toJson(QJsonDocument::Compact);    // 紧致输出（去除换行）
    return m_networkManager.post(createJsonRequest(url), payload);
}

bool ApiClient::isConfigured() const
{
    return m_configurationError.isEmpty();
}

const QString& ApiClient::configurationError() const noexcept
{
    return m_configurationError;
}

QUrl ApiClient::resolveUrl(const QString& url) const
{
    if (!isConfigured() || url.isEmpty() || url.startsWith('/')) return{};

    const QUrl relativeUrl(url);
    if (!relativeUrl.isValid() || !relativeUrl.isRelative()) return {};

    auto resolvedUrl = m_baseUrl.resolved(relativeUrl);
    // 防止调用方通过 ../ 跳出统一API根路径。
    if (resolvedUrl.scheme() != m_baseUrl.scheme() || resolvedUrl.host() != m_baseUrl.host() ||
        resolvedUrl.port() != m_baseUrl.port() || !resolvedUrl.path().startsWith(m_baseUrl.path())) return {};

    return resolvedUrl;
}

QNetworkRequest ApiClient::createJsonRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json"); // 希望返回值也为json格式
    request.setTransferTimeout(10);
    return request;
}
