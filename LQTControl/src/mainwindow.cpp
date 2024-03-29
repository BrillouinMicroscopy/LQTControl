#include "mainwindow.h"
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
#include "version.h"

MainWindow::MainWindow(QWidget *parent) noexcept :
	QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);

	// Set window title
	auto title = QString{ "LQTControl v%1.%2.%3" }.arg(Version::MAJOR).arg(Version::MINOR).arg(Version::PATCH);
	if (Version::PRERELEASE.length() > 0) {
		title += "-" + QString::fromStdString(Version::PRERELEASE);
	}
#ifdef _DEBUG
	title += QString{ " - Debug" };
#endif
	this->setWindowTitle(title);

	qRegisterMetaType<ACQUISITION_PARAMETERS>("ACQUISITION_PARAMETERS");
	qRegisterMetaType<LOCKSTATE>("LOCKSTATE");
	qRegisterMetaType<LQT_SETTINGS>("LQT_SETTINGS");

	// slot laser connection
	static QMetaObject::Connection connection = QWidget::connect(
		m_laserControl,
		&LQT::connected,
		this,
		&MainWindow::laserConnectionChanged
	);

	connection = QWidget::connect(
		m_laserControl,
		&LQT::settingsChanged,
		this,
		&MainWindow::updateLaserSettings
	);

	// slot daq acquisition running
	connection = QWidget::connect(
		m_lockingControl,
		&Locking::s_acquireLockingRunning,
		this,
		&MainWindow::showAcquireLockingRunning
	);

	// slot daq acquisition running
	connection = QWidget::connect(
		m_lockingControl,
		&Locking::s_scanRunning,
		this,
		&MainWindow::showScanRunning
	);

	connection = QWidget::connect(
		m_lockingControl,
		&Locking::s_scanPassAcquired,
		this, 
		&MainWindow::updateScanView
	);

	connection = QWidget::connect(
		m_lockingControl,
		&Locking::locked,
		this,
		&MainWindow::updateLockView
	);

	connection = QWidget::connect(
		m_lockingControl,
		&Locking::lockStateChanged,
		this,
		&MainWindow::updateLockState
	);
	
	// set up live view plots
	liveViewPlots.resize(static_cast<int>(liveViewPlotTypes::COUNT));

	QtCharts::QLineSeries *channelA = new QtCharts::QLineSeries();
	channelA->setUseOpenGL(true);
	channelA->setColor(colors.blue);
	channelA->setName(QString("Detector signal"));
	liveViewPlots[static_cast<int>(liveViewPlotTypes::CHANNEL_A)] = channelA;

	QtCharts::QLineSeries *channelB = new QtCharts::QLineSeries();
	channelB->setUseOpenGL(true);
	channelB->setColor(colors.orange);
	channelB->setName(QString("Reference Signal"));
	liveViewPlots[static_cast<int>(liveViewPlotTypes::CHANNEL_B)] = channelB;
	
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

	// set up lock view plots
	lockViewPlots.resize(static_cast<int>(lockViewPlotTypes::COUNT));

	QtCharts::QLineSeries *absorption = new QtCharts::QLineSeries();
	absorption->setUseOpenGL(true);
	absorption->setColor(colors.orange);
	absorption->setName(QString("Absorption signal"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::ABSORPTION)] = absorption;

	QtCharts::QLineSeries *errorLock = new QtCharts::QLineSeries();
	errorLock->setUseOpenGL(true);
	errorLock->setColor(colors.yellow);
	errorLock->setName(QString("Error signal"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::ERRORSIGNAL)] = errorLock;

	QtCharts::QLineSeries *reference = new QtCharts::QLineSeries();
	reference->setUseOpenGL(true);
	reference->setColor(colors.blue);
	reference->setName(QString("Reference signal"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::REFERENCE)] = reference;

	QtCharts::QLineSeries *lockTemperature = new QtCharts::QLineSeries();
	lockTemperature->setUseOpenGL(true);
	lockTemperature->setColor(colors.orange);
	lockTemperature->setName(QString("Offset temperature"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::TEMPERATUREOFFSET)] = lockTemperature;

	QtCharts::QLineSeries *transmission = new QtCharts::QLineSeries();
	transmission->setUseOpenGL(true);
	transmission->setColor(colors.purple);
	transmission->setName(QString("Transmission"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::TRANSMISSION)] = transmission;

	QtCharts::QLineSeries *errorMean = new QtCharts::QLineSeries();
	errorMean->setUseOpenGL(true);
	errorMean->setColor(colors.green);
	errorMean->setName(QString("Error signal mean"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::ERRORSIGNALMEAN)] = errorMean;

	QtCharts::QLineSeries *errorStd = new QtCharts::QLineSeries();
	errorStd->setUseOpenGL(true);
	errorStd->setColor(colors.red);
	errorStd->setName(QString("Error signal standard deviation"));
	lockViewPlots[static_cast<int>(lockViewPlotTypes::ERRORSIGNALSTD)] = errorStd;

	// set up lock view chart
	lockViewChart = new QtCharts::QChart();
	foreach(QtCharts::QLineSeries* series, lockViewPlots) {
		lockViewChart->addSeries(series);
	}
	lockViewChart->createDefaultAxes();
	lockViewChart->axisX()->setRange(0, 1024);
	lockViewChart->axisY()->setRange(-1, 4);
	lockViewChart->axisX()->setTitleText("Time [s]");
	lockViewChart->setTitle("Lock View");
	lockViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set up scan view plots
	scanViewPlots.resize(static_cast<int>(scanViewPlotTypes::COUNT));

	QtCharts::QLineSeries *scanAbsorption = new QtCharts::QLineSeries();
	scanAbsorption->setUseOpenGL(true);
	scanAbsorption->setColor(colors.orange);
	scanAbsorption->setName(QString("Absorption"));
	scanViewPlots[static_cast<int>(scanViewPlotTypes::ABSORPTION)] = scanAbsorption;

	QtCharts::QLineSeries *scanReference = new QtCharts::QLineSeries();
	scanReference->setUseOpenGL(true);
	scanReference->setColor(colors.yellow);
	scanReference->setName(QString("Reference"));
	scanViewPlots[static_cast<int>(scanViewPlotTypes::REFERENCE)] = scanReference;

	QtCharts::QLineSeries *quotient = new QtCharts::QLineSeries();
	quotient->setUseOpenGL(true);
	quotient->setColor(colors.blue);
	quotient->setName(QString("Quotient"));
	scanViewPlots[static_cast<int>(scanViewPlotTypes::QUOTIENT)] = quotient;

	QtCharts::QLineSeries *scanTransmission = new QtCharts::QLineSeries();
	scanTransmission->setUseOpenGL(true);
	scanTransmission->setColor(colors.purple);
	scanTransmission->setName(QString("Transmission"));
	scanViewPlots[static_cast<int>(scanViewPlotTypes::TRANSMISSION)] = scanTransmission;

	// set up live view chart
	scanViewChart = new QtCharts::QChart();
	foreach(QtCharts::QLineSeries* series, scanViewPlots) {
		scanViewChart->addSeries(series);
	}
	scanViewChart->createDefaultAxes();
	scanViewChart->axisX()->setRange(0, 1024);
	scanViewChart->axisY()->setRange(-1, 4);
	scanViewChart->axisX()->setTitleText("Temperature Offset [K]");
	scanViewChart->setTitle("Scan View");
	scanViewChart->layout()->setContentsMargins(0, 0, 0, 0);

	// set live view chart as default chart
	ui->plotAxes->setChart(liveViewChart);
	ui->plotAxes->setRenderHint(QPainter::Antialiasing);
	ui->plotAxes->setRubberBand(QtCharts::QChartView::RectangleRubberBand);

	// set default values of GUI elements
	SCAN_SETTINGS scanSettings = m_lockingControl->getScanSettings();
	ui->scanStart->setValue(scanSettings.low);
	ui->scanEnd->setValue(scanSettings.high);
	ui->scanSteps->setValue(scanSettings.nrSteps);

	// set default values of lockin parameters
	LOCK_SETTINGS lockSettings = m_lockingControl->getLockSettings();
	ui->proportionalTerm->setValue(lockSettings.proportional);
	ui->integralTerm->setValue(lockSettings.integral);
	ui->derivativeTerm->setValue(lockSettings.derivative);


	//ui->temperatureOffset->setValue(lockParameters.frequency);

	// connect legend marker to toggle visibility of plots
	MainWindow::connectMarkers();

	/* Add labels and indicator to status bar */
	// Status info
	statusInfo = new QLabel("");
	statusInfo->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	ui->statusBar->addPermanentWidget(statusInfo, 1);

	// Locking info
	lockInfo = new QLabel("Locking Inactive");
	lockInfo->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	lockInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui->statusBar->addPermanentWidget(lockInfo, 0);

	// Locking indicator
	lockIndicator = new IndicatorWidget();
	lockIndicator->turnOn();
	lockIndicator->setMinimumSize(20, 18);
	ui->statusBar->addPermanentWidget(lockIndicator, 0);

	// style status bar to not show items borders
	ui->statusBar->setStyleSheet(QString("QLabel { margin-left: 8px; } QStatusBar::item { border: none; }"));

	// hide floating view elements by default
	ui->floatingViewLabel->hide();
	ui->floatingViewCheckBox->hide();

	initDAQ();
	initSettingsDialog();

	// start acquisition thread
	m_acquisitionThread.startWorker(m_lockingControl);
	m_acquisitionThread.startWorker(m_laserControl);

	QMetaObject::invokeMethod(m_laserControl, &LQT::connect, Qt::AutoConnection);
}

