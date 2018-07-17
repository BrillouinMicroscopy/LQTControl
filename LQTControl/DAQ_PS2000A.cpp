#include "daq_PS2000A.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

daq_PS2000A::daq_PS2000A(QObject *parent) : daq(parent, { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000 }) {}

daq_PS2000A::~daq_PS2000A() {
	disconnect_daq();
}

void daq_PS2000A::setAcquisitionParameters() {

	int16_t maxChannels = (2 < m_unitOpened.noOfChannels) ? 2 : m_unitOpened.noOfChannels;

	for (int16_t ch(0); ch < maxChannels; ch++) {
		m_unitOpened.channelSettings[ch].enabled = acquisitionParameters.channelSettings[ch].enabled;
		m_unitOpened.channelSettings[ch].DCcoupled = acquisitionParameters.channelSettings[ch].DCcoupled;
		m_unitOpened.channelSettings[ch].range = acquisitionParameters.channelSettings[ch].range;
	}

	// initialize the ADC
	set_defaults();

	/* Trigger disabled */
	ps2000aSetSimpleTrigger(
		m_unitOpened.handle,		// handle
		NULL,					// enable
		PS2000A_CHANNEL_A,		// source
		NULL,					// threshold
		PS2000A_NONE,			// direction
		NULL,					// delay
		NULL
	);

	/*  find the maximum number of samples, the time interval (in time_units),
	*		 the most suitable time units, and the maximum oversample at the current timebase
	*/
	acquisitionParameters.oversample = 1;

	while (ps2000aGetTimebase(
		m_unitOpened.handle,
		acquisitionParameters.timebase,
		acquisitionParameters.no_of_samples,
		&acquisitionParameters.time_interval,
		acquisitionParameters.oversample,
		&acquisitionParameters.max_samples,
		0)
		) {
		acquisitionParameters.timebase++;
	};

	emit acquisitionParametersChanged(acquisitionParameters);
}

std::array<std::vector<int32_t>, PS2000A_MAX_CHANNELS> daq_PS2000A::collectBlockData() {

	/* Start collecting data,
	*  wait for completion */
	ps2000aRunBlock(
		m_unitOpened.handle,						// handle
		0,										// noOfPreTriggerSamples
		acquisitionParameters.no_of_samples,	// noOfPostTriggerSamples
		acquisitionParameters.timebase,			// timebase
		acquisitionParameters.oversample,		// oversample
		&acquisitionParameters.time_indisposed_ms,	//timeIndisposedMs
		0,										// segmentIndex
		NULL,									// lpReady
		NULL									// * pParameter
	);

	int16_t ready = 0;
	ps2000aIsReady(m_unitOpened.handle, &ready);
	while (!ready) {
		Sleep(10);
		ps2000aIsReady(m_unitOpened.handle, &ready);
	}

	for (int16_t ch(0); ch < 4; ch++) {
		ps2000aSetDataBuffers(
			m_unitOpened.handle,
			(int16_t)ch,
			buffers[ch * 2],
			buffers[ch * 2 + 1],
			DAQ_BUFFER_SIZE,
			0,
			PS2000A_RATIO_MODE_NONE
		);
	}

	ps2000aGetValues(
		m_unitOpened.handle,
		0,
		&acquisitionParameters.no_of_samples,
		NULL,
		PS2000A_RATIO_MODE_NONE,
		0,
		NULL
	);

	ps2000aStop(m_unitOpened.handle);

	// create vector of voltage values
	std::array<std::vector<int32_t>, PS2000A_MAX_CHANNELS> values;
	for (int32_t i(0); i < static_cast<int32_t>(acquisitionParameters.no_of_samples); i++) {
		for (int ch(0); ch < m_unitOpened.noOfChannels; ch++) {
			if (m_unitOpened.channelSettings[ch].enabled) {
				values[ch].push_back(adc_to_mv(buffers[ch * 2][i], m_unitOpened.channelSettings[ch].range));
			}
		}
	}
	return values;
}

