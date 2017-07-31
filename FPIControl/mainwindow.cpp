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
	QWidget::connect(&d, SIGNAL(collectedBlockData(std::array<QVector<QPointF>, PS2000_MAX_CHANNELS>)), this,
		SLOT(updateLiveView(std::array<QVector<QPointF>, PS2000_MAX_CHANNELS>)));

	QWidget::connect(&d, SIGNAL(acquisitionParametersChanged(ACQUISITION_PARAMETERS)), this,
		SLOT(updateAcquisitionParameters(ACQUISITION_PARAMETERS)));
	
	// set up live view plots
	QtCharts::QLineSeries *channelA = new QtCharts::QLineSeries();
	channelA->setUseOpenGL(true);
	channelA->setColor(colors.blue);
	channelA->setName(QString("Detector signal"));
	liveViewPlots.append(channelA);

	QtCharts::QLineSeries *channelB = new QtCharts::QLineSeries();
	channelB->setUseOpenGL(true);
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
	liveViewChart->axisX()->setTitleText("Samples");
	liveViewChart->axisY()->setTitleText("Voltage [V]");
	liveViewChart->setTitle("Live view");
	liveViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set up scan view plots
	QtCharts::QLineSeries *intensity = new QtCharts::QLineSeries();
	intensity->setUseOpenGL(true);
	intensity->setColor(colors.orange);
	intensity->setName(QString("Intensity"));
	scanViewPlots.append(intensity);

	QtCharts::QLineSeries *A1 = new QtCharts::QLineSeries();
	A1->setUseOpenGL(true);
	A1->setColor(colors.yellow);
	A1->setName(QString("Amplitude 1"));
	scanViewPlots.append(A1);

	QtCharts::QLineSeries *A2 = new QtCharts::QLineSeries();
	A2->setUseOpenGL(true);
	A2->setColor(colors.purple);
	A2->setName(QString("Amplitude 2"));
	scanViewPlots.append(A2);

	QtCharts::QLineSeries *quotients = new QtCharts::QLineSeries();
	quotients->setUseOpenGL(true);
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
	scanViewChart->axisX()->setTitleText("Piezo Voltage [V]");
	scanViewChart->setTitle("Scan View");
	scanViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set live view chart as default chart
    ui->plotAxes->setChart(liveViewChart);
    ui->plotAxes->setRenderHint(QPainter::Antialiasing);
	ui->plotAxes->setRubberBand(QtCharts::QChartView::RectangleRubberBand);

	// set default values of GUI elements
	SCAN_PARAMETERS scanParameters = d.getScanParameters();
	ui->scanAmplitude->setValue(scanParameters.amplitude / static_cast<double>(1e6));
	ui->scanOffset->setValue(scanParameters.offset / static_cast<double>(1e6));
	ui->scanFrequency->setValue(scanParameters.frequency);
	ui->scanSteps->setValue(scanParameters.nrSteps);
	ui->scanWaveform->setCurrentIndex(scanParameters.waveform);

	// set default values of lockin parameters
	LOCKIN_PARAMETERS lockInParameters = d.getLockInParameters();
	ui->proportionalTerm->setValue(lockInParameters.proportional);
	ui->integralTerm->setValue(lockInParameters.integral);
	ui->derivativeTerm->setValue(lockInParameters.derivative);

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

void MainWindow::on_sampleRate_activated(const int index) {
	d.setSampleRate(index);
}

void MainWindow::on_chACoupling_activated(const int index) {
	d.setCoupling(index, 0);
}

void MainWindow::on_chBCoupling_activated(const int index) {
	d.setCoupling(index, 1);
}

void MainWindow::on_chARange_activated(const int index) {
	d.setRange(index, 0);
}

void MainWindow::on_chBRange_activated(const int index) {
	d.setRange(index, 1);
}

void MainWindow::on_sampleNumber_valueChanged(const int no_of_samples) {
	d.setNumberSamples(no_of_samples);
}

void MainWindow::on_scanAmplitude_valueChanged(const double value) {
	// value is in [V], has to me set in [mV]
	d.setScanParameters(0, static_cast<int>(1e6*value));
}

void MainWindow::on_scanOffset_valueChanged(const double value) {
	// value is in [V], has to me set in [mV]
	d.setScanParameters(1, static_cast<int>(1e6*value));
}

void MainWindow::on_scanWaveform_activated(const int index) {
	d.setScanParameters(2, index);
}

void MainWindow::on_scanFrequency_valueChanged(const double value) {
	d.setScanParameters(3, value);
}

void MainWindow::on_scanSteps_valueChanged(const int value) {
	d.setScanParameters(4, value);
}

void MainWindow::updateLiveView(std::array<QVector<QPointF>, PS2000_MAX_CHANNELS> data) {
	if (view == 0) {
		int channel = 0;
		foreach(QtCharts::QLineSeries* series, liveViewPlots) {
			if (series->isVisible()) {
				series->replace(data[channel]);
			}
			++channel;
		}
		liveViewChart->axisX()->setRange(0, data[0].length());
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
			intensity.append(QPointF(scanResults.voltages[j] / static_cast<double>(1e6), scanResults.intensity[j] / static_cast<double>(1000)));
			A1.append(QPointF(scanResults.voltages[j] / static_cast<double>(1e6), std::real(scanResults.amplitudes.A1[j]) / static_cast<double>(1000)));
			A2.append(QPointF(scanResults.voltages[j] / static_cast<double>(1e6), std::real(scanResults.amplitudes.A2[j]) / static_cast<double>(1000)));
			quotients.append(QPointF(scanResults.voltages[j] / static_cast<double>(1e6), scanResults.quotients[j] / static_cast<double>(20)));
		}
		scanViewPlots[static_cast<int>(scanViewPlotTypes::INTENSITY)]->replace(intensity);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::A1)]->replace(A1);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::A2)]->replace(A2);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::QUOTIENTS)]->replace(quotients);

		scanViewChart->axisX()->setRange(0, 2);
		scanViewChart->axisY()->setRange(-0.4, 1.2);
	}
}

void MainWindow::updateAcquisitionParameters(ACQUISITION_PARAMETERS acquisitionParameters) {
	// set sample rate
	ui->sampleRate->setCurrentIndex(acquisitionParameters.timebase);
	// set number of samples
	ui->sampleNumber->setValue(acquisitionParameters.no_of_samples);
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
			//MainWindow::updateLiveView();
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
			MainWindow::updateScanView();
			break;
	}
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