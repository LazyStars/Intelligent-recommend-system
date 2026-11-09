#include "singleapplication.h"
#include "app/app.h"

#include <qmessagebox.h>

int main(int argc, char* argv[])
{
    SingleApplication app(argc, argv);

    if (app.hasStartupError())
    {
        QMessageBox::critical(nullptr, QObject::tr("start error"), app.startupErrorString());
        return 1;
    }

    if (app.isSecondaryInstance())
    {
        return 0;
    }

    App a;
    app.setActivationWindow(&a);
    a.show();

    return app.exec();
}
