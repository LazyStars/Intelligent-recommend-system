/**
 * 统一错误模型
 */

#pragma once

#include <qmetatype.h>
#include <qstring.h>

enum class ApiErrorKind
{
    Network,            // 网络错误
    Timeout,            // 超时
    Tls,                // 加密错误
    Http,               // 服务器错误
    ResponseTooLarge,   // 响应过大（后端异常或恶意响应）
    InvalidJson,        // 无效Json
    InvalidPayload,     // 无效载荷（传输用户数据错误）
    Cancelled,          // 取消
    Unknown,            // 未知
};

struct ApiError
{
    // 错误发生具体层级
    ApiErrorKind kind = ApiErrorKind::Unknown;

    // 稳定的机器可读错误码，例如 AUTH_SESSION_EXPIRED
    QString code;

    // 仅用于诊断，不应未经处理直接显示给用户，保存技术调试信息
    QString diagnosticMessage;

    // 没有收到 HTTP 响应时保持为 0
    /**
     * 0 没有收到HTTP响应
     * 400 请求无效
     * 401 需要登录
     * 429 请求过于频繁
     * 500 后端错误
     */
    int httpStatus = 0;

    // 表示当前操作是否适合自动或手动重试
    bool retryable = false;
};
Q_DECLARE_METATYPE(ApiError)