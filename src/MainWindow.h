#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class NotificationPanel;
class NotificationManager;
class AnimationManager;
class NotificationPopupManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void togglePanel();
    void showPanel();
    void hidePanel();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupUI();
    void setupTrayIcon();
    void setupHotkeys();
    void createActions();
    
    NotificationPanel* m_notificationPanel;
    NotificationManager* m_notificationManager;
    AnimationManager* m_animationManager;
    NotificationPopupManager* m_popupManager;
    
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    QAction* m_toggleAction;
    QAction* m_quitAction;
    
    bool m_panelVisible;
};

#endif // MAINWINDOW_H
