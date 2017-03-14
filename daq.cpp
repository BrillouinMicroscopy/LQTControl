#include "daq.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

/* Definitions of PS2000 driver routines */
#include "ps2000.h"
#include <conio.h>

int16_t		values_a[BUFFER_SIZE]; // block mode buffer, Channel A
int16_t		values_b[BUFFER_SIZE]; // block mode buffer, Channel B

int16_t		overflow;
int32_t		scale_to_mv = 1;

int16_t		channel_mv[PS2000_MAX_CHANNELS];
int16_t		timebase = 8;

int16_t		g_overflow = 0;

// Streaming data parameters
int16_t		g_triggered = 0;
uint32_t	g_triggeredAt = 0;
uint32_t	g_nValues;
uint32_t	g_startIndex;			// Start index in application buffer where data should be written to in streaming mode collection
uint32_t	g_prevStartIndex;		// Keep track of previous index into application buffer in streaming mode collection
int16_t		g_appBufferFull = 0;	// Use this in the callback to indicate if it is going to copy past the end of the buffer

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
	PS2000_PWQ_CONDITIONS			*	conditions;
	int16_t							nConditions;
	PS2000_THRESHOLD_DIRECTION		direction;
	uint32_t						lower;
	uint32_t						upper;
	PS2000_PULSE_WIDTH_TYPE			type;
} PULSE_WIDTH_QUALIFIER;

typedef struct
{
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
	int16_t values[BUFFER_SIZE];
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
} UNIT_MODEL;

// Struct to help with retrieving data into 
// application buffers in streaming data capture
typedef struct
{
	UNIT_MODEL unit;
	int16_t *appBuffers[DUAL_SCOPE * 2];
	uint32_t bufferSizes[PS2000_MAX_CHANNELS];
} BUFFER_INFO;

UNIT_MODEL unitOpened;

BUFFER_INFO bufferInfo;

int32_t times[BUFFER_SIZE];

int32_t input_ranges[PS2000_MAX_RANGES] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000 };

daq::daq(QObject *parent) :
	QObject(parent) {
}

QVector<QPointF> daq::acquire(int colCount) {
	QVector<QPointF> points;
	points.reserve(colCount);
	for (int j(0); j < colCount; j++) {
		qreal x(0);
		qreal y(0);
		// data with sin + random component
		y = qSin(3.14159265358979 / 50 * j) + 0.5 + (qreal)rand() / (qreal)RAND_MAX;
		x = j;
		points.append(QPointF(x, y));
	}
	return points;
}

