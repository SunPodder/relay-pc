#include "MainWindow.h"
#include "NotificationPanel.h"
#include "NotificationManager.h"
#include "AnimationManager.h"
#include "NotificationPopupManager.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QShortcut>
#include <QScreen>
#include <QRect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_notificationPanel(nullptr)
    , m_notificationManager(nullptr)
    , m_animationManager(nullptr)
    , m_popupManager(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_toggleAction(nullptr)
    , m_quitAction(nullptr)
    , m_connectAction(nullptr)
    , m_statusAction(nullptr)
    , m_panelVisible(false)
{
    setupUI();
    setupTrayIcon();
    setupHotkeys();
    
    // Hide the main window - we only use the notification panel
    hide();
}

MainWindow::~MainWindow()
{
    delete m_notificationPanel;
    delete m_notificationManager;
    delete m_animationManager;
}

void MainWindow::setupUI()
{
    // Create notification manager first
    m_notificationManager = new NotificationManager(this);
    
    // Create notification panel
    m_notificationPanel = new NotificationPanel();
    
    // Create animation manager
    m_animationManager = new AnimationManager(m_notificationPanel, this);
    
    // Create popup manager
    m_popupManager = new NotificationPopupManager(this);
    
    // Connect signals
    connect(m_notificationManager, &NotificationManager::notificationReceived,
            m_notificationPanel, &NotificationPanel::addNotification);
    
    // Connect popup manager to show popups for new notifications
    connect(m_notificationManager, &NotificationManager::notificationReceived,
            m_popupManager, &NotificationPopupManager::showNotificationPopup);
    
    // Connect network status signals
    connect(m_notificationManager, &NotificationManager::serverConnected,
            this, &MainWindow::onServerConnected);
    connect(m_notificationManager, &NotificationManager::serverDisconnected,
            this, &MainWindow::onServerDisconnected);
    connect(m_notificationManager, &NotificationManager::connectionError,
            this, &MainWindow::onConnectionError);
    
    // Start network client
    m_notificationManager->startNetworkClient();
    
    // Add some dummy notifications for testing
    m_notificationManager->addDummyNotifications();
}

void MainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    
    createActions();
    
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(m_toggleAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_statusAction);
    m_trayMenu->addAction(m_connectAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setIcon(QIcon(":/icons/relay-pc.png")); // You'll need to add this resource
    m_trayIcon->setToolTip("Relay PC - Notification Center");
    
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
    
    m_trayIcon->show();
}

void MainWindow::setupHotkeys()
{
    // Global hotkey for toggle (Ctrl+Shift+N)
    QShortcut* toggleShortcut = new QShortcut(QKeySequence("Ctrl+Shift+N"), this);
    connect(toggleShortcut, &QShortcut::activated, this, &MainWindow::togglePanel);
}

void MainWindow::createActions()
{
    m_toggleAction = new QAction("Toggle Notifications", this);
    connect(m_toggleAction, &QAction::triggered, this, &MainWindow::togglePanel);
    
    m_statusAction = new QAction("Status: Searching...", this);
    m_statusAction->setEnabled(false);
    
    m_connectAction = new QAction("Reconnect", this);
    connect(m_connectAction, &QAction::triggered, [this]() {
        m_notificationManager->startNetworkClient();
    });
    
    m_quitAction = new QAction("Quit", this);
    connect(m_quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);
}

void MainWindow::togglePanel()
{
    if (m_panelVisible) {
        hidePanel();
    } else {
        showPanel();
    }
}

void MainWindow::showPanel()
{
    if (!m_notificationPanel || m_panelVisible) {
        return;
    }
    
    m_animationManager->slideIn();
    m_panelVisible = true;
}

void MainWindow::hidePanel()
{
    if (!m_notificationPanel || !m_panelVisible) {
        return;
    }
    
    m_animationManager->slideOut();
    m_panelVisible = false;
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        togglePanel();
        break;
    default:
        break;
    }
}

void MainWindow::onServerConnected()
{
    if (m_statusAction) {
        m_statusAction->setText("Status: Connected");
    }
    if (m_connectAction) {
        m_connectAction->setEnabled(false);
    }
    if (m_trayIcon) {
        m_trayIcon->showMessage("Relay PC", "Connected to notification server", 
                              QSystemTrayIcon::Information, 3000);
    }
}

void MainWindow::onServerDisconnected()
{
    if (m_statusAction) {
        m_statusAction->setText("Status: Disconnected");
    }
    if (m_connectAction) {
        m_connectAction->setEnabled(true);
    }
}

void MainWindow::onConnectionError(const QString& error)
{
    if (m_statusAction) {
        m_statusAction->setText("Status: " + error);
    }
    if (m_connectAction) {
        m_connectAction->setEnabled(true);
    }
}
