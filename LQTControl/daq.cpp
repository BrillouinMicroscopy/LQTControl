#include "daq.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

/* Definitions of PS2000 driver routines */
#include "ps2000.h"

int16_t		overflow;
int32_t		scale_to_mv = 1;

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
	PS2000_THRESHOLD_DIRECTION	channelA;
	PS2000_THRESHOLD_DIRECTION	channelB;
	PS2000_THRESHOLD_DIRECTION	channelC;
	PS2000_THRESHOLD_DIRECTION	channelD;
	PS2000_THRESHOLD_DIRECTION	ext;
} DIRECTIONS;

typedef struct {
	PS2000_PWQ_CONDITIONS			*conditions;
	int16_t							nConditions;
	PS2000_THRESHOLD_DIRECTION		direction;
	uint32_t						lower;
	uint32_t						upper;
	PS2000_PULSE_WIDTH_TYPE			type;
} PULSE_WIDTH_QUALIFIER;

typedef struct {
	PS2000_CHANNEL channel;
	float threshold;
	int16_t direction;
	float delay;
} SIMPLE;

typedef struct {
	int16_t hysteresis;
	DIRECTIONS directions;
	int16_t nProperties;
	PS2000_TRIGGER_CONDITIONS * conditions;
	PS2000_TRIGGER_CHANNEL_PROPERTIES * channelProperties;
	PULSE_WIDTH_QUALIFIER pwq;
	uint32_t totalSamples;
	int16_t autoStop;
	int16_t triggered;
} ADVANCED;

typedef struct {
	SIMPLE simple;
	ADVANCED advanced;
} TRIGGER_CHANNEL;

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
	TRIGGER_CHANNEL trigger;
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

// Struct to help with retrieving data into 
// application buffers in streaming data capture
typedef struct {
	UNIT_MODEL unit;
	int16_t *appBuffers[DUAL_SCOPE * 2];
	uint32_t bufferSizes[PS2000_MAX_CHANNELS];
} BUFFER_INFO;

UNIT_MODEL unitOpened;

BUFFER_INFO bufferInfo;

int32_t input_ranges[PS2000_MAX_RANGES] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000 };

daq::daq(QObject *parent) :
	QObject(parent) {
}

void daq::startStopAcquisition() {
	if (timer->isActive()) {
		timer->stop();
		m_acquisitionRunning = false;
	} else {
		daq::setAcquisitionParameters();
		timer->start(20);
		m_acquisitionRunning = true;
	}
	emit(s_acquisitionRunning(m_acquisitionRunning));
}

void daq::setSampleRate(int index) {
	acquisitionParameters.timebase = index;
	daq::setAcquisitionParameters();
}

void daq::setNumberSamples(int32_t no_of_samples) {
	acquisitionParameters.no_of_samples = no_of_samples;
	daq::setAcquisitionParameters();
}

void daq::setCoupling(int index, int ch) {
	acquisitionParameters.channelSettings[ch].DCcoupled = index;
	unitOpened.channelSettings[ch].DCcoupled = (bool)index;
	daq::set_defaults();
}

void daq::setRange(int index, int ch) {
	if (index < 9) {
		acquisitionParameters.channelSettings[ch].enabled = true;
		unitOpened.channelSettings[ch].enabled = true;
		acquisitionParameters.channelSettings[ch].range = index + 2;
		unitOpened.channelSettings[ch].range = index + 2;
	} else if (index == 9) {
		// set auto range
	} else {
		acquisitionParameters.channelSettings[ch].enabled = false;
		unitOpened.channelSettings[ch].enabled = false;
	}
	daq::set_defaults();
}

void daq::setAcquisitionParameters() {

	int16_t maxChannels = (2 < unitOpened.noOfChannels) ? 2 : unitOpened.noOfChannels;

	for (int16_t ch(0); ch < maxChannels; ch++) {
		unitOpened.channelSettings[ch].enabled = acquisitionParameters.channelSettings[ch].enabled;
		unitOpened.channelSettings[ch].DCcoupled = acquisitionParameters.channelSettings[ch].DCcoupled;
		unitOpened.channelSettings[ch].range = acquisitionParameters.channelSettings[ch].range;
	}
	
	// initialize the ADC
	set_defaults();

	/* Trigger disabled */
	ps2000_set_trigger(unitOpened.handle, PS2000_NONE, 0, PS2000_RISING, 0, acquisitionParameters.auto_trigger_ms);

	/*  find the maximum number of samples, the time interval (in time_units),
	*		 the most suitable time units, and the maximum oversample at the current timebase
	*/
	acquisitionParameters.oversample = 1;
	while (!ps2000_get_timebase(
		unitOpened.handle,
		acquisitionParameters.timebase,
		acquisitionParameters.no_of_samples,
		&acquisitionParameters.time_interval,
		&acquisitionParameters.time_units,
		acquisitionParameters.oversample,
		&acquisitionParameters.max_samples)
		) {
		acquisitionParameters.timebase++;
	};

	emit acquisitionParametersChanged(acquisitionParameters);
}

