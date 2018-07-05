#ifndef LQT_H
#define LQT_H

#include <QSerialPort>
#include "fmt/format.h"

class LQT {

private:
	QSerialPort *m_laserPort;
	bool m_isConnected = false;
	std::string m_terminator = "\r";

	double m_temperature = 0;
	double m_maxTemperature = 5;
	bool m_modEnabled = false;
	bool m_lockEnabled = false;
	bool m_lock = false;

public:
	explicit LQT() noexcept;
	~LQT();
	void connect();
	void disconnect();

	/*
	* Functions regarding the serial communication
	*/

	std::string receive(std::string request);
	void send(std::string message);
	qint64 writeToDevice(const char * data);

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
	bool getFeature(std::string feature);

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
};

#endif // LQT_H