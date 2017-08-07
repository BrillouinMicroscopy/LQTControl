#include "kcubepiezo.h"

void kcubepiezo::connect() {
	PCC_Open(serialNo);
	// start the device polling at 200ms intervals
	PCC_StartPolling(serialNo, 200);
	// set default values
	setDefaults();
}

void kcubepiezo::disconnect() {
	// stop polling
	PCC_StopPolling(serialNo);
	PCC_Close(serialNo);
}

void kcubepiezo::enable() {
	PCC_Enable(serialNo);
}

void kcubepiezo::disable() {
	PCC_Disable(serialNo);
}

void kcubepiezo::setDefaults() {
	PCC_SetZero(serialNo);											// set zero reference voltage
	PCC_SetPositionControlMode(serialNo, defaultSettings.mode);		// set open loop mode
	PCC_SetMaxOutputVoltage(serialNo, defaultSettings.maxVoltage);	// set maximum output voltage
	PCC_SetVoltageSource(serialNo, defaultSettings.source);			// set voltage source
}

double kcubepiezo::getVoltage() {
	double maxVoltage = PCC_GetMaxOutputVoltage(serialNo);
	double relativeVoltage = PCC_GetOutputVoltage(serialNo);
	return maxVoltage * relativeVoltage / (pow(2, 15) - 1) / 10;
}

void kcubepiezo::setVoltage(double voltage) {
	double maxVoltage = PCC_GetMaxOutputVoltage(serialNo);
	double relativeVoltage = voltage / maxVoltage * (pow(2, 15) - 1) * 10;
	PCC_SetOutputVoltage(serialNo, (int)relativeVoltage);
}

void kcubepiezo::incrementVoltage(int direction) {
	int relativeVoltage = PCC_GetOutputVoltage(serialNo);
	PCC_SetOutputVoltage(serialNo, relativeVoltage + direction);
}
