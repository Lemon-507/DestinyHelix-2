#include "views/main_window.h"

#include "character_factory.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(app);
    CharacterFactory::initialize();

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
