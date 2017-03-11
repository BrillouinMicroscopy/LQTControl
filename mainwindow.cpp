#include "mainwindow.h"
#include "daq.h"
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

	daq d;

	QWidget::connect(&timer, &QTimer::timeout,
		this, &MainWindow::processOneThing);

	new QtCharts::QLineSeries();

	int colCount = 1024;
	QVector<QPointF> points = d.acquire(colCount);
	
    series->setUseOpenGL(true);
	series->replace(points);

    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->legend()->hide();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->axisX()->setRange(0, 1024);
    chart->axisY()->setRange(-1, 4);
    chart->setTitle("Simple line chart example");
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    ui->plotAxes->setChart(chart);
    ui->plotAxes->setRenderHint(QPainter::Antialiasing);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processOneThing() {

	daq d;

    int colCount = 1024;
    QVector<QPointF> points = d.acquire(colCount);

    series->replace(points);
}

void MainWindow::on_playButton_clicked()
{
	if (timer.isActive()) {
		timer.stop();
		ui->playButton->setText(QString("Play"));
	}
	else {
		timer.start(20);
		ui->playButton->setText(QString("Stop"));
	}
}
