#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <string>

QtCharts::QLineSeries *series = new QtCharts::QLineSeries();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //![1]
    //*series = new QtCharts::QLineSeries();
    //![1]

    //![2]
    series->setUseOpenGL(true);
    series->append(0, 6);
    series->append(2, 4);
    series->append(3, 8);
    series->append(7, 4);
    series->append(10, 5);
    *series << QPointF(11, 1) << QPointF(13, 3) << QPointF(17, 6) << QPointF(18, 3) << QPointF(20, 2);
    //![2]

    //![3]
    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->legend()->hide();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->axisX()->setRange(0, 1024);
    chart->axisY()->setRange(-1, 4);
    chart->setTitle("Simple line chart example");
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    //![3]

    //![4]
    ui->plotAxes->setChart(chart);
    ui->plotAxes->setRenderHint(QPainter::Antialiasing);
    //![4]
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processOneThing() {

    int colCount = 1024;
    QVector<QPointF> points;
    points.reserve(colCount);
    for (int j(0); j < colCount; j++) {
        qreal x(0);
        qreal y(0);
        // data with sin + random component
        y = qSin(3.14159265358979 / 50 * j) + 0.5 + (qreal) rand() / (qreal) RAND_MAX;
        x = j;
        points.append(QPointF(x, y));
    }

    series->replace(points);

    //std::string myString("Hello World");
    //std::cout << "* Debug " << myString << std::endl;
}

void MainWindow::on_playButton_clicked()
{
    ui->label_3->setText(QString::number(123));

    MainWindow::processOneThing();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(processOneThing()));
    timer->start(20);
}
