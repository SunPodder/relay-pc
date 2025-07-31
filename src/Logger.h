#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>

class Logger {
public:
    static bool verboseMode;
    
    // Always show info messages (connection status, etc.)
    static void info(const QString& message) {
        qInfo() << "INFO:" << message;
    }
    
    // Only show debug messages if verbose mode is enabled
    static void debug(const QString& message) {
        if (verboseMode) {
            qDebug() << "DEBUG:" << message;
        }
    }
    
    // Always show warnings and errors
    static void warning(const QString& message) {
        qWarning() << "WARNING:" << message;
    }
    
    static void error(const QString& message) {
        qCritical() << "ERROR:" << message;
    }
    
    static void setVerbose(bool enabled) {
        verboseMode = enabled;
    }
};

#endif // LOGGER_H
