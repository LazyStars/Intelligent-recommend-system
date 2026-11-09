#pragma once

#include <qapplication.h>
#include <qlocalserver.h>
#include <qlockfile.h>
#include <qpointer.h>

#include <memory>

class SingleApplication : public QApplication
{
    Q_OBJECT

public:
    explicit SingleApplication(int &argc, char **argv);
    ~SingleApplication() override;

    bool isSecondaryInstance() const;
    void setActivationWindow(QWidget *activeWindow);

    bool hasStartupError() const;
    const QString &startupErrorString() const;

private:
    void initLocalConnection(const QString &serverId);
    void activateExistingWindow();
    bool notifyPrimaryInstance(const QString &serverId);

private slots:
    void handleNewConnection();

private:
    QPointer<QWidget> m_mainWindow = nullptr;
    QLocalServer *m_server = nullptr;
    std::unique_ptr<QLockFile> m_instanceLock;

    bool m_isSecondaryInstance = false;
    QString m_startupError;
};
