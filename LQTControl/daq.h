#ifndef DAQ_H
#define DAQ_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWidgets>
#include <vector>
#include <array>
#include <chrono>
#include <ctime>

#include <gsl/gsl>
#include "LQT.h"
#include "circularBuffer.h"
#include "generalmath.h"

#define DAQ_BUFFER_SIZE 	8000
#define SINGLE_CH_SCOPE 1				// Single channel scope
#define DUAL_SCOPE 2					// Dual channel scope

#define DAQ_MAX_CHANNELS 4

typedef enum enPSCoupling {
	PS_AC,
	PS_DC
} PS_COUPLING;

typedef enum class PSTypes {
	MODEL_PS2000 = 0,
	MODEL_PS2000A = 1
} PS_TYPES;

typedef struct {
	int coupling = PS_DC;
	int16_t range{ 0 };
	bool enabled = false;
	int16_t values[DAQ_BUFFER_SIZE]{ 0 };
} CHANNEL_SETTINGS;

typedef struct {
	int16_t			handle{ 0 };
	int				model{ 0 };
	int				firstRange{ 0 };
	int				lastRange{ 0 };
	int16_t			maxTimebase{ 0 };
	int16_t			timebases{ 0 };
	int16_t			noOfChannels{ 0 };
	CHANNEL_SETTINGS channelSettings[DAQ_MAX_CHANNELS];
	int16_t			hasAdvancedTriggering{ 0 };
	int16_t			hasFastStreaming{ 0 };
	int16_t			hasEts{ 0 };
	int16_t			hasSignalGenerator{ 0 };
	int16_t			awgBufferSize{ 0 };
	int16_t			bufferSize{ 0 };
} UNIT_MODEL;

typedef struct {
	int coupling = PS_DC;
	int16_t range{ 0 };
	bool enabled = false;
} DEFAULT_CHANNEL_SETTINGS;

typedef struct {
	int16_t 	auto_trigger_ms{ 0 };
	int32_t 	time_interval{ 0 };
	int16_t 	time_units{ 0 };
	int16_t 	oversample{ 0 };
	uint32_t 	no_of_samples{ 1000 };
	int32_t 	max_samples{ 0 };
	int32_t 	time_indisposed_ms{ 0 };
	int16_t		timebase{ 10 };
	DEFAULT_CHANNEL_SETTINGS channelSettings[2] = {
		{PS_DC, 4, true},
		{PS_DC, 5, true}
	};
} ACQUISITION_PARAMETERS;

class daq : public QObject {
	Q_OBJECT

	public:
		explicit daq(QObject *parent);
		explicit daq(QObject *parent, std::vector<int32_t> ranges);
		virtual void setAcquisitionParameters() = 0;
		virtual std::array<std::vector<int32_t>, DAQ_MAX_CHANNELS> collectBlockData() = 0;

		void setSampleRate(int index);
		void setCoupling(int coupling, int ch);
		void setRange(int index, int ch);
		void setNumberSamples(int32_t no_of_samples);

		CircularBuffer<int16_t> *m_liveBuffer = new CircularBuffer<int16_t>(4, DAQ_MAX_CHANNELS, 8000);

		std::vector<int32_t> m_input_ranges;

		std::vector<std::string> PS_NAMES = { "PS2000", "PS2000A" };

	public slots:
		virtual void connect_daq() = 0;
		virtual void disconnect_daq() = 0;

		void init();
		void startStopAcquisition();

	protected slots:
		void getBlockData();

	signals:
		void s_acquisitionRunning(bool);
		void connected(bool);
		void acquisitionParametersChanged(ACQUISITION_PARAMETERS);
		void collectedBlockData();

	protected:
		virtual void set_defaults(void) = 0;
		virtual void get_info(void) = 0;

		UNIT_MODEL m_unitOpened;
		bool m_isConnected = false;
		bool m_acquisitionRunning = false;
		int32_t adc_to_mv(int32_t raw, int32_t ch);
		int16_t mv_to_adc(int16_t mv, int16_t ch);
		QTimer *timer = nullptr;
		ACQUISITION_PARAMETERS acquisitionParameters;
		int16_t m_overflow = 0;
		bool m_scale_to_mv = true;
};

#endif // DAQ_H