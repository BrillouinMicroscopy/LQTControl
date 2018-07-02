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
#include "PDH.h"
#include "generalmath.h"

#define BUFFER_SIZE 	8000
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
	int32_t 	no_of_samples = 1000;	// set to a value which allows a clean frequency analysis for 5000 Hz modulation frequency at timebase 10
	int32_t 	max_samples;
	int32_t 	time_indisposed_ms;
	int16_t		timebase = 10;
	DEFAULT_CHANNEL_SETTINGS channelSettings[2] = {
		{0, PS2000_RANGE::PS2000_200MV, TRUE},
		{0, PS2000_RANGE::PS2000_500MV, TRUE}
	};
} ACQUISITION_PARAMETERS;

typedef struct {
	int32_t amplitude = 2e6;		// [�V] peak to peak voltage
	int32_t offset = 0;				// 
	int16_t waveform = 3;			// type of waveform
	int32_t frequency = 100;		// frequency of the scan
	int32_t	nrSteps = 100;			// number of steps for the manual scan
} SCAN_PARAMETERS;

typedef struct {
	int32_t nrSteps = 0;
	std::vector<int32_t> voltages;	// [�V] output voltage (<int32_t> is sufficient for this)
	std::vector<int32_t> intensity;	// [�V] measured intensity (<int32_t> is fine)
	std::vector<double> error;		// PDH error signal
} SCAN_DATA;

typedef struct {
	double proportional = 0.01;		//		control parameter of the proportional part
	double integral = 0.005;			//		control parameter of the integral part
	double derivative = 0.0;		//		control parameter of the derivative part
	double frequency = 5000;		// [Hz] approx. frequency of the reference signal
	double phase = 0;				// [�]	phase shift between reference and detector signal
	bool active = FALSE;			//		locking enabled?
	bool compensate = FALSE;		//		compensate the offset?
	bool compensating = FALSE;		//		is it currently compensating?
	double maxOffset = 0.4;			// [V]	maximum voltage of the external input before the offset compensation kicks in
	double targetOffset = 0.1;		// [V]	target voltage of the offset compensation
} LOCK_PARAMETERS;

typedef struct {
	std::vector<std::chrono::time_point<std::chrono::system_clock>> time;		// [s]	time vector
	std::vector<int32_t> voltage;	// [�V]	output voltage (<int32_t> is sufficient for this)
	std::vector<int32_t> amplitude;	// [�V]	measured intensity (<int32_t> is fine)
	std::vector<double> error;		// [1]	PDH error signal
	double iError = 0;				// [1]	integral value of the error signal
} LOCK_DATA;

typedef enum enLockState {
	INACTIVE,
	ACTIVE,
	FAILURE
} LOCKSTATE;

enum class liveViewPlotTypes {
	CHANNEL_A,
	CHANNEL_B,
	COUNT
};
enum class scanViewPlotTypes {
	INTENSITY,
	ERRORSIGNAL,
	COUNT
};
enum class lockViewPlotTypes {
	VOLTAGE,
	ERRORSIGNAL,
	AMPLITUDE,
	PIEZOVOLTAGE,
	ERRORSIGNALMEAN,
	ERRORSIGNALSTD,
	COUNT
};

class daq : public QObject {
	Q_OBJECT

	public:
		explicit daq(QObject *parent = 0);
		bool startStopAcquisition();
		bool startStopAcquireLocking();
		bool startStopLocking();
		void disableLocking(LOCKSTATE lockstate = LOCKSTATE::INACTIVE);
		void getBlockData();
		void lock();
		bool enablePiezo();
		bool disablePiezo();
		void incrementPiezoVoltage();
		void decrementPiezoVoltage();
		QVector<QPointF> getStreamingBuffer(int ch);
		bool connect();
		bool disconnect();
		bool connectPiezo();
		bool disconnectPiezo();
		void startStreaming();
		void collectStreamingData();
		void stopStreaming();
		void set_sig_gen();
		void set_trigger_advanced();
		void setSampleRate(int index);
		void setCoupling(int index, int ch);
		void setRange(int index, int ch);
		void setNumberSamples(int32_t no_of_samples);
		void setScanParameters(int type, int value);
		void setLockParameters(int type, double value);
		void toggleOffsetCompensation(bool compensate);
		void scanManual();
		SCAN_PARAMETERS getScanParameters();
		SCAN_DATA getScanData();
		LOCK_PARAMETERS getLockParameters();

		std::array<QVector<QPointF>, PS2000_MAX_CHANNELS> data;
		std::array<QVector<QPointF>, static_cast<int>(lockViewPlotTypes::COUNT)> lockDataPlot;

		double currentVoltage = 0;
		double piezoVoltage = 0;
		int compensationTimer = 0;

	private slots:

	signals:
		void scanDone();
		void collectedData();
		void locked(std::array<QVector<QPointF>, static_cast<int>(lockViewPlotTypes::COUNT)> &);
		void collectedBlockData(std::array<QVector<QPointF>, PS2000_MAX_CHANNELS> &);
		void acquisitionParametersChanged(ACQUISITION_PARAMETERS);
		void lockStateChanged(LOCKSTATE);
		void compensationStateChanged(bool);

	private:
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
		QTimer timer;
		QTimer lockingTimer;
		QVector<QPointF> points;
		ACQUISITION_PARAMETERS acquisitionParameters;
		SCAN_PARAMETERS scanParameters;
		SCAN_DATA scanData;
		LOCK_PARAMETERS lockParameters;
		LOCK_DATA lockData;
};

#endif // DAQ_H