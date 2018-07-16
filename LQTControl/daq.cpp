#include "daq.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

/* Definitions of PS2000 driver routines */
#include "ps2000.h"

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
	m_unitOpened.channelSettings[ch].DCcoupled = (bool)index;
	daq::set_defaults();
}

void daq::setRange(int index, int ch) {
	if (index < 9) {
		acquisitionParameters.channelSettings[ch].enabled = true;
		m_unitOpened.channelSettings[ch].enabled = true;
		acquisitionParameters.channelSettings[ch].range = index + 2;
		m_unitOpened.channelSettings[ch].range = index + 2;
	} else if (index == 9) {
		// set auto range
	} else {
		acquisitionParameters.channelSettings[ch].enabled = false;
		m_unitOpened.channelSettings[ch].enabled = false;
	}
	daq::set_defaults();
}

void daq::setAcquisitionParameters() {

	int16_t maxChannels = (2 < m_unitOpened.noOfChannels) ? 2 : m_unitOpened.noOfChannels;

	for (int16_t ch(0); ch < maxChannels; ch++) {
		m_unitOpened.channelSettings[ch].enabled = acquisitionParameters.channelSettings[ch].enabled;
		m_unitOpened.channelSettings[ch].DCcoupled = acquisitionParameters.channelSettings[ch].DCcoupled;
		m_unitOpened.channelSettings[ch].range = acquisitionParameters.channelSettings[ch].range;
	}
	
	// initialize the ADC
	set_defaults();

	/* Trigger disabled */
	ps2000_set_trigger(m_unitOpened.handle, PS2000_NONE, 0, PS2000_RISING, 0, acquisitionParameters.auto_trigger_ms);

	/*  find the maximum number of samples, the time interval (in time_units),
	*		 the most suitable time units, and the maximum oversample at the current timebase
	*/
	acquisitionParameters.oversample = 1;
	while (!ps2000_get_timebase(
		m_unitOpened.handle,
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
		m_unitOpened.handle,
		acquisitionParameters.no_of_samples,
		acquisitionParameters.timebase,
		acquisitionParameters.oversample,
		&acquisitionParameters.time_indisposed_ms
	);

	while (!ps2000_ready(m_unitOpened.handle)) {
		Sleep(10);
	}

	ps2000_stop(m_unitOpened.handle);

	/* Should be done now...
	*  get the times (in nanoseconds)
	*  and the values (in ADC counts)
	*/



	ps2000_get_times_and_values(
		m_unitOpened.handle,
		times,
		m_unitOpened.channelSettings[PS2000_CHANNEL_A].values,
		m_unitOpened.channelSettings[PS2000_CHANNEL_B].values,
		NULL,
		NULL,
		&m_overflow,
		acquisitionParameters.time_units,
		acquisitionParameters.no_of_samples
	);


	// create vector of voltage values
	std::array<std::vector<int32_t>, PS2000_MAX_CHANNELS> values;
	for (int i(0); i < acquisitionParameters.no_of_samples; i++) {
		for (int ch(0); ch < m_unitOpened.noOfChannels; ch++) {
			if (m_unitOpened.channelSettings[ch].enabled) {
				values[ch].push_back(adc_to_mv(m_unitOpened.channelSettings[ch].values[i], m_unitOpened.channelSettings[ch].range));
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
	return (m_scale_to_mv) ? (raw * m_input_ranges[ch]) / 32767 : raw;
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 12-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t daq::mv_to_adc(int16_t mv, int16_t ch) {
	return ((mv * 32767) / m_input_ranges[ch]);
}

/****************************************************************************
* set_defaults - restore default settings
****************************************************************************/
void daq::set_defaults(void) {
	int16_t ch = 0;
	ps2000_set_ets(m_unitOpened.handle, PS2000_ETS_OFF, 0, 0);

	for (ch = 0; ch < m_unitOpened.noOfChannels; ch++) {
		ps2000_set_channel(m_unitOpened.handle,
			ch,
			m_unitOpened.channelSettings[ch].enabled,
			m_unitOpened.channelSettings[ch].DCcoupled,
			m_unitOpened.channelSettings[ch].range
		);
	}
}

void daq::connect() {
	if (!m_isConnected) {
		m_unitOpened.handle = ps2000_open_unit();
		get_info();

		if (!m_unitOpened.handle) {
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
		ps2000_close_unit(m_unitOpened.handle);
		m_unitOpened.handle = NULL;
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

	if (m_unitOpened.handle) {
		for (i = 0; i < 6; i++) {
			ps2000_get_unit_info(m_unitOpened.handle, line, sizeof(line), i);

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
				m_unitOpened.model = MODEL_PS2104;
				m_unitOpened.firstRange = PS2000_100MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2104_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = 1;
				m_unitOpened.hasAdvancedTriggering = false;
				m_unitOpened.hasSignalGenerator = false;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = false;
				break;

			case MODEL_PS2105:
				m_unitOpened.model = MODEL_PS2105;
				m_unitOpened.firstRange = PS2000_100MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2105_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = 1;
				m_unitOpened.hasAdvancedTriggering = false;
				m_unitOpened.hasSignalGenerator = false;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = false;
				break;

			case MODEL_PS2202:
				m_unitOpened.model = MODEL_PS2202;
				m_unitOpened.firstRange = PS2000_100MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2200_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = 2;
				m_unitOpened.hasAdvancedTriggering = false;
				m_unitOpened.hasSignalGenerator = false;
				m_unitOpened.hasEts = false;
				m_unitOpened.hasFastStreaming = false;
				break;

			case MODEL_PS2203:
				m_unitOpened.model = MODEL_PS2203;
				m_unitOpened.firstRange = PS2000_50MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = 2;
				m_unitOpened.hasAdvancedTriggering = true;
				m_unitOpened.hasSignalGenerator = true;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = true;
				break;

			case MODEL_PS2204:
				m_unitOpened.model = MODEL_PS2204;
				m_unitOpened.firstRange = PS2000_50MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = 2;
				m_unitOpened.hasAdvancedTriggering = true;
				m_unitOpened.hasSignalGenerator = true;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = true;
				m_unitOpened.bufferSize = 8000;
				break;

			case MODEL_PS2204A:
				m_unitOpened.model = MODEL_PS2204A;
				m_unitOpened.firstRange = PS2000_50MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = DUAL_SCOPE;
				m_unitOpened.hasAdvancedTriggering = true;
				m_unitOpened.hasSignalGenerator = true;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = true;
				m_unitOpened.awgBufferSize = 4096;
				m_unitOpened.bufferSize = 8000;
				break;

			case MODEL_PS2205:
				m_unitOpened.model = MODEL_PS2205;
				m_unitOpened.firstRange = PS2000_50MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = 2;
				m_unitOpened.hasAdvancedTriggering = true;
				m_unitOpened.hasSignalGenerator = true;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = true;
				m_unitOpened.bufferSize = 16000;
				break;

			case MODEL_PS2205A:
				m_unitOpened.model = MODEL_PS2205A;
				m_unitOpened.firstRange = PS2000_50MV;
				m_unitOpened.lastRange = PS2000_20V;
				m_unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				m_unitOpened.timebases = m_unitOpened.maxTimebase;
				m_unitOpened.noOfChannels = DUAL_SCOPE;
				m_unitOpened.hasAdvancedTriggering = true;
				m_unitOpened.hasSignalGenerator = true;
				m_unitOpened.hasEts = true;
				m_unitOpened.hasFastStreaming = true;
				m_unitOpened.awgBufferSize = 4096;
				m_unitOpened.bufferSize = 16000;
				break;

			default:
				printf("Unit not supported");
		}

		m_unitOpened.channelSettings[PS2000_CHANNEL_A].enabled = 1;
		m_unitOpened.channelSettings[PS2000_CHANNEL_A].DCcoupled = 1;
		m_unitOpened.channelSettings[PS2000_CHANNEL_A].range = PS2000_5V;

		if (m_unitOpened.noOfChannels == DUAL_SCOPE) {
			m_unitOpened.channelSettings[PS2000_CHANNEL_B].enabled = 1;
		} else {
			m_unitOpened.channelSettings[PS2000_CHANNEL_B].enabled = 0;
		}

		m_unitOpened.channelSettings[PS2000_CHANNEL_B].DCcoupled = 1;
		m_unitOpened.channelSettings[PS2000_CHANNEL_B].range = PS2000_5V;

		set_defaults();

	} else {
		printf("Unit Not Opened\n");

		ps2000_get_unit_info(m_unitOpened.handle, line, sizeof(line), 5);

		printf("%s: %s\n", description[5], line);
		m_unitOpened.model = MODEL_NONE;
		m_unitOpened.firstRange = PS2000_100MV;
		m_unitOpened.lastRange = PS2000_20V;
		m_unitOpened.timebases = PS2105_MAX_TIMEBASE;
		m_unitOpened.noOfChannels = SINGLE_CH_SCOPE;
	}
}