std::array<std::vector<int32_t>, PS2000_MAX_CHANNELS> daq::collectBlockData() {

	int32_t times[DAQ_BUFFER_SIZE];

	/* Start collecting data,
	*  wait for completion */
	ps2000_run_block(
		unitOpened.handle,
		acquisitionParameters.no_of_samples,
		acquisitionParameters.timebase,
		acquisitionParameters.oversample,
		&acquisitionParameters.time_indisposed_ms
	);

	while (!ps2000_ready(unitOpened.handle)) {
		Sleep(10);
	}

	ps2000_stop(unitOpened.handle);

	/* Should be done now...
	*  get the times (in nanoseconds)
	*  and the values (in ADC counts)
	*/



	ps2000_get_times_and_values(
		unitOpened.handle,
		times,
		unitOpened.channelSettings[PS2000_CHANNEL_A].values,
		unitOpened.channelSettings[PS2000_CHANNEL_B].values,
		NULL,
		NULL,
		&overflow,
		acquisitionParameters.time_units,
		acquisitionParameters.no_of_samples
	);


	// create vector of voltage values
	std::array<std::vector<int32_t>, PS2000_MAX_CHANNELS> values;
	for (int i(0); i < acquisitionParameters.no_of_samples; i++) {
		for (int ch(0); ch < unitOpened.noOfChannels; ch++) {
			if (unitOpened.channelSettings[ch].enabled) {
				values[ch].push_back(adc_to_mv(unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range));
			}
		}
	}
	return values;
}

void daq::getBlockData() {
	std::array<std::vector<int32_t>, PS2000_MAX_CHANNELS> values = daq::collectBlockData();

	m_liveBuffer->m_freeBuffers->acquire();
	int16_t **buffer = m_liveBuffer->getWriteBuffer();
	for (int channel(0); channel < values.size(); channel++) {
		for (int jj(0); jj < values[channel].size(); jj++) {
			buffer[channel][jj] = values[channel][jj];
		}
	}
	m_liveBuffer->m_usedBuffers->release();

	emit collectedBlockData();
}

