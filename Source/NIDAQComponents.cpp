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

#include <chrono>
#include <math.h>

#include "NIDAQComponents.h"

AnalogOutput::AnalogOutput(String name, NIDAQ::int32 termCfgs) : OutputChannel(name)
{

	sourceTypes.clear();

	if (termCfgs & DAQmx_Val_Bit_TermCfg_RSE)
		sourceTypes.add(SOURCE_TYPE::RSE);
	
	if (termCfgs & DAQmx_Val_Bit_TermCfg_NRSE)
		sourceTypes.add(SOURCE_TYPE::NRSE);

	if (termCfgs & DAQmx_Val_Bit_TermCfg_Diff)
		sourceTypes.add(SOURCE_TYPE::DIFF);

	if (termCfgs & DAQmx_Val_Bit_TermCfg_PseudoDIFF)
		sourceTypes.add(SOURCE_TYPE::PSEUDO_DIFF);

}

static int32 GetTerminalNameWithDevPrefix(NIDAQ::TaskHandle taskHandle, const char terminalName[], char triggerName[]);

static int32 GetTerminalNameWithDevPrefix(NIDAQ::TaskHandle taskHandle, const char terminalName[], char triggerName[])
{

	NIDAQ::int32	error = 0;
	char			device[256];
	NIDAQ::int32	productCategory;
	NIDAQ::uInt32	numDevices, i = 1;

	DAQmxErrChk(NIDAQ::DAQmxGetTaskNumDevices(taskHandle, &numDevices));
	while (i <= numDevices) {
		DAQmxErrChk(NIDAQ::DAQmxGetNthTaskDevice(taskHandle, i++, device, 256));
		DAQmxErrChk(NIDAQ::DAQmxGetDevProductCategory(device, &productCategory));
		if (productCategory != DAQmx_Val_CSeriesModule && productCategory != DAQmx_Val_SCXIModule) {
			*triggerName++ = '/';
			strcat(strcat(strcpy(triggerName, device), "/"), terminalName);
			break;
		}
	}

Error:
	return error;
}

void NIDAQmxDeviceManager::scanForDevices()
{

	devices.clear();

	char data[2048] = { 0 };
	NIDAQ::DAQmxGetSysDevNames(data, sizeof(data));

	StringArray deviceList; 
	deviceList.addTokens(&data[0], ", ", "\"");

	StringArray deviceNames;
	StringArray productList;

	for (int i = 0; i < deviceList.size(); i++)
	{
		if (deviceList[i].length() > 0)
		{
			String deviceName = deviceList[i].toUTF8();

			/* Get product name */
			char pname[2048] = { 0 };
			NIDAQ::DAQmxGetDevProductType(STR2CHR(deviceName), &pname[0], sizeof(pname));
			devices.add(new NIDAQDevice(deviceName));
			devices.getLast()->productName = String(&pname[0]);
		}
	}

	if (!devices.size())
		devices.add(new NIDAQDevice("Simulated")); //TODO: Make SimulatedClass derived from NIDAQDevice

}

int NIDAQmxDeviceManager::getDeviceIndexFromName(String name)
{

	for (int i = 0; i < devices.size(); i++)
		if (devices[i]->getName() == name)
			return i;

	return -1;

}

NIDAQmx::NIDAQmx(NIDAQDevice* device_) 
: Thread("NIDAQmx-" + String(device_->getName())), device(device_)
{

	connect();

	digitalWriteSize = device->digitalWriteSize;

	// Pre-define reasonable sample rates
	float sample_rates[NUM_SAMPLE_RATES] = {
		1000.0f, 1250.0f, 1500.0f,
		2000.0f, 2500.0f,
		3000.0f, 3330.0f,
		4000.0f,
		5000.0f,
		6250.0f,
		8000.0f,
		10000.0f,
		12500.0f,
		15000.0f,
		20000.0f,
		25000.0f,
		30000.0f
	};

	sampleRates.clear();

	int idx = 0;
	while (sample_rates[idx] <= device->sampleRateRange.max && idx < NUM_SAMPLE_RATES)
		sampleRates.add(sample_rates[idx++]);

	// Default to highest sample rate
	sampleRateIndex = sampleRates.size() - 1;

	// Default to largest voltage range
	voltageRangeIndex = device->voltageRanges.size() - 1;

}

