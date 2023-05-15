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

NIDAQOutput::NIDAQOutput() : GenericProcessor("NIDAQ Output")
{

    dm = new NIDAQmxDeviceManager();

    dm->scanForDevices();

    LOGD("Num devices found: ", dm->getNumAvailableDevices());

    openConnection();

}

NIDAQOutput::~NIDAQOutput() {}

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

void NIDAQOutput::setDevice(String name)
{
    for (int i = 0; i < dm->getNumAvailableDevices(); i++)
    {
        if (dm->getDeviceAtIndex(i)->getName() == name)
        {
            deviceIndex = i;
            openConnection();
            break;
        }
    }
}

int NIDAQOutput::openConnection()
{

    mNIDAQ = new NIDAQmx(dm->getDeviceAtIndex(deviceIndex));

    outputAvailable = mNIDAQ->device->numDOChannels > 0;

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

void NIDAQOutput::updateAnalogChannels()
{
    //TODO 
}

void NIDAQOutput::updateDigitalChannels()
{
    //TODO 
}

void NIDAQOutput::updateSettings()
{
    isEnabled = dm->getNumAvailableDevices() > 0;
}

bool NIDAQOutput::startAcquisition()
{
    LOGD("Starting Tasks...");
    mNIDAQ->startTasks();
    return true;
}

bool NIDAQOutput::stopAcquisition()
{
    mNIDAQ->stopThread(5000);
    return true;
}

void NIDAQOutput::process (AudioBuffer<float>& buffer)
{
    /* Check for events */
    checkForEvents();

    return; //TODO: Support analog data output

    /* Mirror analog output from first input channel on first stream */
    int streamIdx = 0;
    for (auto stream : dataStreams)
    {

        int64 firstSampleNumber = getFirstSampleNumberForBlock(stream->getStreamId());
        const uint16 streamId = stream->getStreamId();

        uint32 numSamples = getNumSamplesInBlock(streamId);

        if (streamIdx == 0)
        {
            mNIDAQ->analogWrite(buffer, numSamples);
        }
        streamIdx++;

    }
}

bool NIDAQOutput::toggleDOChannel(int index)
{
	mNIDAQ->dout[index]->setEnabled(!mNIDAQ->dout[index]->isEnabled());
	return mNIDAQ->dout[index]->isEnabled();
}

void NIDAQOutput::handleTTLEvent(TTLEventPtr event)
{
    const int eventBit = event->getLine() + 1;
    DataStream* stream = getDataStream(event->getStreamId());
    int64 firstSampleNumber = getFirstSampleNumberForBlock(event->getStreamId());

    if (mNIDAQ->sendsSynchronizedEvents())
	    mNIDAQ->addEvent(event->getSampleNumber(), eventBit, event->getState());
    else
        mNIDAQ->digitalWrite(eventBit, event->getState());
}