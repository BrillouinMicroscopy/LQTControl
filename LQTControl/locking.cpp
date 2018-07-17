#include "locking.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

Locking::Locking(QObject *parent, daq **dataAcquisition, LQT *laserControl) :
	QObject(parent), m_dataAcquisition(dataAcquisition), m_laserControl(laserControl) {
}

void Locking::startStopAcquireLocking() {
	if (lockingTimer->isActive()) {
		setLockState(LOCKSTATE::INACTIVE);
		m_isAcquireLockingRunning = false;
		lockingTimer->stop();
	} else {
		m_isAcquireLockingRunning = true;
		lockingTimer->start(100);
	}
	emit(s_acquireLockingRunning(m_isAcquireLockingRunning));
}

void Locking::startStopLocking() {
	if (lockSettings.state != LOCKSTATE::ACTIVE) {
		// set integral error to zero before starting to lock
		lockData.iError = 0;
		// set current temperature offset to actual offset
		lockData.currentTempOffset = m_laserControl->getTemperature();
		setLockState(LOCKSTATE::ACTIVE);
	} else {
		setLockState(LOCKSTATE::INACTIVE);
	}
}

void Locking::setLockState(LOCKSTATE lockstate) {
	lockSettings.state = lockstate;
	emit(lockStateChanged(lockSettings.state));
}

SCAN_SETTINGS Locking::getScanSettings() {
	return scanSettings;
}

void Locking::setScanParameters(SCANPARAMETERS type, double value) {
	switch (type) {
		case SCANPARAMETERS::LOW:
			scanSettings.low = value;
			break;
		case SCANPARAMETERS::HIGH:
			scanSettings.high = value;
			break;
		case SCANPARAMETERS::STEPS:
			scanSettings.nrSteps = value;
			break;
		case SCANPARAMETERS::INTERVAL:
			scanSettings.interval = value;
			break;
	}
}

void Locking::setLockParameters(LOCKPARAMETERS type, double value) {
	switch (type) {
		case LOCKPARAMETERS::P:
			lockSettings.proportional = value;
			break;
		case LOCKPARAMETERS::I:
			lockSettings.integral = value;
			break;
		case LOCKPARAMETERS::D:
			lockSettings.derivative = value;
			break;
		case LOCKPARAMETERS::SETPOINT:
			lockSettings.transmissionSetpoint = value;
			break;
	}
}

void Locking::startScan() {
	if (scanTimer->isActive()) {
		scanData.m_running = false;
		scanTimer->stop();
		emit s_scanRunning(scanData.m_running);
	} else {
		// prepare data arrays
		scanData.nrSteps = scanSettings.nrSteps;
		scanData.temperatures = generalmath::linspace<double>(scanSettings.low, scanSettings.high, scanSettings.nrSteps);

		scanData.reference.resize(scanSettings.nrSteps);
		scanData.absorption.resize(scanSettings.nrSteps);
		scanData.quotient.resize(scanSettings.nrSteps);
		scanData.transmission.resize(scanSettings.nrSteps);
		std::fill(scanData.reference.begin(), scanData.reference.end(), NAN);
		std::fill(scanData.absorption.begin(), scanData.absorption.end(), NAN);
		std::fill(scanData.quotient.begin(), scanData.quotient.end(), NAN);
		std::fill(scanData.transmission.begin(), scanData.transmission.end(), NAN);

		(*m_dataAcquisition)->setAcquisitionParameters();

		scanData.pass = 0;
		scanData.m_running = true;
		scanData.m_abort = false;
		// set laser temperature to start value
		m_laserControl->setTemperatureForce(scanData.temperatures[scanData.pass]);
		passTimer.start();
		scanTimer->start(1000);
		emit s_scanRunning(scanData.m_running);
	}
}