void NIDAQmx::connect()
{

	String deviceName = device->getName();

	if (deviceName == "SimulatedDevice")
	{

		device->isUSBDevice = false;
		device->voltageRanges.add(SettingsRange(-10.0f, 10.0f));
		device->productName = String("No Device Detected");

	}
	else
	{

		/* Get category type */
		NIDAQ::DAQmxGetDevProductCategory(STR2CHR(deviceName), &device->deviceCategory);
		LOGD("Product Category: ", device->deviceCategory);

		device->isUSBDevice = device->productName.contains("USB");

		//device->digitalReadSize = device->isUSBDevice ? 32 : 8;

		NIDAQ::DAQmxGetDevProductNum(STR2CHR(deviceName), &device->productNum);
		LOGD("Product Num: ", device->productNum);

		NIDAQ::DAQmxGetDevSerialNum(STR2CHR(deviceName), &device->serialNum);
		LOGD("Serial Num: ", device->serialNum);

		/*
		// Get simultaneous sampling supported
		NIDAQ::bool32 supported = false;
		NIDAQ::DAQmxGetDevAISimultaneousSamplingSupported(STR2CHR(deviceName), &supported);
		device->simAISamplingSupported = supported;
		LOGD("Simultaneous sampling supported: ", supported ? "YES" : "NO");
		*/

		// Get device sample rates

		/*
		NIDAQ::float64 smin;
		NIDAQ::DAQmxGetDevAOMinRate(STR2CHR(deviceName), &smin);
		LOGD("Min sample rate: ", smin);

		NIDAQ::float64 smaxs;
		NIDAQ::DAQmxGetDevAOMaxSingleChanRate(STR2CHR(deviceName), &smaxs);
		LOGD("Max single channel sample rate: ", smaxs);

		NIDAQ::float64 smaxm;
		NIDAQ::DAQmxGetDevAOMaxMultiChanRate(STR2CHR(deviceName), &smaxm);
		LOGD("Max multi channel sample rate: ", smaxm);
		*/

		NIDAQ::float64 data[512];
		NIDAQ::DAQmxGetDevAOVoltageRngs(STR2CHR(deviceName), &data[0], sizeof(data));

		// Get available voltage ranges
		device->voltageRanges.clear();
		LOGD("Detected voltage ranges: \n");
		for (int i = 0; i < 512; i += 2)
		{
			NIDAQ::float64 vmin = data[i];
			NIDAQ::float64 vmax = data[i + 1];
			if (vmin == vmax || abs(vmin) < 1e-10 || vmax < 1e-2)
				break;
			device->voltageRanges.add(SettingsRange(vmin, vmax));
		}

		NIDAQ::int32	error = 0;
		char			errBuff[ERR_BUFF_SIZE] = { '\0' };

		char ai_channel_data[2048];
		//NIDAQ::DAQmxGetDevAIPhysicalChans(STR2CHR(device->getName()), &ai_channel_data[0], sizeof(ai_channel_data));
		NIDAQ::DAQmxGetDevAOPhysicalChans(STR2CHR(device->getName()), &ai_channel_data[0], sizeof(ai_channel_data));

		LOGD(ai_channel_data);

		StringArray channel_list;
		channel_list.addTokens(&ai_channel_data[0], ", ");

		device->numAOChannels = 0;
		aout.clear();

		LOGD("Detected ", channel_list.size(), " analog output channels");

		for (int i = 0; i < channel_list.size(); i++)
		{

			if (channel_list[i].length() > 0)
			{
				/* Get channel termination */
				NIDAQ::int32 termCfgs;
				NIDAQ::DAQmxGetPhysicalChanAOTermCfgs(channel_list[i].toUTF8(), &termCfgs);

				String name = channel_list[i].toRawUTF8();

				String terminalConfigurations = String::toHexString(termCfgs);

				aout.add(new AnalogOutput(name, termCfgs));

				if (device->numAOChannels++ <= numActiveAnalogOutputs)
				{
					aout.getLast()->setAvailable(true);
					aout.getLast()->setEnabled(true);
				}

				LOGD("Adding analog input channel: ", name, " with terminal config: ", " (", termCfgs, ") enabled: ", aout.getLast()->isEnabled() ? "YES" : "NO");
				
			}
		}

		// Get ADC resolution for each voltage range (throwing error as is)
		NIDAQ::TaskHandle adcResolutionQuery;

		NIDAQ::DAQmxCreateTask("ADCResolutionQuery", &adcResolutionQuery);

		SettingsRange vRange;

		for (int i = 0; i < device->voltageRanges.size(); i++)
		{
			vRange = device->voltageRanges[i];

			DAQmxErrChk(NIDAQ::DAQmxCreateAOVoltageChan(
				adcResolutionQuery,			//task handle
				STR2CHR(aout[i]->getName()),	    //NIDAQ physical channel name (e.g. dev1/ai1)
				"",							//user-defined channel name (optional)
				//DAQmx_Val_Cfg_Default,		//input terminal configuration
				vRange.min,					//min input voltage
				vRange.max,					//max input voltage
				DAQmx_Val_Volts,			//voltage units
				NULL));

			NIDAQ::float64 adcResolution;
			DAQmxErrChk(NIDAQ::DAQmxGetAOResolution(adcResolutionQuery, STR2CHR(aout[i]->getName()), &adcResolution));

			//TODO: DAQmxGetAOResolutionUnits/ DAQmxSetAOResolutionUnits

			device->adcResolutions.add(adcResolution);

		}

		NIDAQ::DAQmxStopTask(adcResolutionQuery);
		NIDAQ::DAQmxClearTask(adcResolutionQuery);

		// Get Digital Output Channels

		char di_channel_data[2048];
		//NIDAQ::DAQmxGetDevTerminals(STR2CHR(deviceName), &data[0], sizeof(data)); //gets all terminals
		//NIDAQ::DAQmxGetDevDIPorts(STR2CHR(deviceName), &data[0], sizeof(data));	//gets line name
		NIDAQ::DAQmxGetDevDOLines(STR2CHR(deviceName), &di_channel_data[0], sizeof(di_channel_data));	//gets ports on line

		channel_list.clear();
		channel_list.addTokens(&di_channel_data[0], ", ", "\"");

		device->numDOChannels = 0;
		dout.clear();

		for (int i = 0; i < channel_list.size(); i++)
		{
			StringArray channel_type;
			channel_type.addTokens(channel_list[i], "/", "\"");
			if (channel_list[i].length() > 0)
			{
				String name = channel_list[i].toRawUTF8();

				//LOGD("Found digital line: ", name);

				dout.add(new OutputChannel(name));

				dout.getLast()->setAvailable(true);
				if (device->numDOChannels++ < numActiveDigitalOutputs)
					dout.getLast()->setEnabled(true);
			}
		}

		LOGD("Found ", dout.size(), " digital output channels");

		NIDAQ::int32					di_read = 0;
		static int						totalDIRead = 0;

		// Start a digital task on port 2

		// Set sample rate range
		//NIDAQ::float64 smax = smaxm;
		/*
		if (!device->simAISamplingSupported)
			smax /= numActiveAnalogOutputs;
		*/

		//device->sampleRateRange = SettingsRange(smin, smax);

Error:

		if (DAQmxFailed(error))
			NIDAQ::DAQmxGetExtendedErrorInfo(errBuff, ERR_BUFF_SIZE);

		if (adcResolutionQuery != 0) {
			// DAQmx Stop Code
			NIDAQ::DAQmxStopTask(adcResolutionQuery);
			NIDAQ::DAQmxClearTask(adcResolutionQuery);
		}

		if (DAQmxFailed(error))
			LOGE("DAQmx Error: ", errBuff);
		fflush(stdout);

		return;

	}

}

