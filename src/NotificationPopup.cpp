#include "NotificationPopup.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QDateTime>
#include <QEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QApplication>
#include <QScreen>

NotificationPopup::NotificationPopup(const NotificationData& notification, QWidget *parent)
    : QWidget(parent)
    , m_notificationData(notification)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_appNameLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_bodyLabel(nullptr)
    , m_closeButton(nullptr)
    , m_autoCloseTimer(nullptr)
    , m_showAnimation(nullptr)
    , m_hideAnimation(nullptr)
    , m_opacityEffect(nullptr)
    , m_isHovered(false)
    , m_isClosing(false)
{
    setupUI();
    
    // Set window properties
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(POPUP_WIDTH, POPUP_HEIGHT);
    setMouseTracking(true);
    
    // Setup opacity effect for animations
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(0.0);
    
    // Setup animations
    m_showAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_showAnimation->setDuration(ANIMATION_DURATION);
    m_showAnimation->setStartValue(0.0);
    m_showAnimation->setEndValue(1.0);
    connect(m_showAnimation, &QPropertyAnimation::finished, this, &NotificationPopup::onShowAnimationFinished);
    
    m_hideAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_hideAnimation->setDuration(ANIMATION_DURATION);
    m_hideAnimation->setStartValue(1.0);
    m_hideAnimation->setEndValue(0.0);
    connect(m_hideAnimation, &QPropertyAnimation::finished, this, &NotificationPopup::onHideAnimationFinished);
    
    // Setup auto-close timer
    m_autoCloseTimer = new QTimer(this);
    m_autoCloseTimer->setSingleShot(true);
    connect(m_autoCloseTimer, &QTimer::timeout, this, &NotificationPopup::onAutoCloseTimeout);
}

NotificationPopup::~NotificationPopup()
{
    // QWidget destructor handles child widgets
}

void NotificationPopup::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(POPUP_MARGIN, POPUP_MARGIN, POPUP_MARGIN, POPUP_MARGIN);
    m_mainLayout->setSpacing(POPUP_SPACING);
    
    // Create header layout (app name, time, close button)
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
    
    // Close button
    m_closeButton = new QPushButton("×", this);
    m_closeButton->setFixedSize(20, 20);
    m_closeButton->setStyleSheet(
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
    );
    connect(m_closeButton, &QPushButton::clicked, this, &NotificationPopup::onCloseClicked);
    m_headerLayout->addWidget(m_closeButton);
    
    m_mainLayout->addLayout(m_headerLayout);
    
    // Title label
    if (!m_notificationData.title.isEmpty()) {
        m_titleLabel = new QLabel(m_notificationData.title, this);
        m_titleLabel->setWordWrap(true);
        m_titleLabel->setStyleSheet(
            "QLabel {"
            "    color: white;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    margin-bottom: 4px;"
            "}"
        );
        m_mainLayout->addWidget(m_titleLabel);
    }
    
    // Body label
    if (!m_notificationData.body.isEmpty()) {
        m_bodyLabel = new QLabel(m_notificationData.body, this);
        m_bodyLabel->setWordWrap(true);
        m_bodyLabel->setStyleSheet(
            "QLabel {"
            "    color: rgba(255, 255, 255, 0.9);"
            "    font-size: 12px;"
            "    line-height: 1.4;"
            "}"
        );
        m_mainLayout->addWidget(m_bodyLabel);
    }
}

void NotificationPopup::updateTimeLabel()
{
    if (!m_timeLabel) return;
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsAgo = m_notificationData.timestamp.secsTo(now);
    
    QString timeText;
    if (secondsAgo < 60) {
        timeText = "now";
    } else if (secondsAgo < 3600) {
        timeText = QString("%1m").arg(secondsAgo / 60);
    } else if (secondsAgo < 86400) {
        timeText = QString("%1h").arg(secondsAgo / 3600);
    } else {
        timeText = QString("%1d").arg(secondsAgo / 86400);
    }
    
    m_timeLabel->setText(timeText);
}

void NotificationPopup::setPosition(int x, int y)
{
    move(x, y);
}

void NotificationPopup::startShowAnimation()
{
    show();
    m_showAnimation->start();
}

void NotificationPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw popup background
    QColor backgroundColor = m_isHovered ? 
        QColor(60, 60, 60, 200) : QColor(45, 45, 45, 180);
    
    painter.setBrush(backgroundColor);
    painter.setPen(QPen(QColor(255, 255, 255, m_isHovered ? 40 : 20), 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
}

void NotificationPopup::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = true;
    
    // Pause auto-close timer when hovered
    if (m_autoCloseTimer->isActive()) {
        m_autoCloseTimer->stop();
    }
    
    update();
    QWidget::enterEvent(event);
}

void NotificationPopup::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = false;
    
    // Resume auto-close timer when not hovered
    if (!m_isClosing) {
        m_autoCloseTimer->start(AUTO_CLOSE_DURATION);
    }
    
    update();
    QWidget::leaveEvent(event);
}

bool NotificationPopup::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    Q_UNUSED(event)
    return QWidget::eventFilter(obj, event);
}

void NotificationPopup::onCloseClicked()
{
    if (!m_isClosing) {
        startHideAnimation();
    }
}

void NotificationPopup::onAutoCloseTimeout()
{
    if (!m_isClosing && !m_isHovered) {
        startHideAnimation();
    }
}

void NotificationPopup::onShowAnimationFinished()
{
    // Start auto-close timer after show animation is complete
    if (!m_isHovered) {
        m_autoCloseTimer->start(AUTO_CLOSE_DURATION);
    }
}

void NotificationPopup::onHideAnimationFinished()
{
    emit closeRequested(m_notificationData.id);
    deleteLater();
}

void NotificationPopup::startHideAnimation()
{
    if (m_isClosing) return;
    
    m_isClosing = true;
    m_autoCloseTimer->stop();
    m_hideAnimation->start();
}