MainWindow::~MainWindow() {
	if (m_dataAcquisition) {
		m_dataAcquisition->deleteLater();
		m_dataAcquisition = nullptr;
	}
	m_acquisitionThread.exit();
	m_acquisitionThread.wait();
	delete ui;
}

void MainWindow::on_actionQuit_triggered() {
	QApplication::quit();
}

void MainWindow::initDAQ() {
	// deinitialize DAQ if necessary
	if (m_dataAcquisition) {
		m_dataAcquisition->deleteLater();
		m_dataAcquisition = nullptr;
	}

	// initialize correct DAQ type
	switch (m_daqType) {
	case PS_TYPES::MODEL_PS2000:
		m_dataAcquisition = new daq_PS2000(nullptr);
		break;
	case PS_TYPES::MODEL_PS2000A:
		m_dataAcquisition = new daq_PS2000A(nullptr);
		break;
	default:
		m_dataAcquisition = new daq_PS2000(nullptr);
		break;
	}

	m_acquisitionThread.startWorker(m_dataAcquisition);

	// reestablish m_dataAcquisition connections and store them for
	// possible disconnection
	QMetaObject::Connection connection = QWidget::connect(
		m_dataAcquisition,
		&daq::connected,
		this,
		&MainWindow::daqConnectionChanged
	);

	connection = QWidget::connect(
		m_dataAcquisition,
		&daq::s_acquisitionRunning,
		this,
		&MainWindow::showAcqRunning
	);

	connection = QWidget::connect(
		m_dataAcquisition,
		&daq::collectedBlockData,
		this,
		&MainWindow::updateLiveView
	);

	connection = QWidget::connect(
		m_dataAcquisition,
		&daq::acquisitionParametersChanged,
		this,
		&MainWindow::updateAcquisitionParameters
	);

	updateSamplingRates();

	QMetaObject::invokeMethod(m_dataAcquisition, &daq::connect, Qt::AutoConnection);
}

