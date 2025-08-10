#include "NotificationCard.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QDateTime>
#include <QEvent>
#include <QEnterEvent>
#include <QMouseEvent>

NotificationCard::NotificationCard(const NotificationData& notification, QWidget *parent)
    : QWidget(parent)
    , m_notificationData(notification)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_actionLayout(nullptr)
    , m_appNameLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_bodyLabel(nullptr)
    , m_removeButton(nullptr)
    , m_actionIndicator(nullptr)
    , m_actionWidget(nullptr)
    , m_actionButtonsLayout(nullptr)
    , m_inputWidget(nullptr)
    , m_replyInput(nullptr)
    , m_sendButton(nullptr)
    , m_cancelButton(nullptr)
    , m_isHovered(false)
    , m_actionsVisible(false)
    , m_bodiesExpanded(false)
    , m_inputVisible(false)
{
    setupUI();
    
    // Set widget properties - take only the minimum height needed
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMouseTracking(true);
}

NotificationCard::~NotificationCard()
{
    // QWidget destructor handles child widgets
}

void NotificationCard::updateNotificationData(const NotificationData& newData)
{
    m_notificationData = newData;
    m_actionsVisible = false;
    m_bodiesExpanded = false;
    
    // Update UI elements with new data
    if (m_appNameLabel) {
        m_appNameLabel->setText(m_notificationData.appName);
    }
    if (m_titleLabel) {
        m_titleLabel->setText(m_notificationData.title);
    }
    if (m_bodyLabel) {
        m_bodyLabel->setText(m_notificationData.getDisplayBody());
    }
    updateTimeLabel();
    
    // Update action indicator visibility based on actions or grouping
    if (m_actionIndicator) {
        bool shouldShowIndicator = !m_notificationData.actions.isEmpty() || m_notificationData.isGrouped();
        m_actionIndicator->setVisible(shouldShowIndicator);
    }

    setupActionButtons();
}

void NotificationCard::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(CARD_MARGIN, CARD_MARGIN, CARD_MARGIN, CARD_MARGIN);
    m_mainLayout->setSpacing(CARD_SPACING);
    
    // Create header layout (app name, time, remove button)
    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setSpacing(8);
    
    // App name label
    m_appNameLabel = new QLabel(m_notificationData.appName, this);
    m_appNameLabel->setStyleSheet(
        "QLabel {"
        "    color: rgba(255, 255, 255, 0.8);"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "}"
    );
    m_headerLayout->addWidget(m_appNameLabel);
    
    // Action indicator (down arrow) - show if there are actions OR grouped messages
    if (!m_notificationData.actions.isEmpty() || m_notificationData.isGrouped()) {
        m_actionIndicator = new QLabel("⌄", this);
        m_actionIndicator->setStyleSheet(
            "QLabel {"
            "    color: rgba(255, 255, 255, 0.6);"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    padding: 2px;"
            "}"
            "QLabel:hover {"
            "    color: rgba(255, 255, 255, 0.8);"
            "}"
        );
        m_actionIndicator->setCursor(Qt::PointingHandCursor);
        m_actionIndicator->setMouseTracking(true);
        m_actionIndicator->installEventFilter(this);
        m_headerLayout->addWidget(m_actionIndicator);
    }
    
    m_headerLayout->addStretch(); // Push time and button to the right
    
    // Time label
    m_timeLabel = new QLabel(this);
    updateTimeLabel();
    m_timeLabel->setStyleSheet(
        "QLabel {"
        "    color: rgba(255, 255, 255, 0.5);"
        "    font-size: 10px;"
        "}"
    );
    m_headerLayout->addWidget(m_timeLabel);
    
    // Remove button
    m_removeButton = new QPushButton("×", this);
    m_removeButton->setFixedSize(20, 20);
    m_removeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: rgba(255, 255, 255, 0.6);"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "    color: rgba(255, 255, 255, 0.9);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(255, 255, 255, 0.2);"
        "}"
    );
    connect(m_removeButton, &QPushButton::clicked, this, &NotificationCard::onRemoveClicked);
    m_headerLayout->addWidget(m_removeButton);
    m_mainLayout->addLayout(m_headerLayout);

    m_contentLayout = new QVBoxLayout();
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(4);
    m_mainLayout->addLayout(m_contentLayout);

    // Title label
    if (!m_notificationData.title.isEmpty()) {
        m_titleLabel = new QLabel(m_notificationData.title, this);
        m_titleLabel->setWordWrap(true);
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_titleLabel->setStyleSheet(
            "QLabel {"
            "    color: white;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "}"
        );
        m_contentLayout->addWidget(m_titleLabel);
    }
    
    // Body label  
    if (!m_notificationData.body.isEmpty()) {
        m_bodyLabel = new QLabel(m_notificationData.getDisplayBody(), this);
        m_bodyLabel->setWordWrap(true);
        m_bodyLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_bodyLabel->setStyleSheet(
            "QLabel {"
            "    color: rgba(255, 255, 255, 0.8);"
            "    font-size: 12px;"
            "    line-height: 1.4;"
            "}"
        );
        m_contentLayout->addWidget(m_bodyLabel);
    }
    
    // Setup action buttons (initially hidden)
    setupActionButtons();
    
    // Setup input field (initially hidden)
    setupInputField();
    
    updateGeometry();
}

