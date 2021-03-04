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
    if (way.points.length() > 0)
    {
        QPolygonF poly;
        for (int i=0; i < way.points.count(); ++i)
            poly << way.points[i];

        QGraphicsPolygonItem *item = scene->addPolygon(poly, QPen(lineColor), QBrush(fillColor, Qt::SolidPattern));
        item->setZValue(zValue);
    }
}

void DrawPathWay(QGraphicsScene *scene, wayStruct way, QColor color, Qt::PenStyle style, float width, float zValue)
{
    if (way.points.length() > 1)
    {
        QPainterPath path;
        path.moveTo(way.points[0]);
        for (int i=1; i < way.points.count(); ++i)
            path.lineTo(way.points[i]);

        QPen pen = QPen(QBrush(color), width, style);
        QGraphicsPathItem *item = scene->addPath(path, pen);
        item->setZValue(zValue);
    }
}

void MainWindow::on_action_Open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open OSM File"), "../KelosQtOSM/SampleFiles", tr("Image Files (*.osm)"));

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

    wayStruct way;
    QList<wayStruct> wayList = QList<wayStruct>();

    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "way" || name == "relation")
            {
                wayList.append(way);
                way.points = QList<QPointF>();
                way.tags = QMap<QString,QString>();
            }

            if (name == "nd")
                way.points.append(m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()]);

            if (name == "tag")
                way.tags[xmlReader.attributes().value("k").toString()] = xmlReader.attributes().value("v").toString();
        }

        xmlReader.readNext();
    }

    for (int i=0; i < wayList.count(); ++i)
    {
        wayStruct way = wayList[i];

        if (way.tags.contains("building"))
        {
            DrawPolyWay(m_Scene, way, QColor(158, 136, 118), QColor(196, 182, 171), 10);

            if (way.tags.contains("name") && way.points.length() > 0)
            {
                QGraphicsTextItem *text = m_Scene->addText(way.tags["name"]);
                text->setDefaultTextColor(QColor(158, 136, 118));
                text->setPos(way.points[0]);
                text->setZValue(100);
            }
        }

        if (way.tags.contains("leisure"))
        {
            DrawPolyWay(m_Scene, way, QColor(156, 214, 191), QColor(170, 224, 203), 1);

            if (way.tags.contains("name") && way.points.length() > 0)
            {
                QGraphicsTextItem *text = m_Scene->addText(way.tags["name"]);
                text->setDefaultTextColor(Qt::GlobalColor::darkGreen);
                text->setPos(way.points[0]);
                text->setZValue(100);
            }
        }

        if (way.tags.contains("amenity"))
            DrawPolyWay(m_Scene, way, Qt::GlobalColor::gray, QColor(238, 238, 238), 1);

        if (way.tags.contains("natural"))
        {
            QColor color = QColor(205, 235, 176);
            if (way.tags["natural"] == "grass" || way.tags["natural"] == "grassland")
                color = QColor(205, 235, 176);
            else if (way.tags["natural"] == "wood")
                color = QColor(173, 209, 158);
            else if (way.tags["natural"] == "scrub")
                color = QColor(200, 215, 171);

            DrawPolyWay(m_Scene, way, color, color, 0);
        }

        if (way.tags.contains("landuse"))
        {
            QColor color = QColor(205, 235, 176);
            QString v = way.tags["landuse"];
            if (v == "industrial")
                color = QColor(236, 219, 232);
            else if (v == "brownfield")
                color = QColor(198, 199, 180);
            else if (v == "residential")
                color = QColor(224, 223, 223);
            else if (v == "forest")
                color = QColor(171, 210, 159);

            DrawPolyWay(m_Scene, way, color, color, 0);
        }

        if (way.tags.contains("railway"))
            DrawPathWay(m_Scene, way, Qt::GlobalColor::darkGray, Qt::PenStyle::SolidLine, 2, 2);

        if (way.tags.contains("highway"))
        {
            QString v = way.tags["highway"];
            if (v == "footpath" || v == "path" || v == "footway")
            {
                QColor color = Qt::GlobalColor::red;
                if (way.tags.contains("bicycle"))
                    color = Qt::GlobalColor::blue;

                DrawPathWay(m_Scene, way, color, Qt::PenStyle::DotLine, 1, 3);
                DrawPathWay(m_Scene, way, Qt::white, Qt::PenStyle::SolidLine, 1.5, 2);
            }
            else if (v == "track")
            {
                DrawPathWay(m_Scene, way, Qt::GlobalColor::darkYellow, Qt::PenStyle::DashDotLine, 1, 3);
                DrawPathWay(m_Scene, way, Qt::white, Qt::PenStyle::SolidLine, 1.5, 2);
            }
            else if (v == "steps")
            {
                DrawPathWay(m_Scene, way, Qt::GlobalColor::red, Qt::PenStyle::DotLine, 1.2, 3);
                DrawPathWay(m_Scene, way, Qt::white, Qt::PenStyle::SolidLine, 1.5, 2);
            }
            else
            {
                if (way.tags.contains("area"))
                {
                    DrawPolyWay(m_Scene, way, Qt::GlobalColor::gray, QColor(238, 238, 238), 1);
                }
                else
                {
                    DrawPathWay(m_Scene, way, Qt::black, Qt::PenStyle::SolidLine, 5, 5);
                    DrawPathWay(m_Scene, way, Qt::white, Qt::PenStyle::SolidLine, 4, 6);
                }
            }
        }

        if (way.tags.contains("aeroway"))
        {
            QString v = way.tags["aeroway"];
            if (v == "runway")
                DrawPolyWay(m_Scene, way, QColor(187, 187, 204), QColor(187, 187, 204), 0);
            if (v == "apron")
                DrawPolyWay(m_Scene, way, QColor(218, 218, 224), QColor(218, 218, 224), 0);
            else if (v == "taxiway")
                DrawPathWay(m_Scene, way, QColor(187, 187, 204), Qt::PenStyle::SolidLine, 10, 1);
            else
                DrawPathWay(m_Scene, way, Qt::gray, Qt::PenStyle::SolidLine, 5, 2);
        }

        if (way.tags.contains("waterway"))
        {
            DrawPathWay(m_Scene, way, QColor(170, 210, 223), Qt::PenStyle::SolidLine, 5, 1);
        }

    }
}

