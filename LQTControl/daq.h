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
		void collectedData();
		void collectedBlockData();
		void acquisitionParametersChanged(ACQUISITION_PARAMETERS);

	private:
		bool m_isConnected = false;
		bool m_acquisitionRunning = false;
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
		QElapsedTimer passTimer;
		QVector<QPointF> points;
		ACQUISITION_PARAMETERS acquisitionParameters;
};

#endif // DAQ_H