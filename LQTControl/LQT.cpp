#include "LQT.h"
#include <windows.h>

LQT::LQT() {}

LQT::~LQT() {
	disconnect();
}

void LQT::connect() {
	if (!m_isConnected) {
		m_laserPort->setPortName("COM1");
		m_laserPort->setBaudRate(QSerialPort::Baud19200);
		m_laserPort->setStopBits(QSerialPort::OneStop);
		m_laserPort->setParity(QSerialPort::NoParity);
		m_isConnected = m_laserPort->open(QIODevice::ReadWrite);
	}
	emit(connected(m_isConnected));
}

void LQT::disconnect() {
	if (m_isConnected) {
		m_laserPort->close();
	}
	m_isConnected = false;
	emit(connected(m_isConnected));
}

/*
 * Functions regarding the serial communication
 */

std::string LQT::receive(std::string request) {

	m_laserPort->clear();

	request = request + m_terminator;
	writeToDevice(request.c_str());

	std::string response = "";
	if (m_laserPort->waitForBytesWritten(1000)) {
		// read response
		if (m_laserPort->waitForReadyRead(1000)) {
			QByteArray responseData = m_laserPort->readAll();
			while (m_laserPort->waitForReadyRead(50))
				responseData += m_laserPort->readAll();

			response = responseData;
		}
	}

	return stripCRLF(response);
}

void LQT::send(std::string message) {
	message = message + m_terminator;

	writeToDevice(message.c_str());
	m_laserPort->waitForBytesWritten(1000);
}

qint64 LQT::writeToDevice(const char *data) {
	return m_laserPort->write(data);
}

std::string LQT::stripCRLF(std::string msg) {
	if (msg.size() > 0) {
		if (msg.back() == '\n') {
			msg.pop_back();
		}
	}
	if (msg.size() > 0) {
		if (msg.back() == '\r') {
			msg.pop_back();
		}
	}
	return msg;
}

/*
* Functions for setting and getting the temperature
*/

void LQT::setTemperature(double temperature) {
	receive(fmt::format("utempoffset={:06.3f}", temperature));
}

double LQT::getTemperature() {
	std::string temp = receive("utempoffset?");
	if (temp.size() == 0) {
		return NAN;
	} else {
		return stod(temp);
	}
}

void LQT::setMaxTemperature(double temperature) {
	receive(fmt::format("maxutempoffset={:06.3f}", temperature));
}

double LQT::getMaxTemperature() {
	std::string temp = receive("maxutempoffset?");
	if (temp.size() == 0) {
		return NAN;
	} else {
		return stod(temp);
	}
}

// The laser sometimes does not accept the temperature setting on the first try.
// These functions set the temperature until it has the correct value or it fails for the 10th time.
void LQT::setTemperatureForce(double temperature) {
	setTemperature(temperature);
	int i{ 0 };
	while (abs(getTemperature() - temperature) > 0.001 && i++ < 10) {
		setTemperature(temperature);
	}
}

void LQT::setMaxTemperatureForce(double temperature) {
	setMaxTemperature(temperature);
	int i{ 0 };
	while (abs(getMaxTemperature() - temperature) > 0.001  && i++ < 10) {
		setMaxTemperature(temperature);
	}
}

/*
 * Functions for setting and getting the lock states
 */

void LQT::setFeature(std::string feature, bool value) {
	if (value) {
		receive(feature + "=on");
	} else {
		receive(feature + "=off");
	}
}

void LQT::enableTemperatureControl(bool enable) {
	setMod(!enable);
	setLock(!enable);
	setLe(!enable);
}

void LQT::setMod(bool mod) {
	setFeature("mod", mod);
}

bool LQT::getMod() {
	std::string msg = receive("Mod?");
	if (msg == "Laser Modulation OFF") {
		return false;
	} else {
		return true;
	}
}

void LQT::setLock(bool lock) {
	setFeature("Lock", lock);
}

bool LQT::getLock() {
	std::string msg = receive("Lock?");
	if (msg == "LOCK=OFF") {
		return false;
	} else {
		return true;
	}
}

void LQT::setLe(bool le) {
	setFeature("Le", le);
}

bool LQT::getLe() {
	std::string msg = receive("Le?");
	if (msg == "LOCK ENABLE=OFF") {
		return false;
	} else {
		return true;
	}
}
