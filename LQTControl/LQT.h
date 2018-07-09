#ifndef LQT_H
#define LQT_H

#include <QSerialPort>
#include "fmt/format.h"

class LQT : public QObject {
	Q_OBJECT

private:
	QSerialPort *m_laserPort = new QSerialPort();
	bool m_isConnected = false;
	std::string m_terminator = "\r";

	double m_temperature = 0;
	double m_maxTemperature = 5;
	bool m_modEnabled = false;
	bool m_lockEnabled = false;
	bool m_lock = false;

public:
	LQT();
	~LQT();

	/*
	* Functions regarding the serial communication
	*/

	std::string receive(std::string request);
	void send(std::string message);
	qint64 writeToDevice(const char * data);
	std::string stripCRLF(std::string msg);

	/*
	* Functions for setting and getting the temperature
	*/

	// temperature
	void setTemperature(double temperature);
	double getTemperature();

	// maximum temperature
	void setMaxTemperature(double temperature);
	double getMaxTemperature();

	// force setting the temperature
	void setTemperatureForce(double temperature);
	void setMaxTemperatureForce(double temperature);

	/*
	* Functions for setting and getting the lock states
	*/

	void setFeature(std::string feature, bool value);

	// control temperature feature
	void enableTemperatureControl(bool enable);

	// mod status
	void setMod(bool mod);
	bool getMod();

	// lock status
	void setLock(bool lock);
	bool getLock();

	// lock enabled status
	void setLe(bool le);
	bool getLe();

public slots:
	void connect();
	void disconnect();

signals:
	void connected(bool);
};

#endif // LQT_H