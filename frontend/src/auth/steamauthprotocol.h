#pragma once

#include <qstring.h>
#include <qurl.h>
#include <qdatetime.h>
#include <chrono>

enum class SteamAuthStatus
{
    Pending,        // 待定
    Authorized,     // 已授权
    Denied,         // 拒绝
    Expired,        // 已过期
    Failed,         // 失败
};

struct SteamAuthSession
{
    QString sessionId;
    QUrl authorizationUrl;
    QDateTime expiresAtUtc;
    std::chrono::milliseconds pollInterval{1500};
};

struct SteamAuthStatusResult
{
    SteamAuthStatus status = SteamAuthStatus::Pending;

    QString steamId;
    QString exchangeCode;   // 登录成功后的一次性兑换码，后续接入token时需补充

    QString errorCode;
    QString errorMessage;
};