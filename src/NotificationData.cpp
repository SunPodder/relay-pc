#include "NotificationData.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

QJsonObject NotificationAction::toJson() const
{
    QJsonObject json;
    json["title"] = title;
    json["type"] = type;
    json["key"] = key;
    return json;
}

NotificationAction NotificationAction::fromJson(const QJsonObject& json)
{
    NotificationAction action;
    action.title = json["title"].toString();
    action.type = json["type"].toString();
    action.key = json["key"].toString();
    return action;
}

QJsonObject NotificationData::toJson() const 
{
    QJsonObject json;
    json["appName"] = appName;
    json["title"] = title;
    json["body"] = body;
    json["iconPath"] = iconPath;
    json["packageName"] = packageName;
    json["timestamp"] = timestamp.toString(Qt::ISODate);
    json["id"] = id;
    json["canReply"] = canReply;
    
    QJsonArray actionsArray;
    for (const NotificationAction& action : actions) {
        actionsArray.append(action.toJson());
    }
    json["actions"] = actionsArray;
    
    return json;
}

NotificationData NotificationData::fromJson(const QJsonObject& json) 
{
    NotificationData notification;
    notification.appName = json["appName"].toString();
    notification.title = json["title"].toString();
    notification.body = json["body"].toString();
    notification.iconPath = json["iconPath"].toString();
    notification.packageName = json["packageName"].toString();
    notification.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    notification.id = json["id"].toInt();
    notification.canReply = json["canReply"].toBool();
    
    QJsonArray actionsArray = json["actions"].toArray();
    for (const QJsonValue& value : actionsArray) {
        notification.actions.append(NotificationAction::fromJson(value.toObject()));
    }
    
    return notification;
}
