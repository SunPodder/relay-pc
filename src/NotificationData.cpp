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
    json["groupCount"] = groupCount;
    
    // Serialize bodies array
    QJsonArray bodiesArray;
    for (const QString& bodyText : bodies) {
        bodiesArray.append(bodyText);
    }
    json["bodies"] = bodiesArray;
    
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
    notification.groupCount = json["groupCount"].toInt(1);  // Default to 1 if not present
    
    // Deserialize bodies array
    QJsonArray bodiesArray = json["bodies"].toArray();
    for (const QJsonValue& value : bodiesArray) {
        notification.bodies.append(value.toString());
    }
    // If bodies is empty but body exists, populate bodies for backward compatibility
    if (notification.bodies.isEmpty() && !notification.body.isEmpty()) {
        notification.bodies.append(notification.body);
    }
    
    QJsonArray actionsArray = json["actions"].toArray();
    for (const QJsonValue& value : actionsArray) {
        notification.actions.append(NotificationAction::fromJson(value.toObject()));
    }
    
    return notification;
}

QString NotificationData::getGroupKey() const
{
    // Group by app name and title combination
    return QString("%1|%2").arg(appName, title);
}

void NotificationData::mergeWith(const NotificationData& other)
{
    // Add the new notification's body to our bodies list
    if (!other.body.isEmpty() && !bodies.contains(other.body)) {
        bodies.append(other.body);
    }
    
    // Update to the latest timestamp
    if (other.timestamp > timestamp) {
        timestamp = other.timestamp;
    }
    
    // Update the primary body to be the most recent one
    if (!other.body.isEmpty()) {
        body = other.body;
    }
    
    // Increment group count
    groupCount++;
    
    // Merge actions (avoid duplicates)
    for (const NotificationAction& action : other.actions) {
        bool exists = false;
        for (const NotificationAction& existingAction : actions) {
            if (existingAction.key == action.key) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            actions.append(action);
        }
    }
    
    // Update reply capability if either can reply
    canReply = canReply || other.canReply;
}

QString NotificationData::getDisplayBody() const
{
    // Always show just the latest message (primary body)
    return body;
}

QString NotificationData::getAllBodiesFormatted() const
{
    if (bodies.size() <= 1) {
        return body;
    }
    
    // For grouped notifications, show all bodies with separators
    QString result;
    for (int i = 0; i < bodies.size(); ++i) {
        if (i > 0) {
            result += "\n• ";
        } else {
            result += "• ";
        }
        result += bodies[i];
    }
    
    return result;
}
