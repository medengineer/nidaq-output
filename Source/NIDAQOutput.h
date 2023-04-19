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

    // Get a list of available devices
	Array<NIDAQDevice*> getDevices();
	int getDeviceIndex() { return deviceIndex; };

    /** Set the current device index */
    void setDeviceIndex(int deviceIndex);

    /** Opens a connection to NIDAQ device */
    int openConnection();

	/** Sets the voltage range of the data source. */
	void setVoltageRange(int rangeIndex);

    /** Get available sample rates for current device */
    Array<NIDAQ::float64> getSampleRates() { return mNIDAQ->sampleRates; };

    /** Get the current sample rate */
    NIDAQ::float64 getSampleRate() { return mNIDAQ->getSampleRate(); };

	/** Sets the sample rate of the data source. */
	void setSampleRate(int rateIndex);

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

    /** Called immediately after the end of data acquisition. */
    bool stopAcquisition() override;

    /** Creates the NIDAQOutputEditor. */
    AudioProcessorEditor* createEditor() override;

    /** Saves the connected device*/
    void saveCustomParametersToXml(XmlElement* parentElement) override;

    /** Loads the connected device*/
    void loadCustomParametersFromXml(XmlElement* xml) override;

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
