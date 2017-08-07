#ifndef KCUBEPIEZO_H
#define KCUBEPIEZO_H

#include <Thorlabs.MotionControl.KCube.Piezo.h>
#include <cmath>

typedef struct {
	short maxVoltage = 750;										// maximum output voltage
	PZ_InputSourceFlags source = PZ_InputSourceFlags::PZ_All;	// voltage input source
	PZ_ControlModeTypes mode = PZ_ControlModeTypes::PZ_OpenLoop;// mode type
} DEFAULT_SETTINGS;

class kcubepiezo {

public:
	void connect();
	void disconnect();
	void enable();
	void disable();
	void setDefaults();
	double getVoltage();
	void setVoltage(double voltage);

	double outputVoltage = 0.0;

	char const * serialNo = "29501039";	// serial number of the KCube Piezo device (can be found in Kinesis) TODO: make this a changeable parameter

	DEFAULT_SETTINGS defaultSettings;

};

#endif // KCUBEPIEZO_H