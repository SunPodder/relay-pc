#include "NotificationPanel.h"
#include "NotificationCard.h"
#include "NotificationManager.h"
#include "NotificationClient.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QPainter>
#include <QStyleOption>

NotificationPanel::NotificationPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollWidget(nullptr)
    , m_scrollLayout(nullptr)
    , m_emptyLabel(nullptr)
    , m_clearButton(nullptr)
    , m_notificationManager(nullptr)
{
    setupUI();
    positionPanel();
    
    // Set window properties for floating panel
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // Calculate height based on screen size with margins
    int panelHeight = calculatePanelHeight();
    resize(PANEL_WIDTH, panelHeight);
}

NotificationPanel::~NotificationPanel()
{
    // QWidget destructor will handle child widgets
}

void NotificationPanel::setupUI()
{
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 2, 10);
    m_mainLayout->setSpacing(0);
    
    // Create header layout with title and clear button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);
    
    // Create title
    QLabel* titleLabel = new QLabel("Notifications", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet(
        "QLabel#titleLabel {"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    color: white;"
        "    padding: 10px;"
        "    background-color: rgba(0, 0, 0, 0.8);"
        "    border-radius: 8px;"
        "}"
    );
    
    // Create clear button
    m_clearButton = new QPushButton("Clear All", this);
    m_clearButton->setObjectName("clearButton");
    m_clearButton->setStyleSheet(
        "QPushButton#clearButton {"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    color: white;"
        "    background-color: rgba(200, 60, 60, 0.8);"
        "    border: 1px solid rgba(255, 255, 255, 0.2);"
        "    border-radius: 6px;"
        "    padding: 6px 12px;"
        "    min-width: 60px;"
        "}"
        "QPushButton#clearButton:hover {"
        "    background-color: rgba(220, 80, 80, 0.9);"
        "}"
        "QPushButton#clearButton:pressed {"
        "    background-color: rgba(180, 40, 40, 0.9);"
        "}"
        "QPushButton#clearButton:disabled {"
        "    background-color: rgba(100, 100, 100, 0.5);"
        "    color: rgba(255, 255, 255, 0.4);"
        "}"
    );
    
    // Connect clear button to slot
    connect(m_clearButton, &QPushButton::clicked, this, &NotificationPanel::clearAllNotifications);
    
    // Add widgets to header layout
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(); // Push clear button to the right
    headerLayout->addWidget(m_clearButton);
    
    // Create a header widget to contain the layout
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setLayout(headerLayout);
    headerWidget->setStyleSheet(
        "QWidget {"
        "    background-color: transparent;"
        "    margin-bottom: 10px;"
        "}"
    );
    
    m_mainLayout->addWidget(headerWidget);
    
    setupScrollArea();
    
    // Set panel styling
    setStyleSheet(
        "NotificationPanel {"
        "    background-color: rgba(30, 30, 30, 0.9);"
        "    border-radius: 12px;"
        "    border: 1px solid rgba(255, 255, 255, 0.1);"
        "}"
    );
}

void NotificationPanel::setupScrollArea()
{
    // Create scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    // Create scroll widget and layout
    m_scrollWidget = new QWidget();
    m_scrollWidget->setStyleSheet(
        "QWidget {"
        "    background-color: transparent;"
        "}"
    );
    m_scrollLayout = new QVBoxLayout(m_scrollWidget);
    m_scrollLayout->setContentsMargins(0, 0, 6, 0);
    m_scrollLayout->setSpacing(8);
    m_scrollLayout->addStretch(); // Push notifications to top
    
    // Create empty state label
    m_emptyLabel = new QLabel("No notifications", m_scrollWidget);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(
        "QLabel {"
        "    color: rgba(255, 255, 255, 0.6);"
        "    font-size: 14px;"
        "    padding: 40px;"
        "}"
    );
    m_scrollLayout->addWidget(m_emptyLabel);
    
    m_scrollArea->setWidget(m_scrollWidget);
    m_mainLayout->addWidget(m_scrollArea);
    
    // Style scroll area
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
        "QScrollBar:vertical {"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "    width: 4px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: rgba(255, 255, 255, 0.3);"
        "    border-radius: 4px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: rgba(255, 255, 255, 0.5);"
        "}"
    );
}