void NIDAQmx::startTasks()
{

	NIDAQ::int32	error = 0;
	char			errBuff[ERR_BUFF_SIZE] = { '\0' };

	clearTasks();

	char ports[2048];
	NIDAQ::DAQmxGetDevDOPorts(STR2CHR(device->getName()), &ports[0], sizeof(ports));

	LOGD("Got ports: ", ports);

	StringArray port_list;
	port_list.addTokens(&ports[0], ", ", "\"");

	int portIdx = 0;
	for (auto& port : port_list)
	{

		if (port.length() && (portIdx*PORT_SIZE < dout.size()) && portIdx == 1) //hard-coded to port index 1 for now...
		{

			NIDAQ::TaskHandle taskHandleDO = 0;
			/* Create a digital input task using device serial number to gurantee unique task name per device */
			if (device->isUSBDevice)
				DAQmxErrChk(NIDAQ::DAQmxCreateTask(STR2CHR("DOTask_USB"+getSerialNumber()+"port"+std::to_string(portIdx)), &taskHandleDO));
			else
				DAQmxErrChk(NIDAQ::DAQmxCreateTask(STR2CHR("DOTask_PXI"+getSerialNumber()+"port"+std::to_string(portIdx)), &taskHandleDO));

			LOGC("Creating task for port: ", port);

			/* Create a channel for each digital output port */
			DAQmxErrChk(NIDAQ::DAQmxCreateDOChan(
				taskHandleDO,
				STR2CHR(port),
				"",
				DAQmx_Val_ChanForAllLines)
			);

			taskHandlesDO.push_back(taskHandleDO);

		}

		if (port.length()) portIdx++;

	}


	for (NIDAQ::TaskHandle taskHandle : taskHandlesDO)
	{
		DAQmxErrChk(NIDAQ::DAQmxTaskControl(taskHandle, DAQmx_Val_Task_Commit));
		DAQmxErrChk(NIDAQ::DAQmxStartTask(taskHandle));
	}

Error:

	if (DAQmxFailed(error))
		NIDAQ::DAQmxGetExtendedErrorInfo(errBuff, ERR_BUFF_SIZE);

	if (DAQmxFailed(error))
		LOGE("DAQmx Error: ", errBuff);
	fflush(stdout);

	return;
}