void MainWindow::updateSamplingRates() {
	std::vector<double> samplingRates = m_dataAcquisition->getSamplingRates();
	ui->sampleRate->clear();
	std::for_each(samplingRates.begin(), samplingRates.end(),
		[this](double samplingRate) {
			std::string string = getSamplingRateString(samplingRate);
			ui->sampleRate->addItem(QString::fromStdString(string));
		}
	);
	ACQUISITION_PARAMETERS acquisitionParameters = m_dataAcquisition->getAcquisitionParameters();
	ui->sampleRate->setCurrentIndex(acquisitionParameters.timebaseIndex);
}

std::string MainWindow::getSamplingRateString(double samplingRate) {
	if (samplingRate > 1e6) {
		return std::to_string((int)(samplingRate / 1e6)) + " MS/s";
	} else if (samplingRate > 1e3) {
		return std::to_string((int)(samplingRate / 1e3)) + " kS/s";
	} else {
		return std::to_string((int)(samplingRate)) + " S/s";
	}
}

void MainWindow::on_actionAbout_triggered() {
	QString clean = "Yes";
	if (Version::VerDirty) {
		clean = "No";
	}
	auto preRelease = QString{ "" };
	if (Version::PRERELEASE.length() > 0) {
		preRelease = QString::fromStdString("-" + Version::PRERELEASE);
	}

	auto debugString = QString{ "" };
#ifdef _DEBUG
	debugString = QString{ " - Debug" };
#endif
	QString str = QString("LQTControl v%1.%2.%3%4%11 <br> Build from commit: <a href='%5'>%6</a><br>Clean build: %7<br>Author: <a href='mailto:%8?subject=LQTControl'>%9</a><br>Date: %10")
		.arg(Version::MAJOR)
		.arg(Version::MINOR)
		.arg(Version::PATCH)
		.arg(preRelease)
		.arg(Version::Url.c_str())
		.arg(Version::Commit.c_str())
		.arg(clean)
		.arg(Version::AuthorEmail.c_str())
		.arg(Version::Author.c_str())
		.arg(Version::Date.c_str())
		.arg(debugString);

	QMessageBox::about(this, tr("About LQTControl"), str);
}

