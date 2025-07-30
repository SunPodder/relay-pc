#ifndef NOTIFICATIONPOPUP_H
#define NOTIFICATIONPOPUP_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "NotificationData.h"

class NotificationPopup : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationPopup(const NotificationData& notification, QWidget *parent = nullptr);
    ~NotificationPopup();

    int getNotificationId() const { return m_notificationData.id; }
    void setPosition(int x, int y);
    void startShowAnimation();

signals:
    void closeRequested(int notificationId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onCloseClicked();
    void onAutoCloseTimeout();
    void onShowAnimationFinished();
    void onHideAnimationFinished();

private:
    void setupUI();
    void startHideAnimation();
    void updateTimeLabel();
    
    NotificationData m_notificationData;
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    
    QLabel* m_appNameLabel;
    QLabel* m_timeLabel;
    QLabel* m_titleLabel;
    QLabel* m_bodyLabel;
    QPushButton* m_closeButton;
    
    QTimer* m_autoCloseTimer;
    QPropertyAnimation* m_showAnimation;
    QPropertyAnimation* m_hideAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    
    bool m_isHovered;
    bool m_isClosing;
    
    static constexpr int POPUP_WIDTH = 350;
    static constexpr int POPUP_HEIGHT = 120;
    static constexpr int POPUP_MARGIN = 12;
    static constexpr int POPUP_SPACING = 8;
    static constexpr int AUTO_CLOSE_DURATION = 5000; // 5 seconds
    static constexpr int ANIMATION_DURATION = 300;
};

#endif // NOTIFICATIONPOPUP_H