void NotificationCard::updateTimeLabel()
{
    if (!m_timeLabel) return;
    
    // Show actual time instead of relative time to avoid needing periodic updates
    QString timeText = m_notificationData.timestamp.toString("hh:mm");
    m_timeLabel->setText(timeText);
}

void NotificationCard::onRemoveClicked()
{
    emit removeRequested();
}

void NotificationCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw card background
    QColor backgroundColor = m_isHovered ? 
        QColor(60, 60, 60, 200) : QColor(45, 45, 45, 180);
    
    painter.setBrush(backgroundColor);
    painter.setPen(QPen(QColor(255, 255, 255, m_isHovered ? 40 : 20), 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
}

void NotificationCard::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void NotificationCard::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

void NotificationCard::setupActionButtons()
{
    if (m_notificationData.actions.isEmpty()) {
        return; // No actions to set up
    }
    
    // Create action widget container
    m_actionWidget = new QWidget(this);
    m_actionButtonsLayout = new QHBoxLayout(m_actionWidget);
    m_actionButtonsLayout->setContentsMargins(0, 5, 0, 0);
    m_actionButtonsLayout->setSpacing(8);
    
    // Create buttons for each action
    for (const NotificationAction& action : m_notificationData.actions) {
        QPushButton* button = new QPushButton(action.title, m_actionWidget);
        button->setProperty("actionKey", action.key);
        button->setProperty("actionType", action.type);
        
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: rgba(70, 130, 180, 0.8);"
            "    border: 1px solid rgba(255, 255, 255, 0.2);"
            "    border-radius: 4px;"
            "    color: white;"
            "    font-size: 11px;"
            "    padding: 4px 8px;"
            "    min-width: 60px;"
            "}"
            "QPushButton:hover {"
            "    background-color: rgba(70, 130, 180, 1.0);"
            "    border: 1px solid rgba(255, 255, 255, 0.4);"
            "}"
            "QPushButton:pressed {"
            "    background-color: rgba(50, 110, 160, 1.0);"
            "}"
        );
        
        connect(button, &QPushButton::clicked, this, &NotificationCard::onActionButtonClicked);
        m_actionButtonsLayout->addWidget(button);
        m_actionButtons.append(button);
    }
    
    m_actionButtonsLayout->addStretch();
    m_mainLayout->addWidget(m_actionWidget);
    
    // Initially hidden
    m_actionWidget->hide();
}

void NotificationCard::setupInputField()
{
    // Create input widget container
    m_inputWidget = new QWidget(this);
    m_inputLayout = new QVBoxLayout(m_inputWidget);
    m_inputLayout->setContentsMargins(0, 5, 0, 0);
    m_inputLayout->setSpacing(6);
    
    // Create reply input field
    m_replyInput = new QLineEdit(m_inputWidget);
    m_replyInput->setPlaceholderText("Type your reply...");
    m_replyInput->setStyleSheet(
        "QLineEdit {"
        "    background-color: rgba(60, 60, 60, 0.8);"
        "    border: 1px solid rgba(255, 255, 255, 0.2);"
        "    border-radius: 4px;"
        "    color: white;"
        "    font-size: 12px;"
        "    padding: 6px 8px;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid rgba(70, 130, 180, 0.8);"
        "    background-color: rgba(70, 70, 70, 0.9);"
        "}"
    );
    
    // Connect return key to send
    connect(m_replyInput, &QLineEdit::returnPressed, this, &NotificationCard::onInputReturnPressed);
    
    m_inputLayout->addWidget(m_replyInput);
    
    // Create buttons layout
    m_inputButtonsLayout = new QHBoxLayout();
    m_inputButtonsLayout->setSpacing(8);
    
    // Create send button
    m_sendButton = new QPushButton("Send", m_inputWidget);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(70, 130, 180, 0.8);"
        "    border: 1px solid rgba(255, 255, 255, 0.2);"
        "    border-radius: 4px;"
        "    color: white;"
        "    font-size: 11px;"
        "    padding: 4px 12px;"
        "    min-width: 50px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(70, 130, 180, 1.0);"
        "    border: 1px solid rgba(255, 255, 255, 0.4);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(50, 110, 160, 1.0);"
        "}"
    );
    
    // Create cancel button
    m_cancelButton = new QPushButton("Cancel", m_inputWidget);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(120, 120, 120, 0.6);"
        "    border: 1px solid rgba(255, 255, 255, 0.2);"
        "    border-radius: 4px;"
        "    color: white;"
        "    font-size: 11px;"
        "    padding: 4px 12px;"
        "    min-width: 50px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(140, 140, 140, 0.8);"
        "    border: 1px solid rgba(255, 255, 255, 0.4);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(100, 100, 100, 0.8);"
        "}"
    );
    
    // Connect button signals
    connect(m_sendButton, &QPushButton::clicked, this, &NotificationCard::onSendButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &NotificationCard::onCancelButtonClicked);
    
    // Add buttons to layout
    m_inputButtonsLayout->addWidget(m_sendButton);
    m_inputButtonsLayout->addWidget(m_cancelButton);
    m_inputButtonsLayout->addStretch();
    
    // Create buttons container widget
    QWidget* buttonsWidget = new QWidget(m_inputWidget);
    buttonsWidget->setLayout(m_inputButtonsLayout);
    m_inputLayout->addWidget(buttonsWidget);
    
    // Add input widget to main layout
    m_mainLayout->addWidget(m_inputWidget);
    
    // Initially hidden
    m_inputWidget->hide();
}

