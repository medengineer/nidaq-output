/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2019 Allen Institute for Brain Science and Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __NIDAQCOMPONENTS_H__
#define __NIDAQCOMPONENTS_H__

#include <ProcessorHeaders.h>
#include <stdio.h>
#include <string.h>

#include "nidaq-api/NIDAQmx.h"

#define NUM_SAMPLE_RATES 17

#define PORT_SIZE 8 //number of bits in a port
#define DEFAULT_NUM_ANALOG_OUTPUTS 1
#define DEFAULT_NUM_DIGITAL_OUTPUTS 8

#define ERR_BUFF_SIZE 2048

#define STR2CHR( jString ) ((jString).toUTF8())
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

class NIDAQmx;
class OutputChannel;
class AnalogOutput;
class DigitalOutput;

enum SOURCE_TYPE {
	RSE = 0,
	NRSE,
	DIFF,
	PSEUDO_DIFF
};

struct SettingsRange {
	NIDAQ::float64 min, max;
	SettingsRange() : min(0), max(0) {}
	SettingsRange(NIDAQ::float64 min_, NIDAQ::float64 max_)
		: min(min_), max(max_) {}
};

class OutputChannel
{
public:
	OutputChannel(String name_) : name(name_) {};
	OutputChannel() : name("") {};
	~OutputChannel() {};

	String getName() { return name; }

	// Defines if the channel is available for use
	void setAvailable(bool available_) { available = available_; }
	bool isAvailable() { return available; }

	// Defines if the channel is enabled for use
	void setEnabled(bool enabled_) { enabled = enabled_; }
	bool isEnabled() { return enabled; }

private:

	String name;
	bool available = false;
	bool enabled = false;
};

class AnalogOutput : public OutputChannel
{

public:
	AnalogOutput(String name, NIDAQ::int32 terminalConfig);
	AnalogOutput() : OutputChannel() {};
	~AnalogOutput() {};

	SOURCE_TYPE getSourceType() { return sourceTypes[sourceTypeIndex]; }
	void setNextSourceType() { sourceTypeIndex = (sourceTypeIndex + 1) % sourceTypes.size(); }

private:
	int sourceTypeIndex = 0;
	Array<SOURCE_TYPE> sourceTypes;
};

class NIDAQDevice
{

public:

	NIDAQDevice(String name_) : name(name_) {};
	NIDAQDevice() {};
	~NIDAQDevice() {};

	String getName() { return name; }

	String productName;

	NIDAQ::int32 deviceCategory;
	NIDAQ::uInt32 productNum;
	NIDAQ::uInt32 serialNum;
	NIDAQ::uInt32 numAOChannels;
	NIDAQ::uInt32 numDOChannels;

	int digitalWriteSize;

	SettingsRange sampleRateRange;

	bool isUSBDevice;

	Array<SettingsRange> voltageRanges;
	Array<NIDAQ::float64> adcResolutions;

private:

	String name;

};

class NIDAQmxDeviceManager
{
public:

	NIDAQmxDeviceManager() {};
	~NIDAQmxDeviceManager() {};

	void scanForDevices();

	int getNumAvailableDevices() { return devices.size(); }

	int getDeviceIndexFromName(String deviceName);
	NIDAQDevice* getDeviceAtIndex(int index) { return devices[index]; }

	NIDAQDevice* getDeviceFromName(String deviceName);

	friend class NIDAQThread;

private:

	OwnedArray<NIDAQDevice> devices;
	int activeDeviceIndex;
};

class NIDAQmx : public Thread
{
public:

	NIDAQmx(NIDAQDevice* device_);
	~NIDAQmx() {};

	/* Pointer to the active device */
	NIDAQDevice* device;

	/* Connects to the active device */
	void connect(); 

	/* Unique device properties */
	String getProductName() { return device->productName; };
	String getSerialNumber() { return String(device->serialNum); };

	/* Analog configuration */
	NIDAQ::float64 getSampleRate() { return sampleRates[sampleRateIndex]; };
	void setSampleRate(int index) { sampleRateIndex = index; };

	SettingsRange getVoltageRange() { return device->voltageRanges[voltageRangeIndex]; };
	void setVoltageRange(int index) { voltageRangeIndex = index; };

	SOURCE_TYPE getSourceTypeForOuput(int analogOutputIndex) { return aout[analogOutputIndex]->getSourceType(); };
	void toggleSourceType(int analogOutputIndex) { aout[analogOutputIndex]->setNextSourceType(); }

	void setNumActiveAnalogOutputs(int numActiveAnalogOutputs_) { numActiveAnalogOutputs = numActiveAnalogOutputs_; };
	int getNumActiveAnalogOutputs() { return numActiveAnalogOutputs; };

	void setNumActiveDigitalOutputs(int numActiveDigitalOutputs_) { numActiveDigitalOutputs = numActiveDigitalOutputs_; };
	int getNumActiveDigitalOutputs() { return numActiveDigitalOutputs; };

	/* 32-bit mask indicating which lines are currently enabled */
	uint32 getActiveDigitalLines();

	void run() override { /* TODO: Buffer analog output data and send in chunks to avoid blocking the UI thread */ };

	Array<NIDAQ::float64> sampleRates;

	OwnedArray<AnalogOutput> 	aout;
	OwnedArray<OutputChannel> 	dout;

	friend class NIDAQThread;

private:

	/* Manages connected NIDAQ devices */
	ScopedPointer<NIDAQmxDeviceManager> dm;

	int deviceIndex = 0;
	int sampleRateIndex = 0;
	int voltageRangeIndex = 0;

	int digitalWriteSize = 0;

	int numActiveAnalogOutputs = DEFAULT_NUM_ANALOG_OUTPUTS; //8
	int numActiveDigitalOutputs = DEFAULT_NUM_DIGITAL_OUTPUTS; //8

	HeapBlock<NIDAQ::float64> aout_data;

	HeapBlock<NIDAQ::uInt32> eventCodes;

	int64 aout_timestamp;
	uint64 eventCode;

	std::map<int,int> digitalLineMap;

};

#endif  // __NIDAQCOMPONENTS_H__