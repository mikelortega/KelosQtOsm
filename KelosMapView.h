#ifndef __KELOSMAPVIEW_H__
#define __KELOSMAPVIEW_H__

#include <QGraphicsView>

class KelosMapView : public QGraphicsView
{
    Q_OBJECT

public:

    KelosMapView(QWidget *parent = 0);
    ~KelosMapView();

    inline float getZoom() {return m_fZoomValue;}

public slots:

    void FitSceneAt(QPointF const& center, QRectF const& AbsoluteRect);
    void FitScene();
    void CenterMapOnIndicator(); 

signals:

    void MapZoomChangeSignal(float zoom);
    void MouseRelease();

private:

	// Zoooom //
	void wheelEvent(QWheelEvent* event);

	// Scroll with middle mouse button //
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	const float ZoomFactor;
	float		m_fZoomValue;

};

#endif //__KELOSMAPVIEW_H__