void NotificationCard::showActions()
{
    if (m_actionWidget && !m_notificationData.actions.isEmpty()) {
        m_actionWidget->show();
        m_actionsVisible = true;
        updateGeometry();
    }
}

void NotificationCard::hideActions()
{
    if (m_actionWidget) {
        m_actionWidget->hide();
        m_actionsVisible = false;
        updateGeometry();
    }
}

void NotificationCard::showBodies()
{
    if (m_bodyLabel && m_notificationData.isGrouped()) {
        // Show all bodies formatted
        m_bodyLabel->setText(m_notificationData.getAllBodiesFormatted());
        m_bodiesExpanded = true;
        updateGeometry();
    }
}

void NotificationCard::hideBodies()
{
    if (m_bodyLabel && m_notificationData.isGrouped()) {
        // Show only the latest body
        m_bodyLabel->setText(m_notificationData.getDisplayBody());
        m_bodiesExpanded = false;
        updateGeometry();
    }
}

void NotificationCard::showInput(const QString& actionKey)
{
    if (m_inputWidget) {
        m_currentActionKey = actionKey;
        m_replyInput->clear();
        m_replyInput->setFocus();
        m_inputWidget->show();
        m_inputVisible = true;
        
        // Hide action buttons when showing input
        hideActions();
        
        updateCardHeight();
    }
}

void NotificationCard::hideInput()
{
    if (m_inputWidget) {
        m_inputWidget->hide();
        m_inputVisible = false;
        m_currentActionKey.clear();
        
        updateCardHeight();
    }
}

void NotificationCard::onActionButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QString actionKey = button->property("actionKey").toString();
    QString actionType = button->property("actionType").toString();
    
    if (actionType == "remote_input") {
        // Show input field for reply
        showInput(actionKey);
    } else {
        // Regular action - emit signal immediately
        emit actionClicked(actionKey);
    }
}

bool NotificationCard::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_actionIndicator && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            onActionIndicatorClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void NotificationCard::onSendButtonClicked()
{
    QString replyText = m_replyInput->text().trimmed();
    if (!replyText.isEmpty() && !m_currentActionKey.isEmpty()) {
        emit replyRequested(m_currentActionKey, replyText);
        hideInput();
    }
}

void NotificationCard::onCancelButtonClicked()
{
    hideInput();
}

void NotificationCard::onInputReturnPressed()
{
    onSendButtonClicked();
}

void NotificationCard::onActionIndicatorClicked()
{
    if (m_actionsVisible || m_bodiesExpanded || m_inputVisible) {
        // Hide expanded content (actions, bodies, and/or input)
        hideActions();
        hideBodies();
        hideInput();
        if (m_actionIndicator) {
            m_actionIndicator->setText("⌄"); // Down arrow
        }
    } else {
        // Show expanded content
        showActions();  // This will show actions if they exist
        showBodies();   // This will show all bodies if grouped
        if (m_actionIndicator) {
            m_actionIndicator->setText("⌃"); // Up arrow
        }
    }
}

void NotificationCard::updateCardHeight()
{
    // Let Qt handle the height automatically through layout system
    updateGeometry();
    update();
}
