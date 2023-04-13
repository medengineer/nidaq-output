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

#include "NIDAQOutput.h"
#include "NIDAQOutputEditor.h"

NIDAQOutput::NIDAQOutput()
    : GenericProcessor("NIDAQ Output")
{
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

void NIDAQOutput::updateSettings()
{
    //isEnabled = deviceSelected;
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

    /*
    const int eventBit = event->getLine() + 1;
    DataStream* stream = getDataStream(event->getStreamId());

    if (eventBit == int((*stream)["gate_line"]))
    {
        if (event->getState())
            gateIsOpen = true;
        else
            gateIsOpen = false;
    }

    if (gateIsOpen)
    {
        if (eventBit == int((*stream)["input_line"]))
        {

            if (event->getState())
            {
                NIDAQ.sendDigital(
                    getParameter("output_pin")->getValue(),
                    ARD_LOW);
            }
            else
            {
                NIDAQ.sendDigital(
                    getParameter("output_pin")->getValue(),
                    ARD_HIGH);
            }
        }
    }
    */
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
