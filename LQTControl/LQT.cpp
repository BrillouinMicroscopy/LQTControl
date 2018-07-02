#include "LQT.h"

LQT::LQT() noexcept {}

LQT::~LQT() {
	disconnect();
}

void LQT::connect() {
	if (!m_isConnected) {
		m_laserPort->setBaudRate(QSerialPort::Baud9600);
		m_isConnected = m_laserPort->open(QIODevice::ReadWrite);
	}
}

void LQT::disconnect() {
	if (m_isConnected) {
		m_laserPort->close();
	}
	m_isConnected = false;
}

void LQT::enableTemperatureControl() {
}

void LQT::disableTemperatureControl() {
}

void LQT::setTemperature(double temperature) {
}

void LQT::getTemperature(double temperature) {
}