void MainWindow::on_actionSettings_triggered() {
	m_daqDropdown->setCurrentIndex((int)m_daqType);
	settingsDialog->show();
}

void MainWindow::saveSettings() {
	m_daqType = m_daqTypeTemporary;
	settingsDialog->hide();
	initDAQ();
}

void MainWindow::cancelSettings() {
	m_daqTypeTemporary = m_daqType;
	settingsDialog->hide();
}

void MainWindow::initSettingsDialog() {
	m_daqTypeTemporary = m_daqType;
	settingsDialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	settingsDialog->setWindowTitle("Settings");
	settingsDialog->setWindowModality(Qt::ApplicationModal);

	QVBoxLayout *vLayout = new QVBoxLayout(settingsDialog);

	QWidget *daqWidget = new QWidget();
	daqWidget->setMinimumHeight(100);
	daqWidget->setMinimumWidth(400);
	QGroupBox *box = new QGroupBox(daqWidget);
	box->setTitle("Scanning device");
	box->setMinimumHeight(100);
	box->setMinimumWidth(400);

	vLayout->addWidget(daqWidget);

	QHBoxLayout *layout = new QHBoxLayout(box);

	QLabel *label = new QLabel("Currently selected device");
	layout->addWidget(label);

	m_daqDropdown = new QComboBox();
	layout->addWidget(m_daqDropdown);
	gsl::index i{ 0 };
	for (auto type : m_dataAcquisition->PS_NAMES) {
		m_daqDropdown->insertItem(i, QString::fromStdString(type));
		i++;
	}
	m_daqDropdown->setCurrentIndex((int)m_daqType);

	static QMetaObject::Connection connection = QWidget::connect<void(QComboBox::*)(int)>(
		m_daqDropdown,
		&QComboBox::currentIndexChanged,
		this,
		&MainWindow::selectDAQ
	);

	QWidget *buttonWidget = new QWidget();
	vLayout->addWidget(buttonWidget);

	QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
	buttonLayout->setMargin(0);

	QPushButton *okButton = new QPushButton();
	okButton->setText(tr("OK"));
	okButton->setMaximumWidth(100);
	buttonLayout->addWidget(okButton);
	buttonLayout->setAlignment(okButton, Qt::AlignRight);

	connection = QWidget::connect(
		okButton,
		&QPushButton::clicked,
		this,
		&MainWindow::saveSettings
	);

	QPushButton *cancelButton = new QPushButton();
	cancelButton->setText(tr("Cancel"));
	cancelButton->setMaximumWidth(100);
	buttonLayout->addWidget(cancelButton);

	connection = QWidget::connect(
		cancelButton,
		&QPushButton::clicked,
		this,
		&MainWindow::cancelSettings
	);

	settingsDialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void MainWindow::selectDAQ(int index) {
	m_daqTypeTemporary = (PS_TYPES)index;
}

void MainWindow::connectMarkers() {
	// Connect all markers to handler
	QMetaObject::Connection connection;
	foreach(QtCharts::QLegendMarker * marker, liveViewChart->legend()->markers()) {
		// Disconnect possible existing connection to avoid multiple connections
		QWidget::disconnect(
			marker,
			&QtCharts::QLegendMarker::clicked,
			this,
			&MainWindow::handleMarkerClicked
		);
		connection = QWidget::connect(
			marker,
			&QtCharts::QLegendMarker::clicked,
			this,
			&MainWindow::handleMarkerClicked
		);
	}
	foreach(QtCharts::QLegendMarker * marker, lockViewChart->legend()->markers()) {
		// Disconnect possible existing connection to avoid multiple connections
		QWidget::disconnect(
			marker,
			&QtCharts::QLegendMarker::clicked,
			this,
			&MainWindow::handleMarkerClicked
		);
		connection = QWidget::connect(
			marker,
			&QtCharts::QLegendMarker::clicked,
			this,
			&MainWindow::handleMarkerClicked
		);
	}
	foreach(QtCharts::QLegendMarker * marker, scanViewChart->legend()->markers()) {
		// Disconnect possible existing connection to avoid multiple connections
		QWidget::disconnect(
			marker,
			&QtCharts::QLegendMarker::clicked,
			this,
			&MainWindow::handleMarkerClicked
		);
		connection = QWidget::connect(
			marker,
			&QtCharts::QLegendMarker::clicked,
			this,
			&MainWindow::handleMarkerClicked
		);
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
	QMetaObject::invokeMethod(m_dataAcquisition, &daq::startStopAcquisition, Qt::QueuedConnection);
}

void MainWindow::showAcqRunning(bool running) {
	if (running) {
		ui->acquisitionButton->setText(QString("Stop"));
	} else {
		ui->acquisitionButton->setText(QString("Acquire"));
	}
}

void MainWindow::showScanRunning(bool running) {
	if (running) {
		ui->scanButton->setText(QString("Stop"));
	} else {
		ui->scanButton->setText(QString("Scan"));
	}
}

void MainWindow::on_acquireLockButton_clicked() {
	QMetaObject::invokeMethod(m_lockingControl, &Locking::startStopAcquireLocking, Qt::AutoConnection);
}

void MainWindow::showAcquireLockingRunning(bool running) {
	if (running) {
		ui->acquireLockButton->setText(QString("Stop"));
	} else {
		ui->lockButton->setText(QString("Lock"));
		ui->acquireLockButton->setText(QString("Acquire"));
	}
}

void MainWindow::on_lockButton_clicked() {
	QMetaObject::invokeMethod(m_lockingControl, &Locking::startStopLocking, Qt::AutoConnection);
}

void MainWindow::updateLockState(LOCKSTATE lockState) {
	switch (lockState) {
		case LOCKSTATE::ACTIVE:
			lockInfo->setText("Locking active");
			ui->lockButton->setText(QString("Unlock"));
			lockIndicator->active();
			break;
		case LOCKSTATE::INACTIVE:
			lockInfo->setText("Locking inactive");
			ui->lockButton->setText(QString("Lock"));
			lockIndicator->inactive();
			break;
		case LOCKSTATE::FAILURE:
			lockInfo->setText("Locking failure");
			ui->lockButton->setText(QString("Lock"));
			lockIndicator->failure();
			break;
	}
}

void MainWindow::on_sampleRate_activated(const int index) {
	m_dataAcquisition->setSampleRate(index);
}

void MainWindow::on_chACoupling_activated(const int index) {
	m_dataAcquisition->setCoupling(index, 0);
}

void MainWindow::on_chBCoupling_activated(const int index) {
	m_dataAcquisition->setCoupling(index, 1);
}

void MainWindow::on_chARange_activated(const int index) {
	m_dataAcquisition->setRange(index, 0);
}

void MainWindow::on_chBRange_activated(const int index) {
	m_dataAcquisition->setRange(index, 1);
}

void MainWindow::on_sampleNumber_valueChanged(const int no_of_samples) {
	m_dataAcquisition->setNumberSamples(no_of_samples);
}

void MainWindow::on_scanStart_valueChanged(const double value) {
	m_lockingControl->setScanParameters(SCANPARAMETERS::LOW, value);
}

void MainWindow::on_scanEnd_valueChanged(const double value) {
	m_lockingControl->setScanParameters(SCANPARAMETERS::HIGH, value);
}

void MainWindow::on_scanSteps_valueChanged(const int value) {
	m_lockingControl->setScanParameters(SCANPARAMETERS::STEPS, value);
}

void MainWindow::on_scanInterval_valueChanged(const int value) {
	m_lockingControl->setScanParameters(SCANPARAMETERS::INTERVAL, value);
}

/***********************************
 * Set parameters for cavity locking
 ***********************************/
void MainWindow::on_proportionalTerm_valueChanged(const double value) {
	m_lockingControl->setLockParameters(LOCKPARAMETERS::P, value);
}
void MainWindow::on_integralTerm_valueChanged(const double value) {
	m_lockingControl->setLockParameters(LOCKPARAMETERS::I, value);
}
void MainWindow::on_derivativeTerm_valueChanged(const double value) {
	m_lockingControl->setLockParameters(LOCKPARAMETERS::D, value);
}
void MainWindow::on_transmission_valueChanged(const double value) {
	m_lockingControl->setLockParameters(LOCKPARAMETERS::SETPOINT, value);
}

void MainWindow::on_temperatureOffset_valueChanged(const double offset) {
	QMetaObject::invokeMethod(m_laserControl, [&m_laserControl = m_laserControl, offset]() { m_laserControl->setTemperatureForce(offset); }, Qt::AutoConnection);
}

void MainWindow::updateLiveView() {
	if (m_selectedView == VIEWS::LIVE) {

		m_dataAcquisition->m_liveBuffer->m_usedBuffers->acquire();
		int16_t **buffer = m_dataAcquisition->m_liveBuffer->getReadBuffer();

		std::array<QVector<QPointF>, PS2000_MAX_CHANNELS> data;
		for (gsl::index channel{ 0 }; channel < 4; channel++) {
			data[channel].resize(1000);
			for (gsl::index jj{ 0 }; jj < 1000; jj++) {
				data[channel][jj] = QPointF(jj, buffer[channel][jj] / static_cast<double>(1e3));
			}
		}

		gsl::index channel{ 0 };
		foreach(QtCharts::QLineSeries* series, liveViewPlots) {
			if (series->isVisible()) {
				series->replace(data[channel]);
			}
			++channel;
		}
		liveViewChart->axisX()->setRange(0, data[0].length());

		m_dataAcquisition->m_liveBuffer->m_freeBuffers->release();
	}
}

void MainWindow::updateScanView() {
	if (m_selectedView == VIEWS::SCAN) {
		SCAN_DATA scanData = m_lockingControl->scanData;
		SCAN_SETTINGS scanSettings = m_lockingControl->getScanSettings();

		QVector<QPointF> absorption;
		QVector<QPointF> reference;
		QVector<QPointF> quotient;
		QVector<QPointF> transmission;

		absorption.reserve(scanData.nrSteps);
		reference.reserve(scanData.nrSteps);
		quotient.reserve(scanData.nrSteps);
		transmission.reserve(scanData.nrSteps);

		for (gsl::index j{ 0 }; j < scanData.nrSteps; j++) {
			absorption.append(QPointF(scanData.temperatures[j], scanData.absorption[j]));
			reference.append(QPointF(scanData.temperatures[j], scanData.reference[j]));
			quotient.append(QPointF(scanData.temperatures[j], scanData.quotient[j]));
			transmission.append(QPointF(scanData.temperatures[j], scanData.transmission[j]));
		}
		scanViewPlots[static_cast<int>(scanViewPlotTypes::ABSORPTION)]->replace(absorption);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::REFERENCE)]->replace(reference);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::QUOTIENT)]->replace(quotient);
		scanViewPlots[static_cast<int>(scanViewPlotTypes::TRANSMISSION)]->replace(transmission);

		scanViewChart->axisX()->setRange(scanSettings.low, scanSettings.high);
		scanViewChart->axisY()->setRange(-0.2, 2);
	}
}

