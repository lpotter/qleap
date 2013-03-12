/* QLeapListener.cpp --- 
 * 
 * Author: Julien Wintz
 * Copyright (C) 2008-2011 - Julien Wintz, Inria.
 * Created: Tue Dec 18 17:34:10 2012 (+0100)
 * Version: $Id$
 * Last-Updated: Tue Mar 12 14:17:39 2013 (+0100)
 *           By: Julien Wintz
 *     Update #: 1158
 */

/* Commentary: 
 * 
 */

/* Change log:
 * 
 */

#include "QLeap"
#include "QLeapListener"
#include "QLeapListener_p"
#include "QLeapMapper"

#include <QtCore>
#include <QtDebug>
#include <QtWidgets>

// /////////////////////////////////////////////////////////////////
// QLeapListenerPrivate
// /////////////////////////////////////////////////////////////////

QLeapListenerPrivate::QLeapListenerPrivate(void) : Leap::Listener()
{
    this->prev_touch_event_type = QEvent::None;
    this->curr_touch_event_type = QEvent::None;
    this->prev_mouse_event_type = QEvent::None;
    this->curr_mouse_event_type = QEvent::None;

    this->prev_touch_count = 0;
    this->curr_touch_count = 0;

    this->start = NULL;
}

void QLeapListenerPrivate::onInit(const Leap::Controller& controller)
{
    Q_UNUSED(controller);
}

void QLeapListenerPrivate::onConnect(const Leap::Controller& controller)
{
    Q_UNUSED(controller);
}

void QLeapListenerPrivate::onDisconnect(const Leap::Controller& controller)
{
    Q_UNUSED(controller);
}

void QLeapListenerPrivate::onFrame(const Leap::Controller& controller)
{
    this->prev_touch_count = this->curr_touch_count;
    this->curr_touch_count = 0;

    this->prev_touch_event_type = this->curr_touch_event_type;
    this->curr_touch_event_type = QEvent::None;

    this->prev_mouse_event_type = this->curr_touch_event_type;
    this->curr_mouse_event_type = QEvent::None;

    QList<QTouchEvent::TouchPoint> points;

    const Leap::Screen screen = controller.calibratedScreens()[0];
    const Leap::HandList hands = controller.frame().hands();

    for(int h = 0; h < hands.count(); h++) {
                
        const Leap::FingerList fingers = hands[h].fingers();
        
        for(int f = 0; f < fingers.count(); f++) {
           
            const Leap::Finger finger = fingers[f];

            QTouchEvent::TouchPoint point(finger.id());

            if(this->start) {
                QPointF position = qleap_pointf(screen.intersect(this->start->hands()[h].fingers()[f], true));
                point.setStartPos(QLeapMapper::mapToSpace(position));
                point.setStartScenePos(QLeapMapper::mapToScene(position));
                point.setStartScreenPos(QLeapMapper::mapToScreen(position));
            }

            if(true) {
                QPointF position = qleap_pointf(screen.intersect(finger, true));
                point.setPos(QLeapMapper::mapToSpace(position));
                point.setScenePos(QLeapMapper::mapToScene(position));
                point.setScreenPos(QLeapMapper::mapToScreen(position));
            }

            if(this->prev_touch_event_type == QEvent::TouchBegin || this->prev_touch_event_type == QEvent::TouchUpdate) {
                QPointF position = qleap_pointf(screen.intersect(controller.frame(1).hands()[h].fingers()[f], true));
                point.setLastPos(QLeapMapper::mapToSpace(position));
                point.setLastScenePos(QLeapMapper::mapToScene(position));
                point.setLastScreenPos(QLeapMapper::mapToScreen(position));
            }

            points << point;
        }
    }

    this->curr_touch_count = points.count();

    ( curr_touch_count && !prev_touch_count) ? curr_touch_event_type = QEvent::TouchBegin :
    ( curr_touch_count &&  prev_touch_count) ? curr_touch_event_type = QEvent::TouchUpdate :
    (!curr_touch_count &&  prev_touch_count) ? curr_touch_event_type = QEvent::TouchEnd :
    (!curr_touch_count && !prev_touch_count) ? curr_touch_event_type = QEvent::TouchCancel : curr_touch_event_type = QEvent::None;

    ( curr_touch_count &&  prev_touch_count) ? curr_mouse_event_type = QEvent::MouseMove : curr_mouse_event_type = QEvent::None;

    if(curr_touch_event_type == QEvent::TouchUpdate && curr_touch_count && curr_touch_count != prev_touch_count) {
        curr_touch_event_type = QEvent::TouchEnd;
	curr_mouse_event_type = QEvent::None;
        points.clear();
        curr_touch_count = 0;
    }

    if((this->curr_touch_event_type == QEvent::TouchEnd || this->curr_touch_event_type == QEvent::TouchCancel) && this->start) {
        delete this->start;
        this->start = NULL;
    }

    if (this->curr_touch_event_type == QEvent::TouchBegin) {
        this->start = new Leap::Frame(controller.frame());
    }

// ///////////////////////////////////////////////////////////////////
// Sending mouse events
// ///////////////////////////////////////////////////////////////////

    if(curr_mouse_event_type != QEvent::None) {
	foreach(QObject *target, this->targets) {
	    QWidget *widget = qobject_cast<QWidget *>(target);
	    if(!widget)
		continue;
	    QTouchEvent::TouchPoint point = points.first();
	    QCursor::setPos(point.screenPos().toPoint());
	    if(widget->geometry().contains(point.screenPos().toPoint())) {
		QCoreApplication::postEvent(widget, new QMouseEvent(this->curr_mouse_event_type, point.pos(), point.screenPos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier));
	    }
	}
    }

// ///////////////////////////////////////////////////////////////////
// Sending touch event
// ///////////////////////////////////////////////////////////////////

	foreach(QObject *target, this->targets)
        QCoreApplication::postEvent(target, new QTouchEvent(this->curr_touch_event_type, qLeap, Qt::NoModifier, 0, points));
}

// /////////////////////////////////////////////////////////////////
// QLeapListener
// /////////////////////////////////////////////////////////////////

QLeapListener::QLeapListener(void) : d(new QLeapListenerPrivate)
{

}

QLeapListener::~QLeapListener(void)
{
    delete d;

    d = NULL;
}

void QLeapListener::addTarget(QObject *target)
{
    if(target && !d->targets.contains(target))
        d->targets << target;
}
