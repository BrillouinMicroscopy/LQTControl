#ifndef DAQ_H
#define DAQ_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWidgets>
#include <vector>
#include <array>

#include "ps2000.h"
#include "DFT.h"
#include "generalmath.h"

#define BUFFER_SIZE 	8000
#define BUFFER_SIZE_STREAMING 50000		// Overview buffer size
#define NUM_STREAMING_SAMPLES 100000	// Number of streaming samples to collect
#define MAX_CHANNELS 4
#define SINGLE_CH_SCOPE 1				// Single channel scope
#define DUAL_SCOPE 2					// Dual channel scope

typedef struct {
	int16_t 	auto_trigger_ms = 0;
	int32_t 	time_interval;
	int16_t 	time_units;
	int16_t 	oversample;
	int32_t 	no_of_samples = 6250;	// set to a value which allows a clean frequency analysis for 5000 Hz modulation frequency at timebase 10
	int32_t 	max_samples;
	int32_t 	time_indisposed_ms;
	int16_t		timebase = 10;
} ACQUISITION_PARAMETERS;

typedef struct {
	int32_t amplitude = 2e6;		// [µV] peak to peak voltage
	int32_t offset = 0;				// 
	int16_t waveform = 3;			// type of waveform
	int32_t frequency = 100;		// frequency of the scan
	int32_t	nrSteps = 100;			// number of steps for the manual scan
} SCAN_PARAMETERS;

typedef struct {
	int32_t nrSteps = 0;
	std::vector<int32_t> voltages;	// [µV] output voltage (<int32_t> is sufficient for this)
	std::vector<int32_t> intensity;	// [µV] measured intensity (<int32_t> is fine)
	AMPLITUDES amplitudes;			// amplitudes of the first and second harmonic
	std::vector<double> quotients;	// quotients of the amplitudes of the first and second harmonic
} SCAN_RESULTS;

typedef struct {
	double proportional = 1.0;
	double integral = 1.0;
	double derivative = 1.0;
} LOCKIN_PARAMETERS;

class daq : public QObject {
	Q_OBJECT

	public:
		explicit daq(QObject *parent = 0);
		bool startStopAcquisition();
		void getBlockData();
		QVector<QPointF> getStreamingBuffer(int ch);
		bool connect();
		bool disconnect();
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
		void scanManual();
		SCAN_PARAMETERS getScanParameters();
		SCAN_RESULTS getScanResults();
		LOCKIN_PARAMETERS getLockInParameters();

	private slots:

	signals:
		void scanDone();
		void collectedData();
		void collectedBlockData(std::array<QVector<QPointF>, PS2000_MAX_CHANNELS>);
		void acquisitionParametersChanged(ACQUISITION_PARAMETERS);

	private:
		static void __stdcall ps2000FastStreamingReady2(
			int16_t **overviewBuffers,
			int16_t   overflow,
			uint32_t  triggeredAt,
			int16_t   triggered,
			int16_t   auto_stop,
			uint32_t  nValues
		);
		ACQUISITION_PARAMETERS setAcquisitionParameters();
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
		QVector<QPointF> points;
		SCAN_PARAMETERS scanParameters;
		ACQUISITION_PARAMETERS acquisitionParameters;
		SCAN_RESULTS scanResults;
		LOCKIN_PARAMETERS lockInParameters;
		double mean(std::vector<int>);
		double mean(std::vector<double>);
		std::complex<double> mean(std::vector<std::complex<double>>);
};

#endif // DAQ_H