void MainWindow::updateLockView() {
	if (m_selectedView == VIEWS::LOCK) {

		auto prevIndex = generalmath::indexWrapped((int)m_lockingControl->lockData.nextIndex - 1, m_lockingControl->lockData.storageSize);
		auto passed = std::chrono::duration_cast<std::chrono::milliseconds>(
			m_lockingControl->lockData.time[prevIndex] - m_lockingControl->lockData.startTime
			).count() / 1e3;


		lockViewPlots[static_cast<int>(lockViewPlotTypes::ABSORPTION)]->append(
			QPointF(passed, m_lockingControl->lockData.absorption[prevIndex])
		);
		lockViewPlots[static_cast<int>(lockViewPlotTypes::REFERENCE)]->append(
			QPointF(passed, m_lockingControl->lockData.reference[prevIndex])
		);

		// Replace the transmission array in the correct order
		auto transmission = QList<QPointF>{};
		auto size{ 0 };
		if (m_lockingControl->lockData.wrapped) {
			size = m_lockingControl->lockData.storageSize;
		} else {
			size = m_lockingControl->lockData.nextIndex;
		}
		transmission.reserve(size);
		if (m_lockingControl->lockData.wrapped) {
			for (gsl::index i{ m_lockingControl->lockData.nextIndex }; i < m_lockingControl->lockData.storageSize; i++) {
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
					m_lockingControl->lockData.time[i] - m_lockingControl->lockData.startTime
				).count() / 1e3;
				transmission.append(QPointF(time, m_lockingControl->lockData.transmission[i]));
			}
		}
		for (gsl::index i{ 0 }; i < m_lockingControl->lockData.nextIndex; i++) {
			auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
				m_lockingControl->lockData.time[i] - m_lockingControl->lockData.startTime
			).count() / 1e3;
			transmission.append(QPointF(time, m_lockingControl->lockData.transmission[i]));
		}
		lockViewPlots[static_cast<int>(lockViewPlotTypes::TRANSMISSION)]->replace(transmission);


		lockViewPlots[static_cast<int>(lockViewPlotTypes::ERRORSIGNAL)]->append(
			QPointF(passed, m_lockingControl->lockData.error[prevIndex])
		);
		lockViewPlots[static_cast<int>(lockViewPlotTypes::TEMPERATUREOFFSET)]->append(
			QPointF(passed, m_lockingControl->lockData.tempOffset[prevIndex])
		);

		auto offset = m_lockingControl->lockData.storageSize - m_lockingControl->lockData.nextIndex;
		lockViewPlots[static_cast<int>(lockViewPlotTypes::ERRORSIGNALMEAN)]->append(
			QPointF(passed, generalmath::floatingMean(m_lockingControl->lockData.error, 50, offset))
		);
		lockViewPlots[static_cast<int>(lockViewPlotTypes::ERRORSIGNALSTD)]->append(
			QPointF(passed, generalmath::floatingStandardDeviation(m_lockingControl->lockData.error, 50, offset))
		);

		// If there are more points than desired, remove the first one
		foreach(QtCharts::QLineSeries *series, lockViewPlots) {
			if (series->count() >= m_lockingControl->lockData.storageSize) {
				series->remove(0);
			}
		}

		auto minX = lockViewPlots[0]->at(0).x();
		auto maxX = lockViewPlots[0]->at(lockViewPlots[0]->count() - 1).x();
		// Only show last 60 seconds in floating view
		if (viewSettings.floatingView) {
			auto tmp = maxX - 60;
			if (tmp > minX) {
				minX = tmp;
			}
		}
		lockViewChart->axisX()->setRange(minX, maxX);
	}
}

