#pragma once

#include "ui_App.h"

class App : public QWidget
{
    Q_OBJECT

public:
    explicit App(QWidget* parent = nullptr);

private slots:
    void toggleMaximized();

private:
    Ui::App ui;
};
