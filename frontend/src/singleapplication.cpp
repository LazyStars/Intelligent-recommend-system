#include "singleapplication.h"

#include <qthread.h>
#include <qlocalsocket.h>
#include <qstandardpaths.h>
#include <qdir.h>
#include <qwidget.h>

namespace
{
    const QString kSingleInstanceId =QStringLiteral("IntelligentRecommendSystem-5c73e3f0-4a6f-4db7-94a8-3d5982c0b6af");
}

SingleApplication::SingleApplication(int& argc, char** argv) : QApplication(argc, argv)
{
    initLocalConnection(kSingleInstanceId);
}

SingleApplication::~SingleApplication()
{
    if (m_server) m_server->close();
    m_instanceLock.reset();
}

bool SingleApplication::isSecondaryInstance() const
{
    return m_isSecondaryInstance;
}

void SingleApplication::setActivationWindow(QWidget* activeWindow)
{
    m_mainWindow = activeWindow;
}

bool SingleApplication::hasStartupError() const
{
    return !m_startupError.isEmpty();
}

const QString& SingleApplication::startupErrorString() const
{
    return m_startupError;
}

void SingleApplication::initLocalConnection(const QString& serverId)
{
    const auto lockPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(serverId + ".lock");
    m_instanceLock = std::make_unique<QLockFile>(lockPath);

    // 覆盖应用的长期锁
    m_instanceLock->setStaleLockTime(std::chrono::milliseconds::zero());
    if (!m_instanceLock->tryLock(500))
    {
        if (m_instanceLock->error() != QLockFile::LockFailedError)
        {
            m_startupError = QStringLiteral("无法创建单实例锁:%1").arg(lockPath);
            return;
        }
        // LockFailedError表示另一个实例持有锁
        m_isSecondaryInstance  = true;
        if (!notifyPrimaryInstance(serverId))
        {
            m_startupError = QStringLiteral("程序已经运行，但无法激活主窗口");
        }

        return;
    }

    // 只有获得进程锁的实例才有资格创建本地服务器
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, &SingleApplication::handleNewConnection);

    if (m_server->listen(serverId))
    {
        return;
    }

    if (m_server->serverError() != QAbstractSocket::AddressInUseError)
    {
        m_startupError = QStringLiteral("无法启动本地通信服务：%1").arg(m_server->errorString());
        return;
    }

    // 当前实例已经取得QLockFile，因此这里可能是异常退出后遗留的本地通信端点
    if (!QLocalServer::removeServer(serverId))
    {
        m_startupError = QStringLiteral("无法清理遗留的本地通信端点：%1").arg(m_server->errorString());
        return;
    }

    if (!m_server->listen(serverId))
    {
        m_startupError = QStringLiteral("清理遗留端点后仍无法启动本地通信服务：%1").arg(m_server->errorString());
        return;
    }
}

void SingleApplication::activateExistingWindow()
{
    if (!m_mainWindow) return;
    auto state = m_mainWindow->windowState();
    if (state.testFlag(Qt::WindowMinimized))
    {
        state &= ~Qt::WindowMinimized;
        m_mainWindow->setWindowState(state);
    }
    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}

bool SingleApplication::notifyPrimaryInstance(const QString& serverId)
{
    // 可能存在主实例刚创建，server未开启，子实例启动时连接socket失败的情况
    // 尝试重连5次
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        QLocalSocket socket;
        socket.connectToServer(serverId);

        if (socket.waitForConnected(100))
        {
            socket.disconnectFromServer();
            return true;
        }

        if (attempt + 1 < 5)
        {
            QThread::msleep(100);
        }
    }
    return false;
}

void SingleApplication::handleNewConnection()
{
    while (m_server->hasPendingConnections())
    {
        auto *client = m_server->nextPendingConnection();
        if (!client) continue;

        client->close();
        client->deleteLater();
        activateExistingWindow();
    }
}
