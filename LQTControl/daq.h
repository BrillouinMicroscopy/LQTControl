#ifndef DAQ_H
#define DAQ_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWidgets>
#include <vector>
#include <array>
#include <chrono>
#include <ctime>

#include "ps2000.h"
#include "LQT.h"
#include "circularBuffer.h"
#include "generalmath.h"

#define DAQ_BUFFER_SIZE 	8000
#define BUFFER_SIZE_STREAMING 50000		// Overview buffer size
#define NUM_STREAMING_SAMPLES 100000	// Number of streaming samples to collect
#define MAX_CHANNELS 4
#define SINGLE_CH_SCOPE 1				// Single channel scope
#define DUAL_SCOPE 2					// Dual channel scope

typedef struct {
	int16_t DCcoupled;
	int16_t range;
	int16_t enabled;
} DEFAULT_CHANNEL_SETTINGS;

typedef struct {
	int16_t 	auto_trigger_ms = 0;
	int32_t 	time_interval;
	int16_t 	time_units;
	int16_t 	oversample;
	int32_t 	no_of_samples = 1000;
	int32_t 	max_samples;
	int32_t 	time_indisposed_ms;
	int16_t		timebase = 10;
	DEFAULT_CHANNEL_SETTINGS channelSettings[2] = {
		{0, PS2000_RANGE::PS2000_200MV, TRUE},
		{0, PS2000_RANGE::PS2000_500MV, TRUE}
	};
} ACQUISITION_PARAMETERS;

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
	double proportional = 0.1;				//		control parameter of the proportional part
	double integral = 0.005;				//		control parameter of the integral part
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

class daq : public QObject {
	Q_OBJECT

	public:
		explicit daq(QObject *parent, LQT *laserControl);
		bool startStopAcquireLocking();
		void startStopLocking();
		void setLockState(LOCKSTATE lockstate = LOCKSTATE::INACTIVE);
		QVector<QPointF> getStreamingBuffer(int ch);
		void startStreaming();
		void collectStreamingData();
		void stopStreaming();
		void set_trigger_advanced();
		void setSampleRate(int index);
		void setCoupling(int index, int ch);
		void setRange(int index, int ch);
		void setNumberSamples(int32_t no_of_samples);
		void setScanParameters(SCANPARAMETERS type, double value);
		void setLockParameters(LOCKPARAMETERS type, double value);
		SCAN_SETTINGS getScanSettings();
		SCAN_DATA scanData;
		LOCK_SETTINGS getLockSettings();

		std::array<QVector<QPointF>, PS2000_MAX_CHANNELS> data;

		CircularBuffer<int16_t> *m_liveBuffer = new CircularBuffer<int16_t>(4, PS2000_MAX_CHANNELS, 8000);

		std::array<QVector<QPointF>, static_cast<int>(lockViewPlotTypes::COUNT)> lockDataPlot;

	public slots:
		void connect();
		void disconnect();
		void init();
		void startScan();
		void startStopAcquisition();

	private slots:
		void getBlockData();
		void lock();
		void scan();

	signals:
		void connected(bool);
		void s_acquisitionRunning(bool);
		void s_scanRunning(bool);
		void s_scanPassAcquired();
		void collectedData();
		void locked(std::array<QVector<QPointF>, static_cast<int>(lockViewPlotTypes::COUNT)> &);
		void collectedBlockData();
		void acquisitionParametersChanged(ACQUISITION_PARAMETERS);
		void lockStateChanged(LOCKSTATE);

	private:
		bool m_isConnected = false;
		bool m_acquisitionRunning = false;
		static void __stdcall ps2000FastStreamingReady2(
			int16_t **overviewBuffers,
			int16_t   overflow,
			uint32_t  triggeredAt,
			int16_t   triggered,
			int16_t   auto_stop,
			uint32_t  nValues
		);
		void setAcquisitionParameters();
		std::array<std::vector<int32_t>, PS2000_MAX_CHANNELS> collectBlockData();
		int32_t adc_to_mv(
			int32_t raw,
			int32_t ch
		);
		int16_t mv_to_adc(
			int16_t mv,
			int16_t ch
		);
		void set_defaults(void);
		void get_info(void);
		QTimer *timer = nullptr;
		QTimer *lockingTimer = nullptr;
		QTimer *scanTimer = nullptr;
		QElapsedTimer passTimer;
		LQT *m_laserControl;
		QVector<QPointF> points;
		ACQUISITION_PARAMETERS acquisitionParameters;
		SCAN_SETTINGS scanSettings;
		LOCK_SETTINGS lockSettings;
		LOCK_DATA lockData;
};

#endif // DAQ_H