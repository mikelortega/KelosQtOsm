#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QXmlStreamReader>
#include "UTM.h"
#include <map>
#include <QGraphicsItem>

std::map<qlonglong, QPointF> m_NodeGeoLocs;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_Scene = new QGraphicsScene(this);
    m_Scene->setBackgroundBrush(QBrush(QColor(242, 239, 233), Qt::SolidPattern));

    ui->graphicsView->setScene(m_Scene);
    ui->graphicsView->setRenderHint( QPainter::Antialiasing );
    ui->graphicsView->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

struct wayStruct
{
    QList<QPointF> points;
    QMap<QString, QString> tags;
};

void DrawPolyWay(QGraphicsScene *scene, wayStruct way, QColor lineColor, QColor fillColor, float zValue)
{
    QPolygonF poly;
    for (int i=0; i < way.points.count(); ++i)
        poly << way.points[i];

    QGraphicsPolygonItem *item = scene->addPolygon(poly, QPen(lineColor), QBrush(fillColor, Qt::SolidPattern));
    item->setZValue(zValue);
}

void MainWindow::on_action_Open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open OSM File"), "../KelosQtOSM/SampleFiles", tr("Image Files (*.osm)"));

    QFile file(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::information(this, "Error", "Cannot read file: " + file.errorString());
        return;
    }

    m_Scene->clear();

    QXmlStreamReader xmlReader(&file);

    //
    // Get all nodes and their positions
    //
    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "node")
            {
                qlonglong id = xmlReader.attributes().value("id").toLongLong();
                double lat = xmlReader.attributes().value("lat").toDouble();
                double lon = xmlReader.attributes().value("lon").toDouble();

                double x, y;
                LatLonToUTMXY(lat, lon, 30, x, y);

                m_NodeGeoLocs[id] = QPointF(x, -y);
            }
        }

        xmlReader.readNext();
    }

    //
    // Parse again to find features
    //
    file.seek(0); // to make QFile object pointing to begining
    xmlReader.setDevice(xmlReader.device());

    QPolygonF poly;
    QPainterPath path;

    wayStruct way;
    QList<wayStruct> wayList = QList<wayStruct>();

    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "way" || name == "relation")
            {
                poly.clear();
                path = QPainterPath();

                wayList.append(way);
                way.points = QList<QPointF>();
                way.tags = QMap<QString,QString>();
            }
            if (name == "nd")
            {
                poly << m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()];
                if (path.currentPosition() == QPointF()) // First point
                    path.moveTo(m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()]);
                else // Draw line
                    path.lineTo(m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()]);

                way.points.append(m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()]);
            }

            if (name == "tag")
            {
                way.tags[xmlReader.attributes().value("k").toString()] = xmlReader.attributes().value("v").toString();

                if (xmlReader.attributes().value("k") == "natural")
                {
                    QStringRef v = xmlReader.attributes().value("v");
                    if (v == "grass" || v == "grassland")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(205, 235, 176), Qt::SolidPattern));
                    else if (v == "wood")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(173, 209, 158), Qt::SolidPattern));
                    else if (v == "scrub")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(200, 215, 171), Qt::SolidPattern));
                }

                if (xmlReader.attributes().value("k") == "landuse")
                {
                    QColor color = QColor(205, 235, 176);
                    QStringRef v = xmlReader.attributes().value("v");
                    if (v == "industrial")
                        color = QColor(236, 219, 232);
                    if (v == "brownfield")
                        color = QColor(198, 199, 180);
                    if (v == "residential")
                        color = QColor(224, 223, 223);
                    if (v == "forest")
                        color = QColor(171, 210, 159);

                    m_Scene->addPolygon(poly, Qt::NoPen, QBrush(color, Qt::SolidPattern));
                }

                if (xmlReader.attributes().value("k") == "railway")
                {
                    QPen pen = QPen(QBrush(Qt::GlobalColor::darkGray), 2);
                    QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                    item->setZValue(2);
                }

                if (xmlReader.attributes().value("k") == "highway")
                {
                    QStringRef v = xmlReader.attributes().value("v");
                    if (v == "footpath" || v == "path" || v == "footway")
                    {
                        QColor color = Qt::GlobalColor::red;
                        if (v == "path")
                            color = Qt::GlobalColor::blue;

                        QPen pen = QPen(QBrush(color), 1, Qt::DotLine);
                        QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                        item->setZValue(3);
                        pen = QPen(QBrush(Qt::white), 1.5);
                        item = m_Scene->addPath(path, pen);
                        item->setZValue(2);
                    }
                    else if (xmlReader.attributes().value("v") == "track")
                    {
                        QPen pen = QPen(QBrush(Qt::GlobalColor::darkYellow), 1, Qt::DashDotLine);
                        QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                        item->setZValue(3);
                        pen = QPen(QBrush(Qt::white), 1.5);
                        item = m_Scene->addPath(path, pen);
                        item->setZValue(2);
                    }
                    else if (xmlReader.attributes().value("v") == "steps")
                    {
                        QPen pen = QPen(QBrush(Qt::GlobalColor::red), 1.5, Qt::DotLine);
                        QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                        item->setZValue(2);
                    }
                    else
                    {
                        QPen pen = QPen(QBrush(Qt::black), 5);
                        QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                        item->setZValue(5);
                        pen = QPen(QBrush(Qt::white), 4);
                        item = m_Scene->addPath(path, pen);
                        item->setZValue(6);
                        //if (xmlReader.attributes().value("v") == "service")
                        //{
                        //    QPen pen = QPen(QBrush(Qt::gray), 2, Qt::DotLine);
                        //    item = m_Scene->addPath(path, pen);
                        //    item->setZValue(4);
                        //}
                    }
                }

                if (xmlReader.attributes().value("k") == "aeroway")
                {
                    QStringRef v = xmlReader.attributes().value("v");
                    if (v == "runway")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(187, 187, 204), Qt::SolidPattern));
                    if (v == "apron")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(218, 218, 224), Qt::SolidPattern));
                    else if (v == "taxiway")
                    {
                        QGraphicsPathItem *item = m_Scene->addPath(path, QPen(QBrush(QColor(187, 187, 204)), 10));
                        item->setZValue(1);
                    }
                    else
                    {
                        QGraphicsPathItem *item = m_Scene->addPath(path, QPen());
                        item->setZValue(2);
                    }
                }

                if (xmlReader.attributes().value("k") == "waterway")
                {
                    QPen pen = QPen(QBrush(QColor(170, 210, 223)), 5);
                    QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                    item->setZValue(1);
                }
            }
        }

        xmlReader.readNext();
    }

    for (int i=0; i < wayList.count(); ++i)
    {
        wayStruct way = wayList[i];

        if (way.tags.contains("building"))
            DrawPolyWay(m_Scene, way, QColor(158, 136, 118), QColor(196, 182, 171), 10);

        if (way.tags.contains("leisure"))
            DrawPolyWay(m_Scene, way, QColor(156, 214, 191), QColor(170, 224, 203), 1);

        if (way.tags.contains("amenity"))
            DrawPolyWay(m_Scene, way, Qt::GlobalColor::gray, QColor(238, 238, 238), 1);
    }
}

