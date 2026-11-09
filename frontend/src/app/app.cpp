#include "app.h"

#include <QWKWidgets/widgetwindowagent.h>

App::App(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);
    this->setWindowFlag(Qt::FramelessWindowHint);

    auto agent = new QWK::WidgetWindowAgent(this);
    agent->setup(this);
    agent->setTitleBar(ui.widget_title);

    agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, ui.label_logo);
    agent->setSystemButton(QWK::WindowAgentBase::Minimize, ui.btn_minimize);
    agent->setSystemButton(QWK::WindowAgentBase::Maximize, ui.btn_reduce);
    agent->setSystemButton(QWK::WindowAgentBase::Close, ui.btn_close);

    connect(ui.btn_close, &QPushButton::clicked, this, &App::close);
    connect(ui.btn_minimize, &QPushButton::clicked, this, &App::showMinimized);
    connect(ui.btn_reduce, &QPushButton::clicked, this, &App::toggleMaximized);
}

void App::toggleMaximized()
{
    if (this->isMaximized())
    {
        showNormal();
    } else
    {
        showMaximized();
    }
}
