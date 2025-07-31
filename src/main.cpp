#include <QApplication>
#include <QDebug>
#include "MainWindow.h"
#include "NotificationManager.h"
#include "NotificationClient.h"
#include "Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Relay PC");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("RelayPC");
    
    MainWindow window;
    
    // Parse command line arguments
    bool directMode = false;
    QString serverHost;
    quint16 serverPort = 8080;
    
    for (int i = 1; i < argc; i++) {
        QString arg = argv[i];
        
        if (arg == "--verbose" || arg == "-v") {
            Logger::setVerbose(true);
            Logger::info("Verbose logging enabled");
        }
        else if (arg == "--direct" && i + 1 < argc) {
            directMode = true;
            serverHost = argv[++i];
            if (i + 1 < argc && !QString(argv[i + 1]).startsWith("--")) {
                serverPort = QString(argv[++i]).toUShort();
                if (serverPort == 0) serverPort = 8080;
            }
        }
        else if (arg == "--help" || arg == "-h") {
            qInfo() << "Relay PC - Android Notification Relay";
            qInfo() << "Usage:" << argv[0] << "[options]";
            qInfo() << "";
            qInfo() << "Options:";
            qInfo() << "  --direct <host> [port]  Connect directly to server (default port: 8080)";
            qInfo() << "  --verbose, -v           Enable verbose debug logging";
            qInfo() << "  --help, -h              Show this help message";
            qInfo() << "";
            qInfo() << "Default behavior: Use mDNS to discover Android server automatically";
            return 0;
        }
    }
    
    Logger::info("Starting Relay PC v1.0");
    
    if (directMode) {
        Logger::info(QString("Direct mode: connecting to %1:%2").arg(serverHost).arg(serverPort));
        window.getNotificationManager()->getClient()->connectToServerDirect(serverHost, serverPort);
    } else {
        Logger::info("Searching for Android server via mDNS (_relay._tcp.local)");
        Logger::info("Tip: Use --direct <host> [port] if mDNS discovery fails");
    }
    
    // Don't show the main window - we only use the notification panel
    // window.show(); // Removed this line
    
    return app.exec();
}
