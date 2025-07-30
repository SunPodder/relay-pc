#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Relay PC");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("RelayPC");
    
    MainWindow window;
    // Don't show the main window - we only use the notification panel
    // window.show(); // Removed this line
    
    return app.exec();
}