void MainWindow::updateAcquisitionParameters(ACQUISITION_PARAMETERS acquisitionParameters) {
	// set sample rate
	ui->sampleRate->setCurrentIndex(acquisitionParameters.timebaseIndex);
	// set number of samples
	ui->sampleNumber->setValue(acquisitionParameters.no_of_samples);
	// set range
	ui->chARange->setCurrentIndex(acquisitionParameters.channelSettings[0].range - 2);
	ui->chBRange->setCurrentIndex(acquisitionParameters.channelSettings[1].range - 2);
	// set coupling
	ui->chACoupling->setCurrentIndex(acquisitionParameters.channelSettings[0].coupling);
	ui->chBCoupling->setCurrentIndex(acquisitionParameters.channelSettings[1].coupling);
}

void MainWindow::updateLaserSettings(LQT_SETTINGS settings) {
	ui->temperatureOffset->setValue(settings.temperature);
	if (!settings.lock && !settings.lockEnabled && !settings.modEnabled) {
		ui->enableTemperatureControlCheckbox->setChecked(true);
	} else {
		ui->enableTemperatureControlCheckbox->setChecked(false);
	}
}

void MainWindow::on_scanButton_clicked() {
	if (!m_lockingControl->scanData.m_running) {
		QMetaObject::invokeMethod(m_lockingControl, &Locking::startScan, Qt::AutoConnection);
	} else {
		m_lockingControl->scanData.m_abort = true;
	}
}

