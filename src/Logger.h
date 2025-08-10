#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>

class Logger {
public:
    static bool verboseMode;
    
    // Always show info messages (connection status, etc.)
    static void info(const QString& message) {
        // Cyan
        qInfo().noquote() << "\033[36m[INFO]\033[0m" << message;
    }

    // Only show debug messages if verbose mode is enabled
    static void debug(const QString& message) {
        if (verboseMode) {
            // Green
            qDebug().noquote() << "\033[32m[DEBUG]\033[0m" << message;
        }
    }

    // Always show warnings and errors
    static void warning(const QString& message) {
        // Yellow
        qWarning().noquote() << "\033[33m[WARNING]\033[0m" << message;
    }

    static void error(const QString& message) {
        // Red
        qCritical().noquote() << "\033[31m[ERROR]\033[0m" << message;
    }
    
    static void setVerbose(bool enabled) {
        verboseMode = enabled;
    }
};

#endif // LOGGER_H
