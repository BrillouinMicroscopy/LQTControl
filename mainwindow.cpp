#include "mainwindow.h"
#include "daq.h"
#include "ui_mainwindow.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <string>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {

    ui->setupUi(this);

	QWidget::connect(&timer, &QTimer::timeout,
		this, &MainWindow::updatePlot);

	QVector<QPointF> points = d.getData();
	QVector<QPointF> points2 = d.getData();
	
	plots.series = new QtCharts::QLineSeries();
	plots.series->setUseOpenGL(true);
	plots.series->replace(points);

	plots.intensity = new QtCharts::QLineSeries();
	plots.intensity->setUseOpenGL(true);
	plots.intensity->replace(points2);
	plots.intensity->setVisible(1);

	plots.A1 = new QtCharts::QLineSeries();
	plots.A1->setUseOpenGL(true);
	plots.A1->replace(points2);
	plots.A1->setVisible(1);

	plots.A2 = new QtCharts::QLineSeries();
	plots.A2->setUseOpenGL(true);
	plots.A2->replace(points2);
	plots.A2->setVisible(1);

	plots.quotients = new QtCharts::QLineSeries();
	plots.quotients->setUseOpenGL(true);
	plots.quotients->replace(points2);
	plots.quotients->setVisible(1);
	
	chart = new QtCharts::QChart();
    chart->legend()->hide();
    chart->addSeries(plots.series);
	chart->addSeries(plots.intensity);
	chart->addSeries(plots.A1);
	chart->addSeries(plots.A2);
	chart->addSeries(plots.quotients);
    chart->createDefaultAxes();
    chart->axisX()->setRange(0, 1024);
    chart->axisY()->setRange(-1, 4);
    chart->setTitle("Simple line chart example");
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    ui->plotAxes->setChart(chart);
    ui->plotAxes->setRenderHint(QPainter::Antialiasing);

	SCAN_PARAMETERS scanParameters = d.getScanParameters();
	ui->scanAmplitude->setValue(scanParameters.amplitude / double(1e6));
	ui->scanOffset->setValue(scanParameters.offset / double(1e6));
	ui->scanFrequency->setValue(scanParameters.frequency / double(1e6));
	ui->scanWaveform->setCurrentIndex(scanParameters.waveform);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_acquisitionButton_clicked() {
	bool running = d.startStopAcquisition();
	if (running) {
		ui->acquisitionButton->setText(QString("Stop"));
	} else {
		ui->acquisitionButton->setText(QString("Acquire"));
	}
}

void MainWindow::updatePlot() {
	if (channel == 4) {
		SCAN_RESULTS scanResults = d.getScanResults();
		QVector<QPointF> intensity;
		intensity.reserve(scanResults.nrSteps);
		QVector<QPointF> A1;
		A1.reserve(scanResults.nrSteps);
		QVector<QPointF> A2;
		A2.reserve(scanResults.nrSteps);
		QVector<QPointF> quotients;
		quotients.reserve(scanResults.nrSteps);
		for (int j(0); j < scanResults.nrSteps; j++) {
			//data.append(QPointF(scanResults.voltages[j] / double(1e6), scanResults.intensity[j] / double(1e3)));
			intensity.append(QPointF(scanResults.voltages[j] / double(1e6), scanResults.intensity[j] / double(1e3)));
			A1.append(QPointF(scanResults.voltages[j] / double(1e6), 5*std::real(scanResults.amplitudes.A1[j]) / double(1e3)));
			A2.append(QPointF(scanResults.voltages[j] / double(1e6), 5*std::real(scanResults.amplitudes.A2[j]) / double(1e3)));
			quotients.append(QPointF(scanResults.voltages[j] / double(1e6), 5*scanResults.quotients[j] / double(1e3)));
		}
		plots.intensity->replace(intensity);
		plots.A1->replace(A1);
		plots.A2->replace(A2);
		plots.quotients->replace(quotients);

		plots.intensity->setVisible(1);
		plots.A1->setVisible(1);
		plots.A2->setVisible(1);
		plots.quotients->setVisible(1);

		chart->axisX()->setRange(0, 2);
		chart->axisY()->setRange(-0.4, 1.2);
	}
	else {
		QVector<QPointF> points = d.getBuffer(channel);
		plots.series->replace(points);
		chart->axisX()->setRange(0, points.length());

		plots.intensity->setVisible(0);
		plots.A1->setVisible(0);
		plots.A2->setVisible(0);
		plots.quotients->setVisible(0);
	}
}

void MainWindow::on_playButton_clicked() {
	if (timer.isActive()) {
		timer.stop();
		ui->playButton->setText(QString("Play"));
	} else {
		timer.start(20);
		ui->playButton->setText(QString("Stop"));
	}
}

void MainWindow::on_scanButtonManual_clicked() {
	d.scanManual();
}

void MainWindow::on_selectDisplay_activated(const int index) {
	switch (index) {
		case 0:
		case 1:
			channel = index;
			break;
		case 4:
			channel = index;
			break;
	}
}

void MainWindow::on_scanWaveform_activated(const int index) {
	d.setScanParameters(3, index);
}

void MainWindow::on_actionConnect_triggered() {
	bool connected = d.connect();
	if (connected) {
		ui->actionConnect->setEnabled(false);
		ui->actionDisconnect->setEnabled(true);
		ui->statusBar->showMessage("Successfully connected", 2000);
	} else {
		ui->actionConnect->setEnabled(true);
		ui->actionDisconnect->setEnabled(false);
	}
}

void MainWindow::on_actionDisconnect_triggered() {
	bool connected = d.disconnect();
	if (connected) {
		ui->actionConnect->setEnabled(false);
		ui->actionDisconnect->setEnabled(true);
	}  else {
		ui->actionConnect->setEnabled(true);
		ui->actionDisconnect->setEnabled(false);
		ui->statusBar->showMessage("Successfully disconnected", 2000);
	}
}

void MainWindow::on_scanButton_clicked() {
	d.set_sig_gen();
}