#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include <QObject>
#include <QPropertyAnimation>
#include <QEasingCurve>

class QWidget;

class AnimationManager : public QObject
{
    Q_OBJECT

public:
    explicit AnimationManager(QWidget* targetWidget, QObject *parent = nullptr);
    ~AnimationManager();

    void slideIn();
    void slideOut();
    void fadeIn();
    void fadeOut();

signals:
    void animationFinished();

private slots:
    void onAnimationFinished();

private:
    void setupAnimations();
    QRect getHiddenPosition();
    QRect getVisiblePosition();
    
    QWidget* m_targetWidget;
    QPropertyAnimation* m_slideAnimation;
    QPropertyAnimation* m_fadeAnimation;
    
    bool m_isVisible;
    bool m_isAnimatingOut; // Track if we're animating out (for hiding)
    
    static constexpr int ANIMATION_DURATION = 300;
};

#endif // ANIMATIONMANAGER_H
