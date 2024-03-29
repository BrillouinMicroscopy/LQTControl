#ifndef LOCKING_H
#define LOCKING_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWidgets>
#include <vector>
#include <array>
#include <chrono>
#include <ctime>

#include "Devices\daq.h"
#include "Devices\LQT.h"
#include "generalmath.h"

typedef struct SCAN_SETTINGS {
	double low{ -5 };		// [K] offset start
	double high{ 5 };		// [K] offset end
	int32_t	nrSteps{ 100 };	// number of steps
	int interval{ 10 };		// [s] interval between steps
} SCAN_SETTINGS;

typedef struct SCAN_DATA {
	bool m_running{ false };				// is the scan currently running
	bool m_abort{ false };				// should the scan be aborted
	int pass{ 0 };
	int32_t nrSteps{ 0 };
	std::vector<double> temperatures;	// [K] temperature offset
	std::vector<double> reference;		// [�V] measured voltage of reference detector
	std::vector<double> absorption;	// [�V] measured voltage behind absorption cell
	std::vector<double> quotient;		// [1]	quotient of absorption and reference
	std::vector<double> transmission;	// [1]	measured transmission, e.g. quotient normalized to maximum quotient measured
} SCAN_DATA;

typedef enum enLockState {
	INACTIVE,
	ACTIVE,
	FAILURE
} LOCKSTATE;

typedef struct LOCK_SETTINGS {
	double proportional{ 0.007 };			//		control parameter of the proportional part
	double integral{ 0.000 };				//		control parameter of the integral part
	double derivative{ 0.0 };				//		control parameter of the derivative part
	int lockingTimeout{ 100 };				// [ms]	time until next locking run
	LOCKSTATE state{ LOCKSTATE::INACTIVE };	//		locking enabled?
	double transmissionSetpoint{ 0.5 };		//	[1]	target transmission setpoint
} LOCK_SETTINGS;

typedef struct LOCK_DATA {
	std::vector<std::chrono::time_point<std::chrono::system_clock>> time;		// [s]	time vector
	std::vector<double> relTime;		// [s]	time passed since beginning of measurement
	std::vector<double> tempOffset;		// [K]	timeline of the temperature offset
	std::vector<int32_t> absorption;	// [�V]	measured absorption signal (<int32_t> is fine)
	std::vector<int32_t> reference;		// [�V]	measured reference signal (<int32_t> is fine)
	std::vector<double> quotient;		// [1]	quotient of absorption and reference
	std::vector<double> transmission;	// [1]	transmission behind cell, i.e. the quotient normalized to maximum quotient
	std::vector<double> error;			// [1]	PDH error signal
	double quotient_max{ 0 };			// [1]	the maximum measured quotient
	double iError{ 0 };					// [1]	integral value of the error signal
	double currentTempOffset{ 0 };		// [K] current temperature offset
	int storageDuration{ 4 * 3600 };	// [s]	maximum time to store data for (after this time, data from the start will be overwritten)
	int storageSize;					//		size of the storage array (depends on storageDuration and lockSettings.lockingTimeout)
	gsl::index nextIndex{ 0 };			//		the index to write to next
	bool wrapped{ false };				//		Whether we already wrapped once (and now use the full vector)
	std::chrono::time_point<std::chrono::system_clock> startTime;
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
		explicit Locking(QObject *parent, daq **dataAcquisition, LQT *laserControl);
		void setLockState(LOCKSTATE lockstate = LOCKSTATE::INACTIVE);
		void setScanParameters(SCANPARAMETERS type, double value);
		void setLockParameters(LOCKPARAMETERS type, double value);
		SCAN_SETTINGS getScanSettings();
		SCAN_DATA scanData;
		LOCK_SETTINGS getLockSettings();

		LOCK_DATA lockData;

	public slots:
		void init();
		void startScan();
		void startStopAcquireLocking();
		void startStopLocking();

	private:
		LQT* m_laserControl;
		daq** m_dataAcquisition;
		bool m_acquisitionRunning{ false };
		bool m_isAcquireLockingRunning{ false };
		QTimer* lockingTimer{ nullptr };
		QTimer* scanTimer{ nullptr };
		QElapsedTimer passTimer;
		SCAN_SETTINGS scanSettings;
		LOCK_SETTINGS lockSettings;

	private slots:
		void lock();
		void scan();

	signals:
		void s_scanRunning(bool);
		void s_scanPassAcquired();
		void s_acquireLockingRunning(bool);
		void locked();
		void lockStateChanged(LOCKSTATE);
};

#endif // LOCKING_H