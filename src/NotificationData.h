#ifndef NOTIFICATIONDATA_H
#define NOTIFICATIONDATA_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QList>

struct NotificationAction {
    QString title;
    QString type;  // "remote_input" or "action"
    QString key;
    
    NotificationAction() = default;
    NotificationAction(const QString& title, const QString& type, const QString& key)
        : title(title), type(type), key(key) {}
    
    QJsonObject toJson() const;
    static NotificationAction fromJson(const QJsonObject& json);
};

struct NotificationData {
    QString appName;
    QString title;
    QString body;  // Primary body text (for backward compatibility)
    QStringList bodies;  // Array of all bodies for grouped notifications
    QString iconPath;
    QString packageName;
    int id;
    QString stringId;  // Original string ID from protocol
    QDateTime timestamp;
    bool canReply;
    QList<NotificationAction> actions;
    int groupCount;  // Number of notifications in this group
    
    NotificationData() : id(0), timestamp(QDateTime::currentDateTime()), canReply(false), groupCount(1) {}
    
    NotificationData(const QString& app, const QString& title, const QString& body)
        : appName(app), title(title), body(body), id(0), 
          timestamp(QDateTime::currentDateTime()), canReply(false), groupCount(1) {
        bodies.append(body);
    }
    
    // Helper methods for grouping
    QString getGroupKey() const;
    void mergeWith(const NotificationData& other);
    QString getDisplayBody() const;
    QString getAllBodiesFormatted() const;
    bool isGrouped() const { return groupCount > 1; }
    
    // Convert to/from JSON for serialization
    QJsonObject toJson() const;
    static NotificationData fromJson(const QJsonObject& json);
};

#endif // NOTIFICATIONDATA_H
