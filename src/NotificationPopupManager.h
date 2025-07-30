#ifndef NOTIFICATIONPOPUPMANAGER_H
#define NOTIFICATIONPOPUPMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QScreen>
#include "NotificationData.h"
#include "NotificationPopup.h"

class NotificationPopupManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationPopupManager(QObject *parent = nullptr);
    ~NotificationPopupManager();

    void showNotificationPopup(const NotificationData& notification);

private slots:
    void onPopupCloseRequested(int notificationId);

private:
    void calculatePopupPosition(NotificationPopup* popup);
    void repositionExistingPopups();
    QScreen* getCurrentScreen();
    
    QList<NotificationPopup*> m_activePopups;
    
    static constexpr int POPUP_SPACING = 10;
    static constexpr int SCREEN_MARGIN = 20;
};

#endif // NOTIFICATIONPOPUPMANAGER_H
