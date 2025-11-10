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
    
    customWaveform = new CustomWaveform();

}

NIDAQOutput::~NIDAQOutput() {}

void NIDAQOutput::registerParameters()
{
    addCategoricalParameter(
        Parameter::ParameterScope::STREAM_SCOPE, 
        "outputMode", 
        "Output Mode",
        "Analog output mode",
        {"Mirror Input", "Custom Waveform"},
        1);
}

String NIDAQOutput::handleConfigMessage(const String& message)
{
    LOGC("Got config message: ", message);

    double sampleRate = AudioProcessor::getSampleRate();
    if (sampleRate == 0)
    {
        LOGC("Audio sample rate not set yet, using NIDAQ device sample rate");
        sampleRate = mNIDAQ->getSampleRate();
    }

    if (customWaveform->parseProtocol(message, sampleRate))
    {
        LOGC("Successfully parsed protocol and generated waveform at ", sampleRate, " Hz");
    }
    else
    {
        LOGE("Failed to parse protocol");
    }

    return message;
}

void NIDAQOutput::handleBroadcastMessage(const String& msg, const int64 messageTimeMilliseconds)
{
    LOGC("Got broadcast message: ", msg, " at time: ", messageTimeMilliseconds);
    // Assume message is a flag to trigger the custom_waveform to start
    if (msg == "enable_output")
    {
        customWaveform->reset();
        outputEnabled = true;
        LOGC("Output enabled, waveform reset to start");
        LOGC("Total waveform samples: ", customWaveform->getTotalSamples(), 
             " (duration: ", customWaveform->getTotalSamples() / AudioProcessor::getSampleRate(), " seconds)");
        LOGC("Looping: ", customWaveform->isLooping() ? "enabled" : "disabled");
    }
}

void NIDAQOutput::parameterValueChanged(Parameter* parameter)
{
    if (parameter->getName() == "outputMode")
    {
        if (parameter->getValue() == "MIRROR_INPUT")
        {
            outputMode = MIRROR_INPUT;
        }
        else if (parameter->getValue() == "CUSTOM_WAVEFORM")
        {
            outputMode = CUSTOM_WAVEFORM;
        }
    }
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

    outputEnabled = false;
    
    lastSampleRate = AudioProcessor::getSampleRate();
    mNIDAQ->setAudioSampleRate(lastSampleRate);
    LOGC("Set audio sample rate: ", lastSampleRate);
    
    mNIDAQ->startTasks();
    
    // Load default waveform if none configured
    if (!customWaveform->isValid())
    {
        String defaultProtocol = R"({
            "name": "Default Square Wave",
            "sequences": [{
                "conditions": [{
                    "num_repeats": 1,
                    "stimuli": [{
                        "source": "Probe A",
                        "site": 10,
                        "wavelength": 638,
                        "power": 250.0,
                        "duration": 0.0,
                        "pulse_shape": "Square",
                        "pulse_width": 0.01,
                        "pulse_frequency": 20.0
                    }]
                }],
                "min_iti": 0.0
            }]
        })";
        
        if (customWaveform->parseProtocol(defaultProtocol, lastSampleRate))
        {
            LOGC("Loaded default square wave: 2.5V, 20Hz, 10ms pulse width on AO0");
            LOGC("Waveform sample rate: ", lastSampleRate);
            LOGC("NIDAQ device sample rate: ", mNIDAQ->getSampleRate());
        }
        else
        {
            LOGE("Failed to parse default protocol!");
        }
    }
    
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

    outputMode = getOutputMode();
    
    // Check if sample rate changed and regenerate waveform if needed
    double currentSampleRate = AudioProcessor::getSampleRate();
    if (currentSampleRate != lastSampleRate && currentSampleRate > 0)
    {
        lastSampleRate = currentSampleRate;
        mNIDAQ->setAudioSampleRate(currentSampleRate);
        LOGC("Sample rate changed to: ", currentSampleRate);
        
        if (outputMode == CUSTOM_WAVEFORM && customWaveform->isValid())
        {
            customWaveform->regenerateWithNewSampleRate(currentSampleRate);
        }
    }

    if (!outputEnabled) return;

    if (outputMode == MIRROR_INPUT)
    {
        mNIDAQ->analogWrite(buffer, buffer.getNumSamples());
    }
    else if (outputMode == CUSTOM_WAVEFORM)
    {
        if (customWaveform->isValid())
        {
            // Check if waveform is finished
            if (customWaveform->isFinished())
            {
                outputEnabled = false;
                LOGC("Waveform finished after ", customWaveform->getCurrentSample(), " samples");
                LOGC("Output disabled");
                return;
            }
            
            AudioBuffer<float> outputBuffer(buffer.getNumChannels(), buffer.getNumSamples());
            customWaveform->fillBuffer(outputBuffer, buffer.getNumSamples());
            mNIDAQ->analogWrite(outputBuffer, buffer.getNumSamples());
        }
        else
        {
            static int emptyLogCounter = 0;
            if (emptyLogCounter++ % 1000 == 0)
            {
                LOGE("CustomWaveform is not valid!");
            }
        }
    }
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

