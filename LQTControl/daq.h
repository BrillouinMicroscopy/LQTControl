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
#define SINGLE_CH_SCOPE 1				// Single channel scope
#define DUAL_SCOPE 2					// Dual channel scope

#define DAQ_MAX_CHANNELS PS2000_MAX_CHANNELS

typedef enum {
	MODEL_NONE = 0,
	MODEL_PS2104 = 2104,
	MODEL_PS2105 = 2105,
	MODEL_PS2202 = 2202,
	MODEL_PS2203 = 2203,
	MODEL_PS2204 = 2204,
	MODEL_PS2205 = 2205,
	MODEL_PS2204A = 0xA204,
	MODEL_PS2205A = 0xA205
} MODEL_TYPE;

typedef struct {
	int16_t DCcoupled;
	int16_t range;
	int16_t enabled;
	int16_t values[DAQ_BUFFER_SIZE];
} CHANNEL_SETTINGS;

typedef struct {
	int16_t			handle;
	MODEL_TYPE		model;
	PS2000_RANGE	firstRange;
	PS2000_RANGE	lastRange;
	int16_t			maxTimebase;
	int16_t			timebases;
	int16_t			noOfChannels;
	CHANNEL_SETTINGS channelSettings[PS2000_MAX_CHANNELS];
	int16_t			hasAdvancedTriggering;
	int16_t			hasFastStreaming;
	int16_t			hasEts;
	int16_t			hasSignalGenerator;
	int16_t			awgBufferSize;
	int16_t			bufferSize;
} UNIT_MODEL;

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
		{0, PS2000_RANGE::PS2000_200MV, true},
		{0, PS2000_RANGE::PS2000_500MV, true}
	};
} ACQUISITION_PARAMETERS;

class daq : public QObject {
	Q_OBJECT

	public:
		explicit daq(QObject *parent);
		void setAcquisitionParameters();
		std::array<std::vector<int32_t>, DAQ_MAX_CHANNELS> collectBlockData();
		void setSampleRate(int index);
		void setCoupling(int index, int ch);
		void setRange(int index, int ch);
		void setNumberSamples(int32_t no_of_samples);

		CircularBuffer<int16_t> *m_liveBuffer = new CircularBuffer<int16_t>(4, DAQ_MAX_CHANNELS, 8000);

		int32_t m_input_ranges[PS2000_MAX_RANGES] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000 };

	public slots:
		void connect();
		void disconnect();
		void init();
		void startStopAcquisition();

	private slots:
		void getBlockData();

	signals:
		void s_acquisitionRunning(bool);
		void connected(bool);
		void acquisitionParametersChanged(ACQUISITION_PARAMETERS);
		void collectedBlockData();

	private:
		UNIT_MODEL m_unitOpened;
		bool m_isConnected = false;
		bool m_acquisitionRunning = false;
		int32_t adc_to_mv(int32_t raw, int32_t ch);
		int16_t mv_to_adc(int16_t mv, int16_t ch);
		void set_defaults(void);
		void get_info(void);
		QTimer *timer = nullptr;
		ACQUISITION_PARAMETERS acquisitionParameters;
		int16_t m_overflow;
		bool m_scale_to_mv = 1;
};

#endif // DAQ_H