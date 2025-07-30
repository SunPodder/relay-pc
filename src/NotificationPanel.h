#ifndef NOTIFICATIONPANEL_H
#define NOTIFICATIONPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPropertyAnimation>
#include "NotificationData.h"

class NotificationCard;

class NotificationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationPanel(QWidget *parent = nullptr);
    ~NotificationPanel();

    void setupUI();
    void positionPanel();
    
public slots:
    void addNotification(const NotificationData& notification);
    void removeNotification(int notificationId);
    void clearAllNotifications();

protected:
    void paintEvent(QPaintEvent *event) override;

public:
    static constexpr int PANEL_WIDTH = 350;
    static constexpr int TOP_MARGIN = 50;
    static constexpr int BOTTOM_MARGIN = 50;
    static constexpr int RIGHT_MARGIN = 20;

private:
    void setupScrollArea();
    void updateEmptyState();
    int calculatePanelHeight() const;
    
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollWidget;
    QVBoxLayout* m_scrollLayout;
    QLabel* m_emptyLabel;
    
    QList<NotificationCard*> m_notificationCards;
};

#endif // NOTIFICATIONPANEL_H
