#include "mainwindow.h"
#include "daq.h"
#include "ui_mainwindow.h"
#include "colors.h"
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

	QWidget::connect(&d, SIGNAL(scanDone()), this, SLOT(updateScanView()));
	QWidget::connect(&d, SIGNAL(collectedData()), this, SLOT(updateLiveView()));

	QVector<QPointF> points = d.getData();
	QVector<QPointF> points2 = d.getData();
	
	// set up live view plots
	liveViewPlots.seriesA = new QtCharts::QLineSeries();
	liveViewPlots.seriesA->setUseOpenGL(true);
	liveViewPlots.seriesA->replace(points);
	liveViewPlots.seriesA->setColor(colors.blue);

	liveViewPlots.seriesB = new QtCharts::QLineSeries();
	liveViewPlots.seriesB->setUseOpenGL(true);
	liveViewPlots.seriesB->replace(points2);
	liveViewPlots.seriesB->setColor(colors.orange);
	
	// set up live view chart
	liveViewChart = new QtCharts::QChart();
	liveViewChart->legend()->hide();
	liveViewChart->addSeries(liveViewPlots.seriesA);
	liveViewChart->addSeries(liveViewPlots.seriesB);
	liveViewChart->createDefaultAxes();
	liveViewChart->axisX()->setRange(0, 1024);
	liveViewChart->axisY()->setRange(-1, 4);
	liveViewChart->setTitle("Live view");
	liveViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set up scan view plots
	scanViewPlots.intensity = new QtCharts::QLineSeries();
	scanViewPlots.intensity->setUseOpenGL(true);
	scanViewPlots.intensity->replace(points2);
	scanViewPlots.intensity->setColor(colors.orange);

	scanViewPlots.A1 = new QtCharts::QLineSeries();
	scanViewPlots.A1->setUseOpenGL(true);
	scanViewPlots.A1->replace(points2);
	scanViewPlots.A1->setColor(colors.yellow);

	scanViewPlots.A2 = new QtCharts::QLineSeries();
	scanViewPlots.A2->setUseOpenGL(true);
	scanViewPlots.A2->replace(points2);
	scanViewPlots.A2->setColor(colors.purple);

	scanViewPlots.quotients = new QtCharts::QLineSeries();
	scanViewPlots.quotients->setUseOpenGL(true);
	scanViewPlots.quotients->replace(points2);
	scanViewPlots.quotients->setColor(colors.green);

	// set up live view chart
	scanViewChart = new QtCharts::QChart();
	scanViewChart->legend()->hide();
	scanViewChart->addSeries(scanViewPlots.intensity);
	scanViewChart->addSeries(scanViewPlots.A1);
	scanViewChart->addSeries(scanViewPlots.A2);
	scanViewChart->addSeries(scanViewPlots.quotients);
	scanViewChart->createDefaultAxes();
	scanViewChart->axisX()->setRange(0, 1024);
	scanViewChart->axisY()->setRange(-1, 4);
	scanViewChart->setTitle("Scan View");
	scanViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set live view chart as default chart
    ui->plotAxes->setChart(liveViewChart);
    ui->plotAxes->setRenderHint(QPainter::Antialiasing);

	// set default values of GUI elements
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

void MainWindow::updateLiveView() {
	if (view == 0) {
		QVector<QPointF> points = d.getBuffer(0);
		liveViewPlots.seriesA->replace(points);
		liveViewChart->axisX()->setRange(0, points.length());
	}
}

void MainWindow::updateScanView() {
	if (view == 2) {
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
			intensity.append(QPointF(scanResults.voltages[j] / double(1e6), scanResults.intensity[j] / double(1e3)));
			A1.append(QPointF(scanResults.voltages[j] / double(1e6), std::real(scanResults.amplitudes.A1[j]) / double(1e3)));
			A2.append(QPointF(scanResults.voltages[j] / double(1e6), std::real(scanResults.amplitudes.A2[j]) / double(1e3)));
			quotients.append(QPointF(scanResults.voltages[j] / double(1e6), scanResults.quotients[j] / double(20.0)));
		}
		scanViewPlots.intensity->replace(intensity);
		scanViewPlots.A1->replace(A1);
		scanViewPlots.A2->replace(A2);
		scanViewPlots.quotients->replace(quotients);

		scanViewChart->axisX()->setRange(0, 2);
		scanViewChart->axisY()->setRange(-0.4, 1.2);
	}
}

void MainWindow::on_scanButtonManual_clicked() {
	d.scanManual();
}

void MainWindow::on_selectDisplay_activated(const int index) {
	view = index;
	switch (index) {
		case 0:
			// it is necessary to hide the series, because they do not get removed
			// after setChart in case useOpenGL == true (bug in QT?)
			scanViewPlots.intensity->setVisible(false);
			scanViewPlots.A1->setVisible(false);
			scanViewPlots.A2->setVisible(false);
			scanViewPlots.quotients->setVisible(false);
			liveViewPlots.seriesA->setVisible(true);
			liveViewPlots.seriesB->setVisible(true);

			ui->plotAxes->setChart(liveViewChart);
			break;
		case 1:
			break;
		case 2:
			// it is necessary to hide the series, because they do not get removed
			// after setChart in case useOpenGL == true (bug in QT?)
			liveViewPlots.seriesA->setVisible(false);
			liveViewPlots.seriesB->setVisible(false);
			scanViewPlots.intensity->setVisible(true);
			scanViewPlots.A1->setVisible(true);
			scanViewPlots.A2->setVisible(true);
			scanViewPlots.quotients->setVisible(true);
			ui->plotAxes->setChart(scanViewChart);
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