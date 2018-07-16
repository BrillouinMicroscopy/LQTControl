#ifndef LOCKING_H
#define LOCKING_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWidgets>
#include <vector>
#include <array>
#include <chrono>
#include <ctime>

#include "daq.h"
#include "LQT.h"
#include "generalmath.h"

typedef struct {
	double low = -5;		// [K] offset start
	double high = 5;		// [K] offset end
	int32_t	nrSteps = 100;	// number of steps
	int interval = 10;		// [s] interval between steps
} SCAN_SETTINGS;

typedef struct {
	bool m_running = false;				// is the scan currently running
	bool m_abort = false;				// should the scan be aborted
	int pass = 0;
	int32_t nrSteps = 0;
	std::vector<double> temperatures;	// [K] temperature offset
	std::vector<double> reference;		// [µV] measured voltage of reference detector
	std::vector<double> absorption;	// [µV] measured voltage behind absorption cell
	std::vector<double> quotient;		// [1]	quotient of absorption and reference
	std::vector<double> transmission;	// [1]	measured transmission, e.g. quotient normalized to maximum quotient measured
} SCAN_DATA;

typedef enum enLockState {
	INACTIVE,
	ACTIVE,
	FAILURE
} LOCKSTATE;

typedef struct {
	double proportional = 0.007;			//		control parameter of the proportional part
	double integral = 0.000;				//		control parameter of the integral part
	double derivative = 0.0;				//		control parameter of the derivative part
	LOCKSTATE state = LOCKSTATE::INACTIVE;	//		locking enabled?
	double transmissionSetpoint = 0.5;		//	[1]	target transmission setpoint
} LOCK_SETTINGS;

typedef struct {
	std::vector<std::chrono::time_point<std::chrono::system_clock>> time;		// [s]	time vector
	std::vector<double> relTime;		// [s]	time passed since beginning of measurement
	std::vector<double> tempOffset;		// [K]	timeline of the temperature offset
	std::vector<int32_t> absorption;	// [µV]	measured absorption signal (<int32_t> is fine)
	std::vector<int32_t> reference;		// [µV]	measured reference signal (<int32_t> is fine)
	std::vector<double> quotient;		// [1]	quotient of absorption and reference
	std::vector<double> transmission;	// [1]	transmission behind cell, i.e. the quotient normalized to maximum quotient
	std::vector<double> error;			// [1]	PDH error signal
	double iError = 0;					// [1]	integral value of the error signal
	double currentTempOffset = 0;		// [K] current temperature offset
} LOCK_DATA;

enum class liveViewPlotTypes {
	CHANNEL_A,
	CHANNEL_B,
	COUNT
};
enum class scanViewPlotTypes {
	ABSORPTION,
	REFERENCE,
	QUOTIENT,
	TRANSMISSION,
	COUNT
};
enum class lockViewPlotTypes {
	ABSORPTION,
	REFERENCE,
	TRANSMISSION,
	ERRORSIGNAL,
	ERRORSIGNALMEAN,
	ERRORSIGNALSTD,
	TEMPERATUREOFFSET,
	COUNT
};

typedef enum enScanParameters {
	LOW,
	HIGH,
	STEPS,
	INTERVAL
} SCANPARAMETERS;

typedef enum enLockParameters {
	P,
	I,
	D,
	SETPOINT
} LOCKPARAMETERS;

class Locking : public QObject {
	Q_OBJECT

	public:
		explicit Locking(QObject *parent, daq *dataAcquisition, LQT *laserControl);
		void setLockState(LOCKSTATE lockstate = LOCKSTATE::INACTIVE);
		void setScanParameters(SCANPARAMETERS type, double value);
		void setLockParameters(LOCKPARAMETERS type, double value);
		SCAN_SETTINGS getScanSettings();
		SCAN_DATA scanData;
		LOCK_SETTINGS getLockSettings();

		std::array<QVector<QPointF>, static_cast<int>(lockViewPlotTypes::COUNT)> m_lockDataPlot;

	public slots:
		void init();
		void startScan();
		void startStopAcquireLocking();
		void startStopLocking();

	private slots:
		void lock();
		void scan();

	signals:
		void s_scanRunning(bool);
		void s_scanPassAcquired();
		void s_acquireLockingRunning(bool);
		void locked();
		void lockStateChanged(LOCKSTATE);

	private:
		LQT *m_laserControl;
		daq *m_dataAcquisition;
		bool m_acquisitionRunning = false;
		bool m_isAcquireLockingRunning = false;
		QTimer *lockingTimer = nullptr;
		QTimer *scanTimer = nullptr;
		QElapsedTimer passTimer;
		SCAN_SETTINGS scanSettings;
		LOCK_SETTINGS lockSettings;
		LOCK_DATA lockData;
};

#endif // LOCKING_H