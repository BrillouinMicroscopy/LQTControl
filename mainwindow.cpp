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
#include <QtCharts/QLegendMarker>
#include <QtCharts/QXYLegendMarker>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {

    ui->setupUi(this);

	QWidget::connect(&d, SIGNAL(scanDone()), this, SLOT(updateScanView()));
	QWidget::connect(&d, SIGNAL(collectedData()), this, SLOT(updateLiveView()));

	QVector<QPointF> points = d.getData();
	QVector<QPointF> points2 = d.getData();
	
	// set up live view plots
	QtCharts::QLineSeries *channelA = new QtCharts::QLineSeries();
	channelA->setUseOpenGL(true);
	channelA->replace(points);
	channelA->setColor(colors.blue);
	channelA->setName(QString("Detector signal"));
	liveViewPlots.append(channelA);

	QtCharts::QLineSeries *channelB = new QtCharts::QLineSeries();
	channelB->setUseOpenGL(true);
	channelB->replace(points2);
	channelB->setColor(colors.orange);
	channelB->setName(QString("Reference Signal"));
	liveViewPlots.append(channelB);
	
	// set up live view chart
	liveViewChart = new QtCharts::QChart();
	foreach(QtCharts::QLineSeries* series, liveViewPlots) {
		liveViewChart->addSeries(series);
	}
	liveViewChart->createDefaultAxes();
	liveViewChart->axisX()->setRange(0, 1024);
	liveViewChart->axisY()->setRange(-1, 4);
	liveViewChart->setTitle("Live view");
	liveViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set up scan view plots
	QtCharts::QLineSeries *intensity = new QtCharts::QLineSeries();
	intensity->setUseOpenGL(true);
	intensity->replace(points2);
	intensity->setColor(colors.orange);
	intensity->setName(QString("Intensity"));
	scanViewPlots.append(intensity);

	QtCharts::QLineSeries *A1 = new QtCharts::QLineSeries();
	A1->setUseOpenGL(true);
	A1->replace(points2);
	A1->setColor(colors.yellow);
	A1->setName(QString("Amplitude 1"));
	scanViewPlots.append(A1);

	QtCharts::QLineSeries *A2 = new QtCharts::QLineSeries();
	A2->setUseOpenGL(true);
	A2->replace(points2);
	A2->setColor(colors.purple);
	A2->setName(QString("Amplitude 2"));
	scanViewPlots.append(A2);

	QtCharts::QLineSeries *quotients = new QtCharts::QLineSeries();
	quotients->setUseOpenGL(true);
	quotients->replace(points2);
	quotients->setColor(colors.green);
	quotients->setName(QString("Quotient"));
	scanViewPlots.append(quotients);

	// set up live view chart
	scanViewChart = new QtCharts::QChart();
	foreach(QtCharts::QLineSeries* series, scanViewPlots) {
		scanViewChart->addSeries(series);
	}
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

	// connect legend marker to toggle visibility of plots
	MainWindow::connectMarkers();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::connectMarkers() {
	// Connect all markers to handler
	foreach(QtCharts::QLegendMarker* marker, liveViewChart->legend()->markers()) {
		// Disconnect possible existing connection to avoid multiple connections
		QWidget::disconnect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
		QWidget::connect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
	}
	foreach(QtCharts::QLegendMarker* marker, scanViewChart->legend()->markers()) {
		// Disconnect possible existing connection to avoid multiple connections
		QWidget::disconnect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
		QWidget::connect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
	}
}

void MainWindow::handleMarkerClicked() {
	QtCharts::QLegendMarker* marker = qobject_cast<QtCharts::QLegendMarker*> (sender());
	Q_ASSERT(marker);

	switch (marker->type()) {
		case QtCharts::QLegendMarker::LegendMarkerTypeXY: {
			// Toggle visibility of series
			marker->series()->setVisible(!marker->series()->isVisible());

			// Turn legend marker back to visible, since hiding series also hides the marker
			// and we don't want it to happen now.
			marker->setVisible(true);

			// Dim the marker, if series is not visible
			qreal alpha = 1.0;

			if (!marker->series()->isVisible()) {
				alpha = 0.5;
			}

			QColor color;
			QBrush brush = marker->labelBrush();
			color = brush.color();
			color.setAlphaF(alpha);
			brush.setColor(color);
			marker->setLabelBrush(brush);

			brush = marker->brush();
			color = brush.color();
			color.setAlphaF(alpha);
			brush.setColor(color);
			marker->setBrush(brush);

			QPen pen = marker->pen();
			color = pen.color();
			color.setAlphaF(alpha);
			pen.setColor(color);
			marker->setPen(pen);

			break;
		}
	}
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
		liveViewPlots[static_cast<int>(liveViewPlotTypes::CHANNEL_A)]->replace(points);
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
		scanViewPlots[static_cast<int>(scanViewPlotTypes::INTENSITY)]->replace(intensity);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::A1)]->replace(A1);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::A2)]->replace(A2);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::QUOTIENTS)]->replace(quotients);

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
			foreach(QtCharts::QLineSeries* series, scanViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, liveViewPlots) {
				series->setVisible(true);
			}
			ui->plotAxes->setChart(liveViewChart);
			break;
		case 1:
			break;
		case 2:
			// it is necessary to hide the series, because they do not get removed
			// after setChart in case useOpenGL == true (bug in QT?)
			foreach(QtCharts::QLineSeries* series, liveViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, scanViewPlots) {
				series->setVisible(true);
			}
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