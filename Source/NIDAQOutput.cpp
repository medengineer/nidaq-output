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

#include "NIDAQComponents.h"
#include "NIDAQOutput.h"
#include "NIDAQOutputEditor.h"

NIDAQOutput::NIDAQOutput()
    : GenericProcessor("NIDAQ Output")
{

    dm = new NIDAQmxDeviceManager();

	dm->scanForDevices();

	LOGC("Num devices found: ", dm->getNumAvailableDevices());
    LOGC("Current device name: ", dm->getDeviceAtIndex(deviceIndex)->getName());

	openConnection();

    /* TODO: Will need both categorical and int params for analog/digital out*/
    /*
    addIntParameter(Parameter::GLOBAL_SCOPE, "output_pin", "The NIDAQ pin to use", 13, 0, 13);
    addIntParameter(Parameter::STREAM_SCOPE, "input_line", "The TTL line for triggering output", 1, 1, 16);
    addIntParameter(Parameter::STREAM_SCOPE, "gate_line", "The TTL line for gating the output", 0, 0, 16);
    */
}

NIDAQOutput::~NIDAQOutput()
{
}

AudioProcessorEditor* NIDAQOutput::createEditor()
{
    editor = std::make_unique<NIDAQOutputEditor>(this);
    return editor.get();
}

Array<NIDAQDevice*> NIDAQOutput::getDevices()
{
	Array<NIDAQDevice*> deviceList;

	for (int i = 0; i < dm->getNumAvailableDevices(); i++)
		deviceList.add(dm->getDeviceAtIndex(i));

	return deviceList;
}

void NIDAQOutput::setDevice(int index)
{
	deviceIndex = index;
    openConnection();
}

int NIDAQOutput::openConnection()
{

	mNIDAQ = new NIDAQmx(dm->getDeviceAtIndex(deviceIndex));

    //TODO: Might need buffer for analog output 
	// mNIDAQ->aoBuffer = sourceBuffers.getLast();

	sampleRateIndex = mNIDAQ->sampleRates.size() - 1;
	setSampleRate(sampleRateIndex);

	voltageRangeIndex = mNIDAQ->device->voltageRanges.size() - 1;
	setVoltageRange(voltageRangeIndex);

	return 0;

}

void NIDAQOutput::setSampleRate(int rateIndex)
{
	sampleRateIndex = rateIndex;
	mNIDAQ->setSampleRate(rateIndex);
}

Array<SettingsRange> NIDAQOutput::getVoltageRanges()
{
	return mNIDAQ->device->voltageRanges;
}

void NIDAQOutput::setVoltageRange(int rangeIndex)
{
	voltageRangeIndex = rangeIndex;
	mNIDAQ->setVoltageRange(rangeIndex);
}

void NIDAQOutput::updateSettings()
{
    isEnabled = outputAvailable;
}

bool NIDAQOutput::stopAcquisition()
{
    //TODO: Set everything low
    return true;
}


void NIDAQOutput::process (AudioBuffer<float>& buffer)
{
    checkForEvents ();
}

void NIDAQOutput::handleTTLEvent(TTLEventPtr event)
{

    //TODO: Trigger output based on current settings
    const int eventBit = event->getLine() + 1;
    DataStream* stream = getDataStream(event->getStreamId());

    /* TODO Restore gate? 
    if (eventBit == int((*stream)["gate_line"]))
    {
        if (event->getState())
            gateIsOpen = true;
        else
            gateIsOpen = false;
    }
    */

    if (true)
    {
        if (eventBit == 0) // int((*stream)["input_line"]))
        {

            LOGC("Got event!");

            if (event->getState())
            {
                LOGC("Detected on");
                /* TODO Add NIDAQ sendDigital function
                mNIDAQmx->sendDigital(
                    getParameter("output_pin")->getValue(),
                    0);
                */
            }
            else
            {
                LOGC("Detected off");
                /*
                NIDAQ.sendDigital(
                    getParameter("output_pin")->getValue(),
                    1);
                */
            }
        }
    }

}


void NIDAQOutput::saveCustomParametersToXml(XmlElement* parentElement)
{
    //parentElement->setAttribute("device", deviceString);
}

void NIDAQOutput::loadCustomParametersFromXml(XmlElement* xml)
{
    /*
    setDevice(xml->getStringAttribute("device", ""));
    NIDAQOutputEditor* ed = (NIDAQOutputEditor*) editor.get();

    ed->updateDevice(xml->getStringAttribute("device", ""));
    */

}