void NIDAQmx::clearTasks()
{

	NIDAQ::int32	error = 0;
	char			errBuff[ERR_BUFF_SIZE] = { '\0' };

	if (taskHandlesDO.size() > 0)
	{
		for (auto& taskHandle : taskHandlesDO)
		{
			NIDAQ::DAQmxStopTask(taskHandle);
			NIDAQ::DAQmxClearTask(taskHandle);
		}
		taskHandlesDO.clear();
	}

Error:

	if (DAQmxFailed(error))
		NIDAQ::DAQmxGetExtendedErrorInfo(errBuff, ERR_BUFF_SIZE);

	if (DAQmxFailed(error))
		LOGE("DAQmx Error: ", errBuff);
	fflush(stdout);

	return;
}

void NIDAQmx::sendDigital(int channelIdx, bool state)
{

	NIDAQ::int32	error = 0;
	char			errBuff[ERR_BUFF_SIZE] = { '\0' };
	NIDAQ::int32 	write;
	NIDAQ::uInt8	data[1] = {0};

	if (channelIdx >= dout.size())
		return;

	if (dout[channelIdx]->isEnabled() && taskHandlesDO.size())
	{
		if (state) data[0] = 1 << channelIdx; //data[0] ^= 0xFF; //invert all bits
		DAQmxErrChk(NIDAQ::DAQmxWriteDigitalU8(taskHandlesDO[0], 1, 1, 10.0, DAQmx_Val_GroupByChannel, data, &write, nullptr));
	}

Error:

	if (DAQmxFailed(error))
		NIDAQ::DAQmxGetExtendedErrorInfo(errBuff, ERR_BUFF_SIZE);

	if (DAQmxFailed(error))
		LOGE("DAQmx Error: ", errBuff);
	fflush(stdout);

	return;

}
uint32 NIDAQmx::getActiveDigitalLines()
{
	if (!getNumActiveDigitalOutputs())
		return 0;

	uint32 linesEnabled = 0;
	for (int i = 0; i < dout.size(); i++)
	{
		if (dout[i]->isEnabled())
			linesEnabled += pow(2, i);
	}
	return linesEnabled;
}