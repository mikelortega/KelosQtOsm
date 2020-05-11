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

    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "way" || name == "relation")
            {
                poly.clear();
                path = QPainterPath();
            }
            if (name == "nd")
            {
                poly << m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()];
                if (path.currentPosition() == QPointF()) // First point
                    path.moveTo(m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()]);
                else // Draw line
                    path.lineTo(m_NodeGeoLocs[xmlReader.attributes().value("ref").toLongLong()]);
            }

            if (name == "tag")
            {
                if (xmlReader.attributes().value("k") == "building")
                {
                    QGraphicsPolygonItem *item = m_Scene->addPolygon(poly, QPen(QColor(158, 136, 118)), QBrush(QColor(196, 182, 171), Qt::SolidPattern));
                    item->setZValue(5);
                }

                if (xmlReader.attributes().value("k") == "natural")
                {
                    if (xmlReader.attributes().value("v") == "grass" || xmlReader.attributes().value("v") == "grassland")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(205, 235, 176), Qt::SolidPattern));
                    else if (xmlReader.attributes().value("v") == "wood")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(173, 209, 158), Qt::SolidPattern));
                    else if (xmlReader.attributes().value("v") == "scrub")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(200, 215, 171), Qt::SolidPattern));
                }

                if (xmlReader.attributes().value("k") == "landuse")
                {
                    QColor color = QColor(205, 235, 176);
                    if (xmlReader.attributes().value("v") == "industrial")
                        color = QColor(236, 219, 232);
                    if (xmlReader.attributes().value("v") == "brownfield")
                        color = QColor(198, 199, 180);

                    m_Scene->addPolygon(poly, Qt::NoPen, QBrush(color, Qt::SolidPattern));
                }

                if (xmlReader.attributes().value("k") == "highway")
                {
                    if (xmlReader.attributes().value("v") == "footway" || xmlReader.attributes().value("v") == "footpath")
                    {
                        QPen pen = QPen(QBrush(Qt::blue), 1, Qt::DotLine);
                        QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                        item->setZValue(1);
                    }
                    else
                    {
                        QPen pen = QPen(QBrush(Qt::black), 5);
                        QGraphicsPathItem *item = m_Scene->addPath(path, pen);
                        item->setZValue(1);
                        pen = QPen(QBrush(Qt::white), 4);
                        item = m_Scene->addPath(path, pen);
                        item->setZValue(2);
                        if (xmlReader.attributes().value("v") == "service")
                        {
                            QPen pen = QPen(QBrush(Qt::gray), 2, Qt::DotLine);
                            item = m_Scene->addPath(path, pen);
                            item->setZValue(3);
                        }
                    }
                }

                if (xmlReader.attributes().value("k") == "aeroway")
                {
                    if (xmlReader.attributes().value("v") == "runway")
                        m_Scene->addPolygon(poly, Qt::NoPen, QBrush(QColor(187, 187, 204), Qt::SolidPattern));
                    else if (xmlReader.attributes().value("v") == "taxiway")
                        m_Scene->addPath(path, QPen(QBrush(QColor(187, 187, 204)), 10));
                    else
                    {
                        QGraphicsPathItem *item = m_Scene->addPath(path, QPen());
                        item->setZValue(1);
                    }
                }
            }
        }

        xmlReader.readNext();
    }

}