void MainWindow::on_selectDisplay_activated(const int index) {
	m_selectedView = static_cast<VIEWS>(index);
	switch (m_selectedView) {
		case VIEWS::LIVE:
			// it is necessary to hide the series, because they do not get removed
			// after setChart in case useOpenGL == true (bug in QT?)
			foreach(QtCharts::QLineSeries* series, scanViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, lockViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, liveViewPlots) {
				series->setVisible(true);
			}
			ui->plotAxes->setChart(liveViewChart);
			ui->floatingViewLabel->hide();
			ui->floatingViewCheckBox->hide();
			//MainWindow::updateLiveView();
			break;
		case VIEWS::LOCK:
			// it is necessary to hide the series, because they do not get removed
			// after setChart in case useOpenGL == true (bug in QT?)
			foreach(QtCharts::QLineSeries* series, liveViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, scanViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, lockViewPlots) {
				series->setVisible(true);
			}
			ui->plotAxes->setChart(lockViewChart);
			ui->floatingViewLabel->show();
			ui->floatingViewCheckBox->show();
			//MainWindow::updateScanView();
			break;
		case VIEWS::SCAN:
			// it is necessary to hide the series, because they do not get removed
			// after setChart in case useOpenGL == true (bug in QT?)
			foreach(QtCharts::QLineSeries* series, liveViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, lockViewPlots) {
				series->setVisible(false);
			}
			foreach(QtCharts::QLineSeries* series, scanViewPlots) {
				series->setVisible(true);
			}
			ui->plotAxes->setChart(scanViewChart);
			ui->floatingViewLabel->hide();
			ui->floatingViewCheckBox->hide();
			MainWindow::updateScanView();
			break;
	}
}