void Locking::scan() {
	// abort scan if wanted
	if (scanData.m_abort) {
		scanData.m_running = false;
		scanTimer->stop();
		emit s_scanRunning(scanData.m_running);
	}

	// wait for one minute for first value to let temperature settle
	if (scanData.pass == 0 && passTimer.elapsed() < 6e1) {
		return;
	}

	// acquire new datapoint when interval has passed
	if (passTimer.elapsed() < (scanSettings.interval*1e3)) {
		return;
	}

	// reset timer when enough time has passed
	passTimer.start();

	// acquire detector and reference signal, store and process it
	std::array<std::vector<int32_t>, DAQ_MAX_CHANNELS> values = (*m_dataAcquisition)->collectBlockData();

	double absorption_mean = generalmath::mean(values[0]) / 1e3;
	double reference_mean = generalmath::mean(values[1]) / 1e3;
	double quotient_mean = abs(absorption_mean / reference_mean);

	scanData.absorption[scanData.pass] = absorption_mean;
	scanData.reference[scanData.pass] = reference_mean;
	scanData.quotient[scanData.pass] = quotient_mean;

	double quotient_max = generalmath::max(scanData.quotient);

	scanData.transmission = scanData.quotient;
	std::for_each(scanData.transmission.begin(), scanData.transmission.end(), [quotient_max](double &el) {el /= quotient_max; });

	++scanData.pass;
	emit s_scanPassAcquired();
	// if scan is not done, set temperature to new value, else annouce finished scan
	if (scanData.pass < scanData.nrSteps) {
		m_laserControl->setTemperatureForce(scanData.temperatures[scanData.pass]);
	} else {
		scanData.m_running = false;
		scanTimer->stop();
		emit s_scanRunning(scanData.m_running);
	}
}

LOCK_SETTINGS Locking::getLockSettings() {
	return lockSettings;
}

void Locking::lock() {
	std::array<std::vector<int32_t>, DAQ_MAX_CHANNELS> values = (*m_dataAcquisition)->collectBlockData();

	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();

	double absorption_mean = generalmath::mean(values[0]) / 1e3;
	double reference_mean = generalmath::mean(values[1]) / 1e3;
	double quotient_mean = abs(absorption_mean / reference_mean);

	lockData.quotient.push_back(quotient_mean);

	double quotient_max = generalmath::max(lockData.quotient);

	lockData.transmission = lockData.quotient;
	std::for_each(lockData.transmission.begin(), lockData.transmission.end(), [quotient_max](double &el) {el /= quotient_max; });

	double error = lockData.transmission.back() - lockSettings.transmissionSetpoint;

	// write data to struct for storage

	if (lockSettings.state == LOCKSTATE::ACTIVE) {
		double dError = 0;
		if (lockData.error.size() > 0) {
			double dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - lockData.time.back()).count() / 1e3;
			lockData.iError += lockSettings.integral / 10 * ( lockData.error.back() + error ) * (dt) / 2;
			dError = (error - lockData.error.back()) / dt;
		}
		lockData.currentTempOffset = lockData.currentTempOffset + (lockSettings.proportional / 10 * error + lockData.iError + lockSettings.derivative * dError);

		// abort locking if current absolute value of the temperature offset is above 5.0
		if (abs(lockData.currentTempOffset) > 5.0) {
			setLockState(LOCKSTATE::FAILURE);
		}

		// set laser temperature
		m_laserControl->setTemperatureForce(lockData.currentTempOffset);
	}

	double actualTempOffset = m_laserControl->getTemperature();

	// write data to struct for storage
	lockData.time.push_back(now);
	lockData.error.push_back(error);
	lockData.absorption.push_back(absorption_mean);
	lockData.reference.push_back(reference_mean);
	lockData.tempOffset.push_back(actualTempOffset);
	
	double passed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lockData.time[0]).count() / 1e3;	// store passed time in seconds
	lockData.relTime.push_back(passed);

	// write data to array for plotting
	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::TRANSMISSION)].clear();
	for (int j{ 0 }; j < lockData.transmission.size(); j++) {
		m_lockDataPlot[static_cast<int>(lockViewPlotTypes::TRANSMISSION)].append(QPointF(lockData.relTime[j], lockData.transmission[j]));
	}

	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::ABSORPTION)].append(QPointF(passed, absorption_mean));
	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::REFERENCE)].append(QPointF(passed, reference_mean));
	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::ERRORSIGNAL)].append(QPointF(passed, error));
	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::ERRORSIGNALMEAN)].append(QPointF(passed, generalmath::floatingMean(lockData.error, 50)));
	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::ERRORSIGNALSTD)].append(QPointF(passed, generalmath::floatingStandardDeviation(lockData.error, 50)));
	m_lockDataPlot[static_cast<int>(lockViewPlotTypes::TEMPERATUREOFFSET)].append(QPointF(passed, actualTempOffset));

	emit locked();
}

void Locking::init() {
	// create timers and connect their signals
	// after moving locking to another thread
	lockingTimer = new QTimer();
	scanTimer = new QTimer();
	QWidget::connect(lockingTimer, SIGNAL(timeout()), this, SLOT(lock()));
	QWidget::connect(scanTimer, SIGNAL(timeout()), this, SLOT(scan()));
}