#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QXmlStreamReader>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    m_Scene = new QGraphicsScene(this);
    m_Scene->addText("Hello, world!");

    QPolygonF poly;
    poly << QPointF(10, 10) << QPointF(10, 50) << QPointF(30, 70 )<< QPointF(60, 50) << QPointF(50, 10);
    QPen pen(Qt::green);
    QBrush brush;
    brush.setColor(Qt::red);
    brush.setStyle(Qt::SolidPattern);
    m_Scene->addPolygon(poly, pen, brush);

    ui->graphicsView->setScene(m_Scene);
    ui->graphicsView->show();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_Open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open OSM File"), "", tr("Image Files (*.osm)"));

    QFile file(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Text))
        QMessageBox::information(this, "Error", "Cannot read file: " + file.errorString());

    QXmlStreamReader xmlReader(&file);

    while (!xmlReader.isEndDocument())
    {
        if (xmlReader.isStartElement())
        {
            QString name = xmlReader.name().toString();
            if (name == "node")
            {
                double lat = xmlReader.attributes().value("lat").toDouble();
                double lon = xmlReader.attributes().value("lon").toDouble();

                //QPoint()
                //QMessageBox::information(this, "Error", "lat: " + QString::number(lat));
            }
        }

        xmlReader.readNext();
    }

}
