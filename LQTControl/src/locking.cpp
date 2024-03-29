#include "locking.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

Locking::Locking(QObject *parent, daq **dataAcquisition, LQT *laserControl) :
	QObject(parent), m_dataAcquisition(dataAcquisition), m_laserControl(laserControl) {

	// Calculate the maximum storage size and resize the arrays accordingly
	lockData.storageSize = (int)((1000 * lockData.storageDuration) / lockSettings.lockingTimeout);
	lockData.time.resize(lockData.storageSize);
	lockData.absorption.resize(lockData.storageSize);
	lockData.reference.resize(lockData.storageSize);
	lockData.quotient.resize(lockData.storageSize);
	lockData.tempOffset.resize(lockData.storageSize);
	lockData.error.resize(lockData.storageSize);
	lockData.startTime = std::chrono::system_clock::now();
}

void Locking::startStopAcquireLocking() {
	if (lockingTimer->isActive()) {
		setLockState(LOCKSTATE::INACTIVE);
		m_isAcquireLockingRunning = false;
		lockingTimer->stop();
	} else {
		m_isAcquireLockingRunning = true;
		lockingTimer->start(lockSettings.lockingTimeout);
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

	lockData.quotient[lockData.nextIndex] = quotient_mean;

	double quotient_max = generalmath::max(lockData.quotient);
	if (quotient_max > lockData.quotient_max) {
		lockData.quotient_max = quotient_max;
	}

	lockData.transmission = lockData.quotient;
	std::for_each(lockData.transmission.begin(), lockData.transmission.end(), [this](double &el) {el /= lockData.quotient_max; });

	double error = lockData.transmission[lockData.nextIndex] - lockSettings.transmissionSetpoint;

	// write data to struct for storage
	double actualTempOffset;
	if (lockSettings.state == LOCKSTATE::ACTIVE) {
		double dError = 0;
		if (lockData.error.size() > 0) {
			auto prevIndex = generalmath::indexWrapped((int)lockData.nextIndex - 1, lockData.storageSize);
			double dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - lockData.time[prevIndex]).count() / 1e3;
			lockData.iError += lockSettings.integral / 10 * ( lockData.error.back() + error ) * (dt) / 2;
			dError = (error - lockData.error[prevIndex]) / dt;
		}
		lockData.currentTempOffset = lockData.currentTempOffset + (lockSettings.proportional / 10 * error + lockData.iError + lockSettings.derivative * dError);

		// abort locking if current absolute value of the temperature offset is above 5.0
		if (abs(lockData.currentTempOffset) > 5.0) {
			setLockState(LOCKSTATE::FAILURE);
		}

		// set laser temperature
		actualTempOffset = m_laserControl->setTemperatureForce(lockData.currentTempOffset);
	} else {
		actualTempOffset = m_laserControl->getTemperature();
	}

	// write data to struct for storage
	lockData.time[lockData.nextIndex] = now;
	lockData.error[lockData.nextIndex] = error;
	lockData.absorption[lockData.nextIndex] = absorption_mean;
	lockData.reference[lockData.nextIndex] = reference_mean;
	lockData.tempOffset[lockData.nextIndex] = actualTempOffset;
	lockData.nextIndex++;

	// If the next index to write to is outside of the array, we wrap around to the start
	if (lockData.nextIndex >= lockData.storageSize) {
		lockData.nextIndex = 0;
		lockData.wrapped = true;
	}

	emit locked();
}

void Locking::init() {
	// create timers and connect their signals
	// after moving locking to another thread
	lockingTimer = new QTimer();
	scanTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(
		lockingTimer,
		&QTimer::timeout,
		this,
		&Locking::lock
	);
	connection = QWidget::connect(
		scanTimer,
		&QTimer::timeout,
		this,
		&Locking::scan
	);
}