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
        // Update NIDAQ channel count to match waveform
        int numWaveformChannels = customWaveform->getNumChannels();
        if (mNIDAQ->getNumActiveAnalogOutputs() != numWaveformChannels)
        {
            // Stop thread if running
            bool wasRunning = mNIDAQ->isThreadRunning();
            if (wasRunning)
            {
                mNIDAQ->stopThread(1000);
            }
            
            mNIDAQ->setNumActiveAnalogOutputs(numWaveformChannels);
            mNIDAQ->startTasks(); // Reconfigure DAQmx task with new channel count
            
            // Thread will restart automatically when analogWrite is called
        }
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
        /*
        For Custom waveforms:
        - pulse_width:          Stretch array across this time (e.g., 0.1 = 100ms)
        - waveform_sample_rate: Each sample = 1/rate seconds (e.g., 40Hz = 25ms per sample)
        - duration:             Number of cycles to repeat the waveform
        
        Default: 4-step ramp at 40Hz, 3 cycles = 300ms total
        */
        String defaultProtocol = R"({
            "name": "Default Custom Waveform",
            "sequences": [{
                "conditions": [{
                    "num_repeats": 1,
                    "stimuli": [{
                        "pulse_shape": "Custom",
                        "waveform_sample_rate": 40.0,
                        "custom_waveform": [100.0, 200.0, 300.0, 400.0],
                        "duration": 3.0
                    }]
                }],
                "min_iti": 0.0
            }]
        })";
        
        if (customWaveform->parseProtocol(defaultProtocol, lastSampleRate))
        {
            LOGC("Loaded default custom waveform: 4-step ramp, 3 cycles (300ms total)");
            LOGC("Output sample rate: ", lastSampleRate);
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
    //mNIDAQ->stopThread(5000);
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
                // Write zeros to reset output before disabling
                int numChannels = customWaveform->getNumChannels();
                AudioBuffer<float> zeroBuffer(numChannels, buffer.getNumSamples());
                zeroBuffer.clear();
                mNIDAQ->analogWrite(zeroBuffer, buffer.getNumSamples());
                
                outputEnabled = false;
                LOGC("Waveform finished after ", customWaveform->getCurrentSample(), " samples");
                LOGC("Output disabled and reset to 0");
                return;
            }
            
            int numWaveformChannels = customWaveform->getNumChannels();
            AudioBuffer<float> outputBuffer(numWaveformChannels, buffer.getNumSamples());
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
        return false;
    
    // Detect format: wave_player format has pattern fields (pulse, sine, custom)
    if (root.hasProperty("patternType") || root.hasProperty("pulse") || 
        root.hasProperty("sine") || root.hasProperty("custom"))
        return parseWavePlayer(root);
    
    // Original protocol format with sequences
    if (!root.hasProperty("sequences") || !root["sequences"].isArray())
        return false;
    
    const var& sequences = root["sequences"];
    if (sequences.size() == 0)
        return false;
    
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
                String pulseShape = stimulus.getProperty("pulse_shape", "Square").toString();
                double duration = stimulus.getProperty("duration", 0.0);
                double durationSeconds = 0.0;
                
                if (pulseShape == "Custom")
                {
                    // For Custom: duration = number of cycles
                    const var& customWaveformArray = stimulus["custom_waveform"];
                    double pulseWidth = stimulus.getProperty("pulse_width", 0.0);
                    double waveformSampleRate = stimulus.getProperty("waveform_sample_rate", 0.0);
                    
                    double waveformLengthSeconds = 0.0;
                    if (customWaveformArray.isArray() && customWaveformArray.size() > 0)
                    {
                        int numSteps = customWaveformArray.size();
                        if (pulseWidth > 0)
                            waveformLengthSeconds = pulseWidth;
                        else if (waveformSampleRate > 0)
                            waveformLengthSeconds = numSteps / waveformSampleRate;
                        else
                            waveformLengthSeconds = numSteps / sampleRate;
                    }
                    
                    int numCycles = (duration > 0) ? static_cast<int>(duration) : 1;
                    durationSeconds = waveformLengthSeconds * numCycles;
                }
                else
                {
                    // For Square/other: duration = time in seconds
                    if (duration == 0.0)
                    {
                        shouldLoop = true;
                        double pulseFrequency = stimulus.getProperty("pulse_frequency", 1.0);
                        if (pulseFrequency > 0)
                            duration = 1.0 / pulseFrequency;
                        else
                            duration = 1.0;
                    }
                    durationSeconds = duration;
                }
                
                totalSamples += static_cast<int>(durationSeconds * sampleRate) * numRepeats;
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
                    int durationSamples;
                    
                    if (pulseShape == "Square")
                    {
                        // For Square: duration = time in seconds
                        if (duration == 0.0)
                        {
                            double pulseFrequency = stimulus.getProperty("pulse_frequency", 1.0);
                            if (pulseFrequency > 0)
                                duration = 1.0 / pulseFrequency;
                            else
                                duration = 1.0;
                        }
                        durationSamples = static_cast<int>(duration * sampleRate);
                        
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
                        const var& customWaveformArray = stimulus["custom_waveform"];
                        double pulseWidth = stimulus.getProperty("pulse_width", 0.0);
                        double waveformSampleRate = stimulus.getProperty("waveform_sample_rate", 0.0);
                        
                        if (customWaveformArray.isArray() && customWaveformArray.size() > 0)
                        {
                            int numSteps = customWaveformArray.size();
                            int pulseWidthSamples;
                            int samplesPerStep;
                            
                            if (pulseWidth > 0)
                            {
                                // pulse_width mode: stretch waveform across pulse_width seconds
                                pulseWidthSamples = static_cast<int>(pulseWidth * sampleRate);
                                samplesPerStep = pulseWidthSamples / numSteps;
                            }
                            else if (waveformSampleRate > 0)
                            {
                                // waveform_sample_rate mode: each sample = 1/waveformSampleRate seconds
                                samplesPerStep = static_cast<int>(sampleRate / waveformSampleRate);
                                pulseWidthSamples = samplesPerStep * numSteps;
                            }
                            else
                            {
                                // Default: 1:1 mapping (1 sample per output sample)
                                samplesPerStep = 1;
                                pulseWidthSamples = numSteps;
                            }
                            
                            // For Custom: duration = number of cycles
                            int numCycles = (duration > 0) ? static_cast<int>(duration) : 1;
                            durationSamples = pulseWidthSamples * numCycles;
                            
                            // Repeat waveform back-to-back for each cycle
                            int pulsePeriodSamples = pulseWidthSamples;
                            
                            for (int i = 0; i < durationSamples && (currentPosition + i) < waveformBuffer.getNumSamples(); i++)
                            {
                                int posInPeriod = i % pulsePeriodSamples;
                                
                                if (posInPeriod < pulseWidthSamples)
                                {
                                    int stepIndex = posInPeriod / samplesPerStep;
                                    if (stepIndex >= numSteps) stepIndex = numSteps - 1;
                                    
                                    float value = customWaveformArray[stepIndex];
                                    waveformBuffer.setSample(stimIdx, currentPosition + i, value);
                                }
                                else
                                {
                                    waveformBuffer.setSample(stimIdx, currentPosition + i, 0.0f);
                                }
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

bool CustomWaveform::parseWavePlayer(const var& root)
{
    double sourceSampleRate = root.getProperty("sampleRate", 30000.0);
    double maxVoltage = root.getProperty("maxVoltage", 5.0);
    double ratio = sampleRate / sourceSampleRate;
    
    // Collect all enabled patterns and determine max channel and total samples
    struct PatternInfo {
        int channel;
        int samples;
    };
    std::vector<PatternInfo> patterns;
    numChannels = 1;
    int totalSamples = 0;
    
    if (root.hasProperty("pulse"))
    {
        const var& pulse = root["pulse"];
        int channel = pulse.getProperty("analogOutputChannel", 0);
        int onDuration = pulse.getProperty("onDuration", 100);
        int offDuration = pulse.getProperty("offDuration", 100);
        int delayDuration = pulse.getProperty("delayDuration", 0);
        int repeatNumber = pulse.getProperty("repeatNumber", 1);
        
        int delayOut = static_cast<int>(delayDuration * ratio);
        int onOut = static_cast<int>(onDuration * ratio);
        int offOut = static_cast<int>(offDuration * ratio);
        int samples = delayOut + repeatNumber * (onOut + offOut);
        
        patterns.push_back({channel, samples});
        numChannels = jmax(numChannels, channel + 1);
        totalSamples = jmax(totalSamples, samples);
    }
    
    if (root.hasProperty("sine"))
    {
        const var& sine = root["sine"];
        int channel = sine.getProperty("analogOutputChannel", 0);
        double frequency = sine.getProperty("frequency", 5.0);
        int cycles = sine.getProperty("cycles", 1);
        int delayDuration = sine.getProperty("delayDuration", 0);
        
        int delayOut = static_cast<int>(delayDuration * ratio);
        int samplesPerCycle = static_cast<int>(sampleRate / frequency);
        int samples = delayOut + cycles * samplesPerCycle;
        
        patterns.push_back({channel, samples});
        numChannels = jmax(numChannels, channel + 1);
        totalSamples = jmax(totalSamples, samples);
    }
    
    if (root.hasProperty("custom"))
    {
        const var& custom = root["custom"];
        int channel = custom.getProperty("analogOutputChannel", 0);
        String customString = custom.getProperty("string", "0").toString();
        
        StringArray tokens;
        tokens.addTokens(customString, ",", "");
        int samples = tokens.size();
        
        patterns.push_back({channel, samples});
        numChannels = jmax(numChannels, channel + 1);
        totalSamples = jmax(totalSamples, samples);
    }
    
    if (totalSamples == 0)
        return false;
    
    waveformBuffer.setSize(numChannels, totalSamples);
    waveformBuffer.clear();
    
    LOGC("parseWavePlayer: numChannels=", numChannels, ", totalSamples=", totalSamples);
    
    // Generate pulse waveform
    if (root.hasProperty("pulse"))
    {
        const var& pulse = root["pulse"];
        int channel = pulse.getProperty("analogOutputChannel", 0);
        int onDuration = pulse.getProperty("onDuration", 100);
        int offDuration = pulse.getProperty("offDuration", 100);
        int delayDuration = pulse.getProperty("delayDuration", 0);
        int repeatNumber = pulse.getProperty("repeatNumber", 1);
        int rampOnDuration = pulse.getProperty("rampOnDuration", 0);
        int rampOffDuration = pulse.getProperty("rampOffDuration", 0);
        float pulseVoltage = pulse.getProperty("maxVoltage", maxVoltage);
        
        int delayOut = static_cast<int>(delayDuration * ratio);
        int onOut = static_cast<int>(onDuration * ratio);
        int offOut = static_cast<int>(offDuration * ratio);
        int rampOnOut = static_cast<int>(rampOnDuration * ratio);
        int rampOffOut = static_cast<int>(rampOffDuration * ratio);
        
        int pos = 0;
        for (int i = 0; i < delayOut; i++)
            waveformBuffer.setSample(channel, pos++, 0.0f);
        
        for (int rep = 0; rep < repeatNumber; rep++)
        {
            for (int i = 0; i < onOut; i++)
            {
                float value = pulseVoltage;
                if (rampOnOut > 0 && i < rampOnOut)
                    value = pulseVoltage * (float(i) / rampOnOut);
                if (rampOffOut > 0 && i >= (onOut - rampOffOut))
                    value = pulseVoltage * (float(onOut - i) / rampOffOut);
                waveformBuffer.setSample(channel, pos++, value);
            }
            for (int i = 0; i < offOut; i++)
                waveformBuffer.setSample(channel, pos++, 0.0f);
        }
    }
    
    // Generate sine waveform
    if (root.hasProperty("sine"))
    {
        const var& sine = root["sine"];
        int channel = sine.getProperty("analogOutputChannel", 0);
        double frequency = sine.getProperty("frequency", 5.0);
        int cycles = sine.getProperty("cycles", 1);
        int delayDuration = sine.getProperty("delayDuration", 0);
        float sineVoltage = sine.getProperty("maxVoltage", maxVoltage);
        
        int delayOut = static_cast<int>(delayDuration * ratio);
        int samplesPerCycle = static_cast<int>(sampleRate / frequency);
        
        int pos = 0;
        for (int i = 0; i < delayOut; i++)
            waveformBuffer.setSample(channel, pos++, 0.0f);
        
        int sineSamples = cycles * samplesPerCycle;
        LOGC("Generating sine: channel=", channel, ", frequency=", frequency, ", cycles=", cycles, ", samplesPerCycle=", samplesPerCycle, ", totalSamples=", sineSamples);
        for (int i = 0; i < sineSamples && pos < totalSamples; i++)
        {
            float value = sineVoltage * std::sin(2.0 * MathConstants<double>::pi * frequency * i / sampleRate);
            waveformBuffer.setSample(channel, pos++, value);
        }
        LOGC("Sine waveform generated: wrote ", pos - delayOut, " samples to channel ", channel);
    }
    
    // Generate custom waveform
    if (root.hasProperty("custom"))
    {
        const var& custom = root["custom"];
        int channel = custom.getProperty("analogOutputChannel", 0);
        String customString = custom.getProperty("string", "0").toString();
        
        StringArray tokens;
        tokens.addTokens(customString, ",", "");
        
        for (int i = 0; i < tokens.size(); i++)
            waveformBuffer.setSample(channel, i, tokens[i].getFloatValue());
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
    
    static int debugCounter = 0;
    if (debugCounter++ % 1000 == 0)
    {
        LOGC("fillBuffer: buffer channels=", buffer.getNumChannels(), ", waveform channels=", waveformBuffer.getNumChannels(), ", currentSample=", currentSample);
    }
    
    for (int sample = 0; sample < numSamples; sample++)
    {
        for (int channel = 0; channel < jmin(buffer.getNumChannels(), waveformBuffer.getNumChannels()); channel++)
        {
            if (currentSample < waveformBuffer.getNumSamples())
            {
                float value = waveformBuffer.getSample(channel, currentSample);
                buffer.setSample(channel, sample, value);
                if (debugCounter % 1000 == 0 && sample == 0 && channel == 1)
                {
                    LOGC("Channel 1 sample[0]=", value);
                }
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