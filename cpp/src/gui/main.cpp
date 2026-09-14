#include "main_window.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Matrix Filling Game");
    QCoreApplication::setOrganizationName("MatrixFillingGame");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Matrix Filling Game desktop editor and computations");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addPositionalArgument("matrix", "Optional .mfg or plain text matrix to open.", "[matrix]");
    parser.process(app);
    mfg::gui::MainWindow window;
    const auto arguments = parser.positionalArguments();
    if (!arguments.isEmpty()) {
        QString error;
        window.openPath(arguments.front(), &error);
        if (!error.isEmpty()) QMessageBox::warning(&window, "Cannot open matrix", error);
    }
    window.show();
    return app.exec();
}