/****************************************************************************
*
* collect_fast_streaming_triggered2
*
* Demonstates how to retrieve data from the device while it is collecting
* streaming data. This data is not aggregated.
*
* Data is collected into an application buffer specified for a channel. If
* the maximum size of the application buffer has been reached, the
* application will stop collecting data.
*
* Ensure minimal processes are running on the PC to reduce risk of lost data
* values.
*
****************************************************************************/
void daq::acquire2(void) {
	uint32_t	i;
	FILE 		*fp;
	int32_t 	ok;
	int16_t		ch;
	uint32_t	nPreviousValues = 0;
	//double		startTime = 0.0;
	uint32_t	appBufferSize = (int)(NUM_STREAMING_SAMPLES * 1.5);
	uint32_t	overviewBufferSize = BUFFER_SIZE_STREAMING;
	uint32_t	sample_count;

	printf("Collect streaming...\n");
	printf("Data is written to disk file (fast_streaming_trig_data2.txt)\n");
	printf("Press a key to start\n");
	_getch();

	set_defaults();

	// Simple trigger, 500mV, rising
	ok = ps2000_set_trigger(unitOpened.handle, PS2000_CHANNEL_A,
		mv_to_adc(500, unitOpened.channelSettings[PS2000_CHANNEL_A].range), PS2000_RISING, 0, 0);

	unitOpened.trigger.advanced.autoStop = 0;
	unitOpened.trigger.advanced.totalSamples = 0;
	unitOpened.trigger.advanced.triggered = 0;

	//Reset global values
	g_nValues = 0;
	g_triggered = 0;
	g_triggeredAt = 0;
	g_startIndex = 0;
	g_prevStartIndex = 0;
	g_appBufferFull = 0;

	bufferInfo.unit = unitOpened;

	// Allocate memory for data arrays

	// Max A buffer at index 0, min buffer at index 1
	bufferInfo.appBuffers[PS2000_CHANNEL_A * 2] = (int16_t *)calloc(appBufferSize, sizeof(int16_t));
	bufferInfo.bufferSizes[PS2000_CHANNEL_A * 2] = appBufferSize;

	if (unitOpened.channelSettings[PS2000_CHANNEL_B].enabled) {
		// Max B buffer at index 2, min buffer at index 3
		bufferInfo.appBuffers[PS2000_CHANNEL_B * 2] = (int16_t *)calloc(appBufferSize, sizeof(int16_t));
		bufferInfo.bufferSizes[PS2000_CHANNEL_B * 2] = appBufferSize;
	}

	/* Collect data at 10us intervals
	* 100000 points with an aggregation of 100 : 1
	*	Auto stop after the 100000 samples
	*  Start it collecting,
	*/
	//ok = ps2000_run_streaming_ns ( unitOpened.handle, 10, PS2000_US, NUM_STREAMING_SAMPLES, 1, 100, overviewBufferSize );

	/* Collect data at 1us intervals
	* 1000000 points after trigger with 0 aggregation
	* Auto stop after the 1000000 samples
	* Start it collecting,
	* NOTE: The actual sampling interval used by the driver might not be that which is specified below. Use the sampling intervals
	* returned by the ps2000_get_timebase function to work out the most appropriate sampling interval to use. As these are low memory
	* devices, the fastest sampling intervals may result in lost data.
	*/
	ok = ps2000_run_streaming_ns(unitOpened.handle, 1, PS2000_US, NUM_STREAMING_SAMPLES, 1, 1, overviewBufferSize); // No aggregation

	printf("OK: %d\n", ok);

	/* From here on, we can get data whenever we want...*/

	while (!_kbhit() && !unitOpened.trigger.advanced.autoStop && !g_appBufferFull) {
		ps2000_get_streaming_last_values(unitOpened.handle, &daq::ps2000FastStreamingReady2);

		if (nPreviousValues != unitOpened.trigger.advanced.totalSamples) {
			sample_count = unitOpened.trigger.advanced.totalSamples - nPreviousValues;

			//Printing to console can take up resources
			//printf ("Values collected: %ld, Total samples: %ld ", sample_count, unitOpened.trigger.advanced.totalSamples);

			/*if(g_triggered)
			{
			printf("Triggered at index: %lu, overall %lu", g_triggeredAt, nPreviousValues + g_triggeredAt);
			}*/

			nPreviousValues = unitOpened.trigger.advanced.totalSamples;
			//printf("\n");

			if (g_appBufferFull) {
				unitOpened.trigger.advanced.totalSamples = appBufferSize;
				printf("\nApplication buffer full - stopping data collection.\n");
			}
		}
	}

	ps2000_stop(unitOpened.handle);

	printf("\nCollected %lu samples. Writing to file...\n", unitOpened.trigger.advanced.totalSamples);

	fopen_s(&fp, "fast_streaming_trig_data2.txt", "w");

	fprintf(fp, "For each of the %d Channels, results shown are....\n", unitOpened.noOfChannels);
	fprintf(fp, "Channel ADC Count & mV\n\n");

	for (ch = 0; ch < unitOpened.noOfChannels; ch++) {
		if (unitOpened.channelSettings[ch].enabled) {
			fprintf(fp, "Ch%C   Max ADC    Max mV   ", (char)('A' + ch));
		}
	}

	fprintf(fp, "\n");

	for (i = 0; i < unitOpened.trigger.advanced.totalSamples; i++) {
		if (fp != NULL) {
			for (ch = 0; ch < unitOpened.noOfChannels; ch++) {
				if (unitOpened.channelSettings[ch].enabled) {
					fprintf(fp, "%4C, %7d, %7d, ",
						'A' + ch,
						bufferInfo.appBuffers[ch * 2][i],
						adc_to_mv(bufferInfo.appBuffers[ch * 2][i], unitOpened.channelSettings[ch].range));
				}
			}

			fprintf(fp, "\n");
		} else {
			printf("Cannot open the file fast_streaming_trig_data2.txt for writing.\n");
		}
	}

	printf("Writing to file complete.\n");

	fclose(fp);

	// Free buffers
	for (ch = 0; ch < unitOpened.noOfChannels; ch++) {
		if (unitOpened.channelSettings[ch].enabled) {
			free(bufferInfo.appBuffers[ch * 2]);
		}
	}

	if (_kbhit()) {
		_getch();
	}
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

/****************************************************************************
*
* Streaming callback
*
* This demonstrates how to copy data to application buffers
*
****************************************************************************/
void  __stdcall daq::ps2000FastStreamingReady2(
	int16_t	**overviewBuffers,
	int16_t		overflow,
	uint32_t	triggeredAt,
	int16_t		triggered,
	int16_t		auto_stop,
	uint32_t	nValues
) {
	int16_t channel = 0;

	unitOpened.trigger.advanced.totalSamples += nValues;
	unitOpened.trigger.advanced.autoStop = auto_stop;

	g_triggered = triggered;
	g_triggeredAt = triggeredAt;

	g_overflow = overflow;

	if (nValues > 0 && g_appBufferFull == 0) {
		for (channel = (int16_t)PS2000_CHANNEL_A; channel < DUAL_SCOPE; channel++) {
			if (bufferInfo.unit.channelSettings[channel].enabled) {
				if (unitOpened.trigger.advanced.totalSamples <= bufferInfo.bufferSizes[channel * 2] && !g_appBufferFull) {
					g_nValues = nValues;
				} else if (g_startIndex < bufferInfo.bufferSizes[channel * 2]) {
					g_nValues = bufferInfo.bufferSizes[channel * 2] - (g_startIndex + 1);			// Only copy data into application buffer up to end
					unitOpened.trigger.advanced.totalSamples = bufferInfo.bufferSizes[channel * 2];	// Total samples limited to application buffer
					g_appBufferFull = 1;
				} else {
					// g_startIndex might be >= buffer length
					g_nValues = 0;
					unitOpened.trigger.advanced.totalSamples = bufferInfo.bufferSizes[channel * 2];
					g_appBufferFull = 1;
				}

				// Copy data...

				// Max buffers
				if (overviewBuffers[channel * 2] && bufferInfo.appBuffers[channel * 2]) {
					memcpy_s(
						(void *)(bufferInfo.appBuffers[channel * 2] + g_startIndex),
						g_nValues * sizeof(int16_t),
						(void *)(overviewBuffers[channel * 2]),
						g_nValues * sizeof(int16_t)
					);
				}

				// Min buffers
				if (overviewBuffers[channel * 2 + 1] && bufferInfo.appBuffers[channel * 2 + 1]) {
					memcpy_s(
						(void *)(bufferInfo.appBuffers[channel * 2 + 1] + g_startIndex),
						g_nValues * sizeof(int16_t),
						(void *)(overviewBuffers[channel * 2 + 1]), g_nValues * sizeof(int16_t)
					);
				}
			}
		}

		g_prevStartIndex = g_startIndex;
		g_startIndex = unitOpened.trigger.advanced.totalSamples;
	}
}

bool daq::connect() {
	if (!unitOpened.handle) {
		unitOpened.handle = ps2000_open_unit();
		
		get_info();

		timebase = 0;

		if (!unitOpened.handle) {
			return false;
		} else {
			return true;
		}
	} else {
		return true;
	}
}

bool daq::disconnect() {
	if (unitOpened.handle) {
		ps2000_close_unit(unitOpened.handle);
		unitOpened.handle = NULL;
	}
	return false;
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
				unitOpened.hasAdvancedTriggering = FALSE;
				unitOpened.hasSignalGenerator = FALSE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = FALSE;
				break;

			case MODEL_PS2105:
				unitOpened.model = MODEL_PS2105;
				unitOpened.firstRange = PS2000_100MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2105_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 1;
				unitOpened.hasAdvancedTriggering = FALSE;
				unitOpened.hasSignalGenerator = FALSE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = FALSE;
				break;

			case MODEL_PS2202:
				unitOpened.model = MODEL_PS2202;
				unitOpened.firstRange = PS2000_100MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2200_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = FALSE;
				unitOpened.hasSignalGenerator = FALSE;
				unitOpened.hasEts = FALSE;
				unitOpened.hasFastStreaming = FALSE;
				break;

			case MODEL_PS2203:
				unitOpened.model = MODEL_PS2203;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = TRUE;
				unitOpened.hasSignalGenerator = TRUE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = TRUE;
				break;

			case MODEL_PS2204:
				unitOpened.model = MODEL_PS2204;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = TRUE;
				unitOpened.hasSignalGenerator = TRUE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = TRUE;
				break;

			case MODEL_PS2204A:
				unitOpened.model = MODEL_PS2204A;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = DUAL_SCOPE;
				unitOpened.hasAdvancedTriggering = TRUE;
				unitOpened.hasSignalGenerator = TRUE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = TRUE;
				unitOpened.awgBufferSize = 4096;
				break;

			case MODEL_PS2205:
				unitOpened.model = MODEL_PS2205;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = 2;
				unitOpened.hasAdvancedTriggering = TRUE;
				unitOpened.hasSignalGenerator = TRUE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = TRUE;
				break;

			case MODEL_PS2205A:
				unitOpened.model = MODEL_PS2205A;
				unitOpened.firstRange = PS2000_50MV;
				unitOpened.lastRange = PS2000_20V;
				unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
				unitOpened.timebases = unitOpened.maxTimebase;
				unitOpened.noOfChannels = DUAL_SCOPE;
				unitOpened.hasAdvancedTriggering = TRUE;
				unitOpened.hasSignalGenerator = TRUE;
				unitOpened.hasEts = TRUE;
				unitOpened.hasFastStreaming = TRUE;
				unitOpened.awgBufferSize = 4096;
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

void daq::set_sig_gen() {
	int16_t waveform = 0;
	int32_t frequency = 1000;

	/*
	printf("Enter frequency in Hz: "); // Ask user to enter signal frequency;
	do {
		scanf_s("%lu", &frequency);
	} while (frequency < 1000 || frequency > PS2000_MAX_SIGGEN_FREQ);

	printf("Signal generator On");
	printf("Enter type of waveform (0..9 or 99)\n");
	printf("0:\tSINE\n");
	printf("1:\tSQUARE\n");
	printf("2:\tTRIANGLE\n");
	printf("3:\tRAMP UP\n");
	printf("4:\tRAMP DOWN\n");

	do {
		scanf_s("%hd", &waveform);
	} while (waveform < 0 || waveform >= PS2000_DC_VOLTAGE);
	*/

	ps2000_set_sig_gen_built_in(
		unitOpened.handle,				// handle of the oscilloscope
		0,								// offsetVoltage in microvolt
		1000000,						// peak to peak voltage in microvolt
		(PS2000_WAVE_TYPE)waveform,		// type of waveform
		(float)frequency,				// startFrequency in Hertz
		(float)frequency,				// stopFrequency in Hertz
		0,								// increment
		0,								// dwellTime, time in seconds between frequency changes in sweep mode
		PS2000_UPDOWN,					// sweepType
		0								// sweeps, number of times to sweep the frequency
	);
}