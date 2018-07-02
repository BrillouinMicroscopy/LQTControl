#ifndef LQT_H
#define LQT_H

#include <QSerialPort>

class LQT {

private:
	QSerialPort *m_laserPort;
	bool m_isConnected = false;

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

	// control temperature feature
	void enableTemperatureControl();
	void disableTemperatureControl();

	// temperature
	void setTemperature(double temperature);
	void getTemperature(double temperature);

	// maximum temperature
	void setMaxTemperature(double temperature);
	void getMaxTemperature(double temperature);

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