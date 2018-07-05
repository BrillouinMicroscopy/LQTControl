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

	return response;
}

void LQT::send(std::string message) {
	message = message + m_terminator;

	writeToDevice(message.c_str());
	m_laserPort->waitForBytesWritten(1000);
}

qint64 LQT::writeToDevice(const char *data) {
	return m_laserPort->write(data);
}

/*
* Functions for setting and getting the temperature
*/

void LQT::setTemperature(double temperature) {
	send(fmt::format("utempoffset={:06.3f}", temperature));
}

double LQT::getTemperature() {
	std::string temp = receive("utempoffset?");
	return stod(temp);
}

void LQT::setMaxTemperature(double temperature) {
	send(fmt::format("maxutempoffset={:06.3f}", temperature));
}

double LQT::getMaxTemperature() {
	std::string temp = receive("maxutempoffset?");
	return stod(temp);
}

// The laser sometimes does not accept the temperature setting on the first try.
// These functions set the temperature until it has the correct value or it fails for the 10th time.
void LQT::setTemperatureForce(double temperature) {
	int i{ 0 };
	setTemperature(temperature);
	while (getTemperature() != temperature && i++ < 10) {
		Sleep(100);
		setTemperature(temperature);
	}
}

void LQT::setMaxTemperatureForce(double temperature) {
	int i{ 0 };
	setMaxTemperature(temperature);
	while (getMaxTemperature() != temperature && i++ < 10) {
		Sleep(100);
		setMaxTemperature(temperature);
	}
}

/*
 * Functions for setting and getting the lock states
 */

void LQT::setFeature(std::string feature, bool value) {
	if (value) {
		send(feature + "=on");
	}
	else {
		send(feature + "=off");
	}
}

bool LQT::getFeature(std::string feature) {
	std::string lock = receive(feature + "?");
	if (lock == "off") {
		return false;
	}
	else {
		return true;
	}
}

void LQT::enableTemperatureControl(bool enable) {
	setMod(!enable);
	setLock(!enable);
	setLe(!enable);
}

void LQT::setMod(bool mod) {
	setFeature("Mod", mod);
}

bool LQT::getMod() {
	return getFeature("Mod");
}

void LQT::setLock(bool lock) {
	setFeature("Lock", lock);
}

bool LQT::getLock() {
	return getFeature("Lock");
}

void LQT::setLe(bool le) {
	setFeature("Le", le);
}

bool LQT::getLe() {
	return getFeature("Le");
}
