#ifndef NOTIFICATIONCARD_H
#define NOTIFICATIONCARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
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
    void updateNotificationData(const NotificationData& newData);

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
    void onSendButtonClicked();
    void onCancelButtonClicked();
    void onInputReturnPressed();

private:
    void setupUI();
    void updateTimeLabel();
    void setupActionButtons();
    void setupInputField();
    void showActions();
    void hideActions();
    void showBodies();
    void hideBodies();
    void showInput(const QString& actionKey);
    void hideInput();
    void updateCardHeight();
    
    NotificationData m_notificationData;
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QVBoxLayout* m_contentLayout;
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
    
    // Input field for remote_input actions
    QWidget* m_inputWidget; // Container for input field and send/cancel buttons
    QVBoxLayout* m_inputLayout;
    QLineEdit* m_replyInput;
    QHBoxLayout* m_inputButtonsLayout;
    QPushButton* m_sendButton;
    QPushButton* m_cancelButton;
    QString m_currentActionKey; // Track which action key the input is for
    
    bool m_isHovered;
    bool m_actionsVisible;
    bool m_bodiesExpanded;  // Track if we're showing all bodies
    bool m_inputVisible;    // Track if input field is visible
    
    static constexpr int CARD_MARGIN = 12;
    static constexpr int CARD_SPACING = 8;
};

#endif // NOTIFICATIONCARD_H