void NotificationPanel::positionPanel()
{
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->availableGeometry();
        
        // Position on the right side of the screen
        int x = screenGeometry.right() - PANEL_WIDTH - RIGHT_MARGIN;
        int y = screenGeometry.top() + TOP_MARGIN; // Use TOP_MARGIN
        
        move(x, y);
    }
}

void NotificationPanel::setNotificationManager(NotificationManager* manager)
{
    m_notificationManager = manager;
}

void NotificationPanel::addNotification(const NotificationData& notification)
{
    // Hide empty label if visible
    if (m_emptyLabel->isVisible()) {
        m_emptyLabel->hide();
    }
    
    // Check if we should group this notification with an existing one
    QString groupKey = notification.getGroupKey();
    NotificationCard* existingCard = nullptr;
    
    // Look for an existing notification with the same group key
    for (NotificationCard* card : m_notificationCards) {
        if (card->getNotificationData().getGroupKey() == groupKey) {
            existingCard = card;
            break;
        }
    }
    
    if (existingCard) {
        // Merge with existing notification
        NotificationData updatedData = existingCard->getNotificationData();
        updatedData.mergeWith(notification);
        
        // Update the existing card with merged data
        existingCard->updateNotificationData(updatedData);
        
        // Move the updated card to the top (newest first)
        int currentIndex = m_notificationCards.indexOf(existingCard);
        if (currentIndex > 0) {
            m_scrollLayout->removeWidget(existingCard);
            m_notificationCards.removeAt(currentIndex);
            
            m_scrollLayout->insertWidget(0, existingCard);
            m_notificationCards.prepend(existingCard);
        }
    } else {
        // Create new notification card
        NotificationCard* card = new NotificationCard(notification, m_scrollWidget);
        
        // Connect card signals
        connect(card, &NotificationCard::removeRequested,
                this, [this, card]() {
                    removeNotification(card->getNotificationId());
                });
        
        // Connect action signals to notification client
        if (m_notificationManager && m_notificationManager->getClient()) {
            auto client = m_notificationManager->getClient();
            
            connect(card, &NotificationCard::actionClicked,
                    this, [client, card](const QString& actionKey) {
                        client->sendNotificationAction(card->getNotificationData().stringId, actionKey);
                    });
            
            connect(card, &NotificationCard::replyRequested,
                    this, [client, card](const QString& actionKey, const QString& replyText) {
                        client->sendNotificationReply(card->getNotificationData().stringId, actionKey, replyText);
                    });
        }
        
        // Insert at the beginning (newest first)
        m_scrollLayout->insertWidget(0, card);
        m_notificationCards.prepend(card);
    }
    
    // Scroll to top to show new/updated notification
    m_scrollArea->verticalScrollBar()->setValue(0);
    
    updateEmptyState();
}

void NotificationPanel::removeNotification(int notificationId)
{
    for (int i = 0; i < m_notificationCards.size(); ++i) {
        NotificationCard* card = m_notificationCards[i];
        if (card->getNotificationId() == notificationId) {
            m_scrollLayout->removeWidget(card);
            m_notificationCards.removeAt(i);
            card->deleteLater();
            break;
        }
    }
    
    updateEmptyState();
}

void NotificationPanel::clearAllNotifications()
{
    for (NotificationCard* card : m_notificationCards) {
        m_scrollLayout->removeWidget(card);
        card->deleteLater();
    }
    m_notificationCards.clear();
    
    updateEmptyState();
}

void NotificationPanel::updateEmptyState()
{
    bool isEmpty = m_notificationCards.isEmpty();
    m_emptyLabel->setVisible(isEmpty);
    
    // Enable/disable clear button based on whether there are notifications
    if (m_clearButton) {
        m_clearButton->setEnabled(!isEmpty);
    }
}

void NotificationPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    // Paint semi-transparent background
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw rounded background
    painter.setBrush(QBrush(QColor(30, 30, 30, 230))); // Semi-transparent dark background
    painter.setPen(QPen(QColor(255, 255, 255, 25), 1)); // Subtle border
    painter.drawRoundedRect(rect(), 12, 12);
}

int NotificationPanel::calculatePanelHeight() const
{
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->availableGeometry();
        
        // Full height minus top and bottom margins
        int availableHeight = screenGeometry.height() - TOP_MARGIN - BOTTOM_MARGIN;
        return availableHeight;
    }
    
    // Fallback to a reasonable default
    return 600;
}