void MainWindow::on_floatingViewCheckBox_clicked(const bool checked) {
	viewSettings.floatingView = checked;
}

void MainWindow::on_actionConnect_DAQ_triggered() {
	QMetaObject::invokeMethod(m_dataAcquisition, &daq::connect, Qt::AutoConnection);
}

void MainWindow::on_actionDisconnect_DAQ_triggered() {
	QMetaObject::invokeMethod(m_dataAcquisition, &daq::disconnect, Qt::AutoConnection);
}

void MainWindow::daqConnectionChanged(bool connected) {
	m_isDAQConnected = connected;
	if (connected) {
		statusInfo->setText("Successfully connected to data acquisition card.");
	} else {
		statusInfo->setText("Successfully disconnected from data acquisition card.");
	}
	ui->actionConnect_DAQ->setEnabled(!connected);
	ui->actionDisconnect_DAQ->setEnabled(connected);
	ui->AcquisitionBox->setEnabled(connected);
	if (m_isDAQConnected && m_isLaserConnected) {
		ui->ScanBox->setEnabled(connected);
		ui->LockingBox->setEnabled(connected);
	}
}

void MainWindow::on_actionConnect_Laser_triggered() {
	QMetaObject::invokeMethod(m_laserControl, &LQT::connect, Qt::AutoConnection);
}

void MainWindow::on_actionDisconnect_Laser_triggered() {
	QMetaObject::invokeMethod(m_laserControl, &LQT::disconnect, Qt::AutoConnection);
}

void MainWindow::laserConnectionChanged(bool connected) {
	m_isLaserConnected = connected;
	if (connected) {
		statusInfo->setText("Successfully connected to Laser.");
	} else {
		statusInfo->setText("Successfully disconnected from Laser.");
	}
	ui->actionConnect_Laser->setEnabled(!connected);
	ui->actionDisconnect_Laser->setEnabled(connected);
	ui->LaserTemperatureBox->setEnabled(connected);
	ui->enableTemperatureControlCheckbox->setEnabled(connected);
	if (m_isDAQConnected && m_isLaserConnected) {
		ui->ScanBox->setEnabled(connected);
		ui->LockingBox->setEnabled(connected);
	}
}

void MainWindow::on_enableTemperatureControlCheckbox_clicked(const bool checked) {
	QMetaObject::invokeMethod(m_laserControl, [&m_laserControl = m_laserControl, checked]() { m_laserControl->enableTemperatureControl(checked); }, Qt::AutoConnection);
}