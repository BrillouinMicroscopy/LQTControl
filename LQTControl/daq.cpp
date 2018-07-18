#include "daq_PS2000.h"
#include <QtWidgets>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include "daq_PS2000.h"

daq::daq(QObject *parent) :
	QObject(parent) {
}

daq::daq(QObject *parent, std::vector<int32_t> ranges) :
	QObject(parent), m_input_ranges(ranges) {
}

void daq::startStopAcquisition() {
	if (timer->isActive()) {
		timer->stop();
		m_acquisitionRunning = false;
	} else {
		setAcquisitionParameters();
		timer->start(20);
		m_acquisitionRunning = true;
	}
	emit(s_acquisitionRunning(m_acquisitionRunning));
}

void daq::setSampleRate(int index) {
	acquisitionParameters.timebase = index;
	setAcquisitionParameters();
}

void daq::setNumberSamples(int32_t no_of_samples) {
	acquisitionParameters.no_of_samples = no_of_samples;
	setAcquisitionParameters();
}

void daq::setCoupling(int coupling, int ch) {
	acquisitionParameters.channelSettings[ch].coupling = coupling;
	m_unitOpened.channelSettings[ch].coupling = coupling;
	set_defaults();
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
	set_defaults();
}

void daq::getBlockData() {
	std::array<std::vector<int32_t>, DAQ_MAX_CHANNELS> values = collectBlockData();

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

int32_t daq::adc_to_mv(int32_t raw, int32_t ch) {
	return (m_scale_to_mv) ? (raw * m_input_ranges[ch]) / 32767 : raw;
}

int16_t daq::mv_to_adc(int16_t mv, int16_t ch) {
	return ((mv * 32767) / m_input_ranges[ch]);
}

void daq::init() {
	// create timers and connect their signals
	// after moving daq_PS2000 to another thread
	timer = new QTimer();
	QWidget::connect(timer, SIGNAL(timeout()), this, SLOT(getBlockData()));
}