// ===== CustomWaveform Implementation =====

bool CustomWaveform::parseProtocol(const String& jsonString, double sRate)
{
    sampleRate = sRate;
    currentSample = 0;
    lastProtocolJson = jsonString;
    shouldLoop = false;
    
    var root;
    Result result = JSON::parse(jsonString, root);
    
    if (!result.wasOk())
    {
        return false;
    }
    
    if (!root.hasProperty("sequences") || !root["sequences"].isArray())
    {
        return false;
    }
    
    const var& sequences = root["sequences"];
    if (sequences.size() == 0)
    {
        return false;
    }
    
    // Calculate total buffer size needed
    int totalSamples = 0;
    int maxChannels = 0;
    
    for (int seqIdx = 0; seqIdx < sequences.size(); seqIdx++)
    {
        const var& sequence = sequences[seqIdx];
        
        if (!sequence.hasProperty("conditions") || !sequence["conditions"].isArray())
            continue;
            
        const var& conditions = sequence["conditions"];
        
        for (int condIdx = 0; condIdx < conditions.size(); condIdx++)
        {
            const var& condition = conditions[condIdx];
            int numRepeats = condition.getProperty("num_repeats", 1);
            
            if (!condition.hasProperty("stimuli") || !condition["stimuli"].isArray())
                continue;
                
            const var& stimuli = condition["stimuli"];
            
            for (int stimIdx = 0; stimIdx < stimuli.size(); stimIdx++)
            {
                const var& stimulus = stimuli[stimIdx];
                double duration = stimulus.getProperty("duration", 0.0);
                
                // If duration is 0, calculate one period and set loop flag
                if (duration == 0.0)
                {
                    shouldLoop = true;
                    double pulseFrequency = stimulus.getProperty("pulse_frequency", 1.0);
                    if (pulseFrequency > 0)
                    {
                        duration = 1.0 / pulseFrequency; // One period
                    }
                    else
                    {
                        duration = 1.0; // Default to 1 second
                    }
                }
                
                totalSamples += static_cast<int>(duration * sampleRate) * numRepeats;
                maxChannels = jmax(maxChannels, stimIdx + 1);
            }
        }
        
        double minIti = sequence.getProperty("min_iti", 0.0);
        totalSamples += static_cast<int>(minIti * sampleRate);
    }
    
    numChannels = jmax(maxChannels, 1);
    waveformBuffer.setSize(numChannels, totalSamples);
    waveformBuffer.clear();
    
    // Generate waveforms for each stimulus
    int currentPosition = 0;
    
    for (int seqIdx = 0; seqIdx < sequences.size(); seqIdx++)
    {
        const var& sequence = sequences[seqIdx];
        
        if (!sequence.hasProperty("conditions") || !sequence["conditions"].isArray())
            continue;
            
        const var& conditions = sequence["conditions"];
        
        for (int condIdx = 0; condIdx < conditions.size(); condIdx++)
        {
            const var& condition = conditions[condIdx];
            int numRepeats = condition.getProperty("num_repeats", 1);
            
            if (!condition.hasProperty("stimuli") || !condition["stimuli"].isArray())
                continue;
                
            const var& stimuli = condition["stimuli"];
            
            for (int repeat = 0; repeat < numRepeats; repeat++)
            {
                for (int stimIdx = 0; stimIdx < stimuli.size(); stimIdx++)
                {
                    const var& stimulus = stimuli[stimIdx];
                    String pulseShape = stimulus.getProperty("pulse_shape", "Square").toString();
                    double duration = stimulus.getProperty("duration", 0.0);
                    
                    // If duration is 0, use one period
                    if (duration == 0.0)
                    {
                        double pulseFrequency = stimulus.getProperty("pulse_frequency", 1.0);
                        if (pulseFrequency > 0)
                        {
                            duration = 1.0 / pulseFrequency;
                        }
                        else
                        {
                            duration = 1.0;
                        }
                    }
                    
                    int durationSamples = static_cast<int>(duration * sampleRate);
                    
                    if (pulseShape == "Square")
                    {
                        double pulseWidth = stimulus.getProperty("pulse_width", 0.01);
                        double pulseFrequency = stimulus.getProperty("pulse_frequency", 0.0);
                        double power = stimulus.getProperty("power", 0.0);
                        
                        int pulseWidthSamples = static_cast<int>(pulseWidth * sampleRate);
                        int pulsePeriodSamples = pulseFrequency > 0 ? static_cast<int>(sampleRate / pulseFrequency) : durationSamples;
                        
                        for (int i = 0; i < durationSamples && (currentPosition + i) < waveformBuffer.getNumSamples(); i++)
                        {
                            int posInPeriod = i % pulsePeriodSamples;
                            float value = (posInPeriod < pulseWidthSamples) ? power : 0.0f;
                            waveformBuffer.setSample(stimIdx, currentPosition + i, value);
                        }
                    }
                    else if (pulseShape == "Custom")
                    {
                        const var& customWaveform = stimulus["custom_waveform"];
                        if (customWaveform.isArray() && customWaveform.size() > 0)
                        {
                            for (int i = 0; i < durationSamples && (currentPosition + i) < waveformBuffer.getNumSamples(); i++)
                            {
                                int waveformIdx = i % customWaveform.size();
                                float value = customWaveform[waveformIdx];
                                waveformBuffer.setSample(stimIdx, currentPosition + i, value);
                            }
                        }
                    }
                    
                    currentPosition += durationSamples;
                }
            }
        }
        
        double minIti = sequence.getProperty("min_iti", 0.0);
        currentPosition += static_cast<int>(minIti * sampleRate);
    }
    
    return true;
}

bool CustomWaveform::regenerateWithNewSampleRate(double newSampleRate)
{
    if (lastProtocolJson.isEmpty())
        return false;
    
    LOGC("Regenerating waveform with new sample rate: ", newSampleRate);
    return parseProtocol(lastProtocolJson, newSampleRate);
}

void CustomWaveform::fillBuffer(AudioBuffer<float>& buffer, int numSamples)
{
    if (!isValid())
    {
        buffer.clear();
        return;
    }
    
    buffer.clear();
    
    for (int sample = 0; sample < numSamples; sample++)
    {
        for (int channel = 0; channel < jmin(buffer.getNumChannels(), waveformBuffer.getNumChannels()); channel++)
        {
            if (currentSample < waveformBuffer.getNumSamples())
            {
                buffer.setSample(channel, sample, waveformBuffer.getSample(channel, currentSample));
            }
            else
            {
                buffer.setSample(channel, sample, 0.0f);
            }
        }
        
        currentSample++;
        
        // If looping is enabled and we reached the end, wrap around
        if (shouldLoop && currentSample >= waveformBuffer.getNumSamples())
        {
            currentSample = 0;
        }
    }
}