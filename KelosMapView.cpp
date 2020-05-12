#include "KelosMapView.h"

#include <QGraphicsItem>
#include <QWheelEvent>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <iostream>
#include <math.h>

KelosMapView::KelosMapView(QWidget *parent) :
	QGraphicsView(parent),
	ZoomFactor(1.2f),
	m_fZoomValue(1.0f)
{
    setResizeAnchor(QGraphicsView::AnchorUnderMouse); // anchor under the cursor
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // Hacer que el zoom se haga donde está el ratón

	this->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::FullViewportUpdate);
}

KelosMapView::~KelosMapView()
{
}

void KelosMapView::FitScene()
{
	this->centerOn(0,0);
	fitInView(this->scene()->sceneRect(),Qt::KeepAspectRatio);

    m_fZoomValue = matrix().m11();
    emit MapZoomChangeSignal(m_fZoomValue);
}

void KelosMapView::CenterMapOnIndicator()
{
//	this->centerOn(static_cast<QGraphicsScene*>(this->scene())->GetPositionIndicatorScenePos());
}

void KelosMapView::FitSceneAt(QPointF const& center, QRectF const& rect)
{
	this->centerOn(center);
	fitInView(rect,Qt::KeepAspectRatio);

    m_fZoomValue = matrix().m11();
    emit MapZoomChangeSignal(m_fZoomValue);
}

void KelosMapView::wheelEvent(QWheelEvent* event)
{
    int numSteps = event->delta() / 15 / 8;

    if (numSteps == 0) {
        event->ignore();
        return;
    }
	float fFactor = pow(ZoomFactor, numSteps);

	QTransform NewTransform = this->transform();
	NewTransform.scale(fFactor, fFactor);
	float fZoom = NewTransform.m11();
	// Limitar el zoom
	if ( fZoom >= 0.01 && fZoom < 100.0)
	{
		this->hide();
		this->resetTransform();
		this->setTransform(NewTransform);
		this->show(); // AS: Prevenir artifacts
		m_fZoomValue = fZoom ;
		emit MapZoomChangeSignal(m_fZoomValue);
	}
	event->accept();
}

void KelosMapView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton)
    {
        setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
		QMouseEvent fake(event->type(), event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
		QGraphicsView::mousePressEvent(&fake);
		
    }
	else if (event->button() == Qt::LeftButton && event->modifiers() & Qt::KeyboardModifier::ShiftModifier)
	{
		setDragMode(QGraphicsView::DragMode::RubberBandDrag); // Importante el orden.. tras el evento!
		QGraphicsView::mousePressEvent(event);
	}
	else
		QGraphicsView::mousePressEvent(event);

}

void KelosMapView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton)
    {
		QMouseEvent fake(event->type(), event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
		QGraphicsView::mousePressEvent(&fake);
		setCursor(Qt::ArrowCursor);
		setDragMode(QGraphicsView::DragMode::NoDrag); // Importante el orden.. tras el evento!
	}	
	else if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) )
	{
		QGraphicsView::mouseReleaseEvent(event);
		setDragMode(QGraphicsView::DragMode::NoDrag); // Importante el orden.. tras el evento!
	}
	else
		QGraphicsView::mouseReleaseEvent(event);
}
