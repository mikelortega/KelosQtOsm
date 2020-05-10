#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QXmlStreamReader>
#include "UTM.h"
#include <map>

std::map<long, QPointF> m_NodeGeoLocs;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_Scene = new QGraphicsScene(this);

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

    QXmlStreamReader xmlReader(&file);

    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "node")
            {
                long id = xmlReader.attributes().value("id").toLong();
                double lat = xmlReader.attributes().value("lat").toDouble();
                double lon = xmlReader.attributes().value("lon").toDouble();

                double x, y;
                LatLonToUTMXY(lat, lon, 30, x, y);

                m_NodeGeoLocs[id] = QPointF(x * 1, -y * 1);
            }
        }

        xmlReader.readNext();
    }

    file.seek(0); // to make QFile object pointing to begining
    xmlReader.setDevice(xmlReader.device());

    QPolygonF poly;

    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "way")
                poly.clear();
            if (name == "nd")
                poly << m_NodeGeoLocs[xmlReader.attributes().value("ref").toLong()];
            if (name == "tag" && xmlReader.attributes().value("k") == "building")
                m_Scene->addPolygon(poly, QPen(), QBrush(Qt::lightGray, Qt::SolidPattern));
        }

        xmlReader.readNext();
    }
}
