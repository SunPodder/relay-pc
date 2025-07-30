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
    QString body;
    QString iconPath;
    QString packageName;
    int id;
    QDateTime timestamp;
    bool canReply;
    QList<NotificationAction> actions;
    
    NotificationData() : id(0), timestamp(QDateTime::currentDateTime()), canReply(false) {}
    
    NotificationData(const QString& app, const QString& title, const QString& body)
        : appName(app), title(title), body(body), id(0), 
          timestamp(QDateTime::currentDateTime()), canReply(false) {}
    
    // Convert to/from JSON for serialization
    QJsonObject toJson() const;
    static NotificationData fromJson(const QJsonObject& json);
};

#endif // NOTIFICATIONDATA_H
