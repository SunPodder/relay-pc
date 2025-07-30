#ifndef NOTIFICATIONCARD_H
#define NOTIFICATIONCARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEnterEvent>
#include "NotificationData.h"

class NotificationCard : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationCard(const NotificationData& notification, QWidget *parent = nullptr);
    ~NotificationCard();

    int getNotificationId() const { return m_notificationData.id; }
    const NotificationData& getNotificationData() const { return m_notificationData; }

signals:
    void removeRequested();
    void actionClicked(const QString& action);
    void replyRequested(const QString& key, const QString& message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onRemoveClicked();
    void onActionButtonClicked();
    void onActionIndicatorClicked();

private:
    void setupUI();
    void updateTimeLabel();
    void setupActionButtons();
    void showActions();
    void hideActions();
    void updateCardHeight();
    
    NotificationData m_notificationData;
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QHBoxLayout* m_actionLayout;
    
    QLabel* m_appNameLabel;
    QLabel* m_timeLabel;
    QLabel* m_titleLabel;
    QLabel* m_bodyLabel;
    QPushButton* m_removeButton;
    QLabel* m_actionIndicator; // Down arrow for actions
    QWidget* m_actionWidget; // Container for action buttons
    QHBoxLayout* m_actionButtonsLayout;
    QList<QPushButton*> m_actionButtons;
    
    bool m_isHovered;
    bool m_actionsVisible;
    
    static constexpr int CARD_MARGIN = 12;
    static constexpr int CARD_SPACING = 8;
    static constexpr int CARD_HEIGHT_COLLAPSED = 100;
    static constexpr int CARD_HEIGHT_EXPANDED = 150;
};

#endif // NOTIFICATIONCARD_H
