#pragma once

#include <qdialog.h>

#include "ui_generalpopup.h"

class GeneralPopup : public QDialog
{
    Q_OBJECT

public:
    explicit GeneralPopup(QWidget* parent = nullptr);

private:
    Ui::GeneralPopup ui;
};
