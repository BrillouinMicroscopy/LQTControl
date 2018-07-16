#ifndef LQT_H
#define LQT_H

#include <QSerialPort>
#include "fmt/format.h"

typedef struct {
	double temperature = 0;
	double maxTemperature = 5;
	bool modEnabled = false;
	bool lockEnabled = false;
	bool lock = false;
} LQT_SETTINGS;

class LQT : public QObject {
	Q_OBJECT

private:
	QSerialPort *m_laserPort = nullptr;
	bool m_isConnected = false;
	std::string m_terminator = "\r";

	LQT_SETTINGS m_settings;
	void readSettings();

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
	double setTemperature(double temperature);
	double getTemperature();

	// maximum temperature
	double setMaxTemperature(double temperature);
	double getMaxTemperature();

	/*
	* Functions for setting and getting the lock states
	*/

	void setFeature(std::string feature, bool value);

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
	void init();
	void connect();
	void disconnect();

	// control temperature feature
	void enableTemperatureControl(bool enable);

	// force setting the temperature
	void setTemperatureForce(double temperature);
	void setMaxTemperatureForce(double temperature);

signals:
	void connected(bool);
	void settingsChanged(LQT_SETTINGS);
};

#endif // LQT_H