/****************************************************************************
* set_defaults - restore default settings
****************************************************************************/
void daq_PS2000A::set_defaults(void) {
	int16_t ch = 0;

	ps2000aSetEts(
		m_unitOpened.handle,
		PS2000A_ETS_OFF,
		0,
		0,
		NULL
	);

	for (ch = 0; ch < m_unitOpened.noOfChannels; ch++) {
		ps2000aSetChannel(
			m_unitOpened.handle,
			PS2000A_CHANNEL(ch),
			m_unitOpened.channelSettings[ch].enabled,
			(PS2000A_COUPLING)m_unitOpened.channelSettings[ch].DCcoupled,
			(PS2000A_RANGE)m_unitOpened.channelSettings[ch].range,
			0											// analog offset
		);
	}
}

void daq_PS2000A::connect_daq() {
	if (!m_isConnected) {
		ps2000aOpenUnit(&m_unitOpened.handle, NULL);

		if (m_unitOpened.handle < 0) {
			m_isConnected = false;
		} else {
			get_info();
			setAcquisitionParameters();
			m_isConnected = true;
		}
	}
	emit(connected(m_isConnected));
}

void daq_PS2000A::disconnect_daq() {
	if (m_isConnected) {
		if (timer->isActive()) {
			timer->stop();
			m_acquisitionRunning = false;
		}
		ps2000aCloseUnit(m_unitOpened.handle);
		m_unitOpened.handle = NULL;
		m_isConnected = false;
	}
	emit(connected(m_isConnected));
}

void daq_PS2000A::get_info(void) {
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

	char line[80];
	int32_t	variant;
	int16_t requiredSize = 0;

	if (m_unitOpened.handle) {
		ps2000aGetUnitInfo(
			m_unitOpened.handle,
			(int8_t *)line,
			sizeof(line),
			&requiredSize,
			PICO_VARIANT_INFO
		);

		variant = atoi((const char*)line);

		// Identify if 2204A or 2205A
		if (strlen((const char*)line) == 5) {
			line[4] = toupper(line[4]);

			// i.e 2204A -> 0xA204
			if (line[1] == '2' && line[4] == 'A') {
				variant += 0x9968;
			}
		}


		switch (variant) {
		case (int)PS2000A_TYPE::MODEL_PS2405A:
			m_unitOpened.model = (int)PS2000A_TYPE::MODEL_PS2405A;
			m_unitOpened.firstRange = PS2000A_50MV;
			m_unitOpened.lastRange = PS2000A_20V;
			m_unitOpened.noOfChannels = QUAD_SCOPE;
			m_unitOpened.hasAdvancedTriggering = TRUE;
			m_unitOpened.hasSignalGenerator = TRUE;
			m_unitOpened.hasEts = TRUE;
			m_unitOpened.hasFastStreaming = TRUE;
			m_unitOpened.awgBufferSize = 4096;
			m_unitOpened.bufferSize = 16000;
			break;

		default:
			printf("Unit not supported");
		}

		m_unitOpened.channelSettings[PS2000A_CHANNEL_A].enabled = 1;
		m_unitOpened.channelSettings[PS2000A_CHANNEL_A].DCcoupled = PS2000A_DC;
		m_unitOpened.channelSettings[PS2000A_CHANNEL_A].range = PS2000A_5V;

		if (m_unitOpened.noOfChannels == DUAL_SCOPE) {
			m_unitOpened.channelSettings[PS2000A_CHANNEL_B].enabled = 1;
		}
		else {
			m_unitOpened.channelSettings[PS2000A_CHANNEL_B].enabled = 0;
		}

		m_unitOpened.channelSettings[PS2000A_CHANNEL_B].DCcoupled = PS2000A_DC;
		m_unitOpened.channelSettings[PS2000A_CHANNEL_B].range = PS2000A_5V;

		set_defaults();

	}
	else {
		//printf("Unit Not Opened\n");

		//ps2000_get_unit_info(m_unitOpened.handle, line, sizeof(line), 5);

		//printf("%s: %s\n", description[5], line);
		//m_unitOpened.model = MODEL_NONE;
		//m_unitOpened.firstRange = PS2000A_100MV;
		//m_unitOpened.lastRange = PS2000A_20V;
		//m_unitOpened.timebases = PS2105A_MAX_TIMEBASE;
		//m_unitOpened.noOfChannels = SINGLE_CH_SCOPE;
	}
}