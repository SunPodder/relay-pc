#include "NotificationPanel.h"
#include "NotificationCard.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
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
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(0);
    
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
        "    margin-bottom: 10px;"
        "}"
    );
    m_mainLayout->addWidget(titleLabel);
    
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

void NotificationPanel::addNotification(const NotificationData& notification)
{
    // Hide empty label if visible
    if (m_emptyLabel->isVisible()) {
        m_emptyLabel->hide();
    }
    
    // Create new notification card
    NotificationCard* card = new NotificationCard(notification, m_scrollWidget);
    
    // Connect card signals
    connect(card, &NotificationCard::removeRequested,
            this, [this, card]() {
                removeNotification(card->getNotificationId());
            });
    
    // Insert at the beginning (newest first)
    m_scrollLayout->insertWidget(0, card);
    m_notificationCards.prepend(card);
    
    // Scroll to top to show new notification
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
