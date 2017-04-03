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
	
	series = new QtCharts::QLineSeries();
    series->setUseOpenGL(true);
	series->replace(points);

	series2 = new QtCharts::QLineSeries();
	series2->setUseOpenGL(true);
	series2->replace(points2);
	
	chart = new QtCharts::QChart();
    chart->legend()->hide();
    chart->addSeries(series);
	chart->addSeries(series2);
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
		QVector<QPointF> data;
		data.reserve(scanResults.nrSteps);
		for (int j(0); j < scanResults.nrSteps; j++) {
			//data.append(QPointF(scanResults.voltages[j] / double(1e6), scanResults.intensity[j] / double(1e3)));
			data.append(QPointF(scanResults.voltages[j] / double(1e6), std::real(scanResults.amplitudes.A1[j]) / double(1e3)));
		}
		series->replace(data);
		chart->axisX()->setRange(0, 2);
		//chart->axisY()->setRange(-0.05, 0.05);
	}
	else {
		QVector<QPointF> points = d.getBuffer(channel);
		series->replace(points);
		chart->axisX()->setRange(0, points.length());
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