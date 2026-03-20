#include <QApplication>
#include <QCommandLineParser>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("7-Zip ZS");
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("7-Zip ZS");
    app.setDesktopFileName("7zz-gui");

    QCommandLineParser parser;
    parser.setApplicationDescription("7-Zip ZS - Archive Manager with Zstandard support");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("archive", "Archive file to open", "[archive]");
    parser.process(app);

    MainWindow window;
    window.show();

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        window.openArchive(args.first());
    }

    return app.exec();
}