/****************************************************************************
* adc_to_mv
*
* If the user selects scaling to millivolts,
* Convert an 12-bit ADC count into millivolts
****************************************************************************/
int32_t daq::adc_to_mv(int32_t raw, int32_t ch) {
	return (scale_to_mv) ? (raw * input_ranges[ch]) / 32767 : raw;
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 12-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t daq::mv_to_adc(int16_t mv, int16_t ch) {
	return ((mv * 32767) / input_ranges[ch]);
}

/****************************************************************************
* set_defaults - restore default settings
****************************************************************************/
void daq::set_defaults(void) {
	int16_t ch = 0;
	ps2000_set_ets(unitOpened.handle, PS2000_ETS_OFF, 0, 0);

	for (ch = 0; ch < unitOpened.noOfChannels; ch++) {
		ps2000_set_channel(unitOpened.handle,
			ch,
			unitOpened.channelSettings[ch].enabled,
			unitOpened.channelSettings[ch].DCcoupled,
			unitOpened.channelSettings[ch].range
		);
	}
}

void daq::connect() {
	if (!m_isConnected) {
		unitOpened.handle = ps2000_open_unit();
		get_info();

		if (!unitOpened.handle) {
			m_isConnected = false;
		} else {
			daq::setAcquisitionParameters();
			m_isConnected = true;
		}
	}
	emit(connected(m_isConnected));
}

void daq::disconnect() {
	if (m_isConnected) {
		ps2000_close_unit(unitOpened.handle);
		unitOpened.handle = NULL;
		m_isConnected = false;
	}
	emit(connected(m_isConnected));
}

void daq::init() {
	// create timers and connect their signals
	// after moving daq to another thread
	timer = new QTimer();
	QWidget::connect(timer, SIGNAL(timeout()), this, SLOT(getBlockData()));
}

void daq::get_info(void) {
	int8_t description[8][25] = {
		"Driver Version   ",
		"USB Version      ",
		"Hardware Version ",
		"Variant Info     ",
		"Serial           ",
		"Cal Date         ",
		"Error Code       ",
		"Kernel Driver    " 
	};
	int16_t	i;
	int8_t	line[80];
	int32_t	variant;

	if (unitOpened.handle) {
		for (i = 0; i < 6; i++) {
			ps2000_get_unit_info(unitOpened.handle, line, sizeof(line), i);

			if (i == 3) {
				variant = atoi((const char*)line);

				// Identify if 2204A or 2205A
				if (strlen((const char*)line) == 5) {
					line[4] = toupper(line[4]);

					// i.e 2204A -> 0xA204
					if (line[1] == '2' && line[4] == 'A') {
						variant += 0x9968;
					}
				}
			}
			printf("%s: %s\n", description[i], line);
		}

		switch (variant) {
			case MODEL_PS2104:
				unitOpened.model = MODEL_PS2104;
				unitOpened.firstRange = PS2000_100MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2104_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 1;
				unitOpened.hasAdvancedTriggering = false;
				unitOpened.hasSignalGenerator = false;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = false;
				break;

			case MODEL_PS2105:
				unitOpened.model = MODEL_PS2105;
				unitOpened.firstRange = PS2000_100MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2105_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 1;
				unitOpened.hasAdvancedTriggering = false;
				unitOpened.hasSignalGenerator = false;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = false;
				break;

			case MODEL_PS2202:
				unitOpened.model = MODEL_PS2202;
				unitOpened.firstRange = PS2000_100MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2200_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = false;
				unitOpened.hasSignalGenerator = false;
				unitOpened.hasEts = false;
				unitOpened.hasFastStreaming = false;
				break;

			case MODEL_PS2203:
				unitOpened.model = MODEL_PS2203;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = true;
				unitOpened.hasSignalGenerator = true;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = true;
				break;

			case MODEL_PS2204:
				unitOpened.model = MODEL_PS2204;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = true;
				unitOpened.hasSignalGenerator = true;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = true;
				unitOpened.bufferSize = 8000;
				break;

			case MODEL_PS2204A:
				unitOpened.model = MODEL_PS2204A;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = DUAL_SCOPE;
				unitOpened.hasAdvancedTriggering = true;
				unitOpened.hasSignalGenerator = true;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = true;
				unitOpened.awgBufferSize = 4096;
				unitOpened.bufferSize = 8000;
				break;

			case MODEL_PS2205:
				unitOpened.model = MODEL_PS2205;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = true;
				unitOpened.hasSignalGenerator = true;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = true;
				unitOpened.bufferSize = 16000;
				break;

			case MODEL_PS2205A:
				unitOpened.model = MODEL_PS2205A;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = DUAL_SCOPE;
				unitOpened.hasAdvancedTriggering = true;
				unitOpened.hasSignalGenerator = true;
				unitOpened.hasEts = true;
				unitOpened.hasFastStreaming = true;
				unitOpened.awgBufferSize = 4096;
				unitOpened.bufferSize = 16000;
				break;

			default:
				printf("Unit not supported");
		}

		unitOpened.channelSettings[PS2000_CHANNEL_A].enabled = 1;
		unitOpened.channelSettings[PS2000_CHANNEL_A].DCcoupled = 1;
		unitOpened.channelSettings[PS2000_CHANNEL_A].range = PS2000_5V;

		if (unitOpened.noOfChannels == DUAL_SCOPE) {
			unitOpened.channelSettings[PS2000_CHANNEL_B].enabled = 1;
		} else {
			unitOpened.channelSettings[PS2000_CHANNEL_B].enabled = 0;
		}

		unitOpened.channelSettings[PS2000_CHANNEL_B].DCcoupled = 1;
		unitOpened.channelSettings[PS2000_CHANNEL_B].range = PS2000_5V;

		set_defaults();

	} else {
		printf("Unit Not Opened\n");

		ps2000_get_unit_info(unitOpened.handle, line, sizeof(line), 5);

		printf("%s: %s\n", description[5], line);
		unitOpened.model = MODEL_NONE;
		unitOpened.firstRange = PS2000_100MV;
		unitOpened.lastRange = PS2000_20V;
		unitOpened.timebases = PS2105_MAX_TIMEBASE;
		unitOpened.noOfChannels = SINGLE_CH_SCOPE;
	}
}