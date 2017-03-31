#ifndef DAQ_H
#define DAQ_H

#include <QMainWindow>
#include <QtCore/QObject>
#include <QtWidgets>
#include <vector>

#include "ps2000.h"

#define BUFFER_SIZE 	1024
#define BUFFER_SIZE_STREAMING 50000		// Overview buffer size
#define NUM_STREAMING_SAMPLES 100000	// Number of streaming samples to collect
#define MAX_CHANNELS 4
#define SINGLE_CH_SCOPE 1				// Single channel scope
#define DUAL_SCOPE 2					// Dual channel scope

typedef struct {
	int32_t amplitude = 2e6;		// peak to peak voltage in microvolt
	int32_t offset = 0;				// 
	int16_t waveform = 3;			// type of waveform
	int32_t frequency = 100;		// frequency of the scan
	int32_t	steps = 100;			// number of steps for the manual scan
} SCAN_PARAMETERS;

typedef struct {
	std::vector<double> voltages;
	std::vector<double> intensity;
	std::vector<double> A1;
	std::vector<double> A2;
} SCAN_RESULTS;

class daq : public QObject {
	Q_OBJECT

	public:
		explicit daq(QObject *parent = 0);
		bool startStopAcquisition();
		void acquire();
		QVector<QPointF> getData();
		QVector<QPointF> getBuffer(int ch);
		bool connect();
		bool disconnect();
		void startStreaming();
		void collectStreamingData();
		void stopStreaming();
		void set_sig_gen();
		void set_trigger_advanced();
		SCAN_PARAMETERS getScanParameters();
		void setScanParameters(int type, int value);
		void scanManual();

	private slots:

	private:
		static void __stdcall ps2000FastStreamingReady2(
			int16_t **overviewBuffers,
			int16_t   overflow,
			uint32_t  triggeredAt,
			int16_t   triggered,
			int16_t   auto_stop,
			uint32_t  nValues
		);
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
		int colCount = 1024;
		QVector<QPointF> points;
		SCAN_PARAMETERS scanParameters;
		SCAN_RESULTS scanResults;
};

#endif // DAQ_H