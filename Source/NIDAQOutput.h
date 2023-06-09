/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Open Ephys

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

#ifndef __NIDAQOUTPUT_H_F7BDA585__
#define __NIDAQOUTPUT_H_F7BDA585__

#include <ProcessorHeaders.h>

#include "NIDAQComponents.h"

/**

    Provides an interface to control NIDAQ devices with output capabilities.

    @see GenericProcessor
 */
class NIDAQOutput : public GenericProcessor
{
public:

    /** Constructor */
    NIDAQOutput();

    /** Destructor */
    ~NIDAQOutput();

    /** Get a list of available devices */
    Array<NIDAQDevice*> getDevices();
    int getDeviceIndex() { return deviceIndex; };
    String getDeviceName() { return mNIDAQ->device->getName(); };

    /** Set the current device by name */
    void setDevice(String deviceName);

    /** Opens a connection to NIDAQ device */
    int openConnection();

    /** Flag whether an output is available */
    bool isOutputAvailable() { return outputAvailable; };

    /** Sets the voltage range of the data source. */
    void setVoltageRange(int rangeIndex);

    /** Get available sample rates for current device */
    Array<NIDAQ::float64> getSampleRates() { return mNIDAQ->sampleRates; };

    /** Get the current sample rate */
    NIDAQ::float64 getSampleRate() { return mNIDAQ->getSampleRate(); };
    int getSampleRateIndex() { return sampleRateIndex; };

    /** Sets the sample rate of the data source. */
    void setSampleRate(int rateIndex);

    /** Returns total number of available analog inputs on device */
	int getTotalAvailableAnalogOutputs() { return mNIDAQ->device->numAOChannels; }; 

	/** Returns total number of available digital inputs on device */
	int getTotalAvailableDigitalOutputs() { return mNIDAQ->device->numDOChannels; };

    /** Get the number of active analog outputs */
    int getNumActiveAnalogOutputs() { return mNIDAQ->getNumActiveAnalogOutputs(); };
	void setNumActiveAnalogChannels(int numChannels) { mNIDAQ->setNumActiveAnalogOutputs(numChannels); };

    /** Get the number of active digital outputs */
    int getNumActiveDigitalOutputs() { return mNIDAQ->getNumActiveDigitalOutputs(); };
	void setNumActiveDigitalChannels(int numChannels) { mNIDAQ->setNumActiveDigitalOutputs(numChannels); };

    /** TODO update buffers based on channel counts / active channels */
    void updateAnalogChannels();
	void updateDigitalChannels();

	/** Get/set digital write size */
	int getDigitalWriteSize() { return mNIDAQ->getDigitalWriteSize(); };
	void setDigitalWriteSize(int size) { mNIDAQ->setDigitalWriteSize(size); };

    // Returns the state of the digital ports to be used as input
	int getNumPorts() { return mNIDAQ->getNumPorts(); };
	bool getPortState(int portIdx) { return mNIDAQ->getPortState(portIdx); };
	void setPortState(int portIdx, bool state) { mNIDAQ->setPortState(portIdx, state); };

    int getTotalAvailableDigitalPorts() { return mNIDAQ->device->numDOPorts; };
    std::vector<int> getActiveDigitalPorts() { return mNIDAQ->getActiveDigitalPorts(); };

    SOURCE_TYPE getSourceTypeForOutput(int outputIndex) { return mNIDAQ->getSourceTypeForOutput(outputIndex); };

    /** Set Analog channel enabled state */
    void setAnalogEnable(int id, bool enabled) { mNIDAQ->aout[id]->setEnabled(enabled); };

    /** Set Digital channel enabled state */
    void setDigitalEnable(int id, bool enabled) { mNIDAQ->dout[id]->setEnabled(enabled); };

    /** Get the available output voltage ranges for this device */
    Array<SettingsRange> getVoltageRanges();

    /** Get the current voltage range index */
    int getVoltageRangeIndex() { return voltageRangeIndex; };

    /** Searches for events and triggers the NIDAQ output when appropriate. */
    void process (AudioBuffer<float>& buffer) override;

    /** Convenient interface for responding to incoming events. */
    void handleTTLEvent (TTLEventPtr event) override;

    /** Called when settings need to be updated. */
    void updateSettings() override;

    /** Called immediately at the start of data acquisition. */
    bool startAcquisition() override;

    /** Called immediately after the end of data acquisition. */
    bool stopAcquisition() override;

    /** Creates the NIDAQOutputEditor. */
    AudioProcessorEditor* createEditor() override;

private:

    /* Manages connected NIDAQ devices */
    ScopedPointer<NIDAQmxDeviceManager> dm;

    /* Flag any available devices */
    bool outputAvailable = false;

    /* Handle to current NIDAQ device */
    ScopedPointer<NIDAQmx> mNIDAQ;

    int deviceIndex = 0;
    int sampleRateIndex = 0;
    int voltageRangeIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NIDAQOutput);
};

#endif  // __NIDAQOUTPUT_H_F7BDA585__