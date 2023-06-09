/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

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

#ifndef __NIDAQOUTPUTEDITOR_H_28EB4CC9__
#define __NIDAQOUTPUTEDITOR_H_28EB4CC9__


#include <EditorHeaders.h>
#include "NIDAQOutput.h"
#include <SerialLib.h>

class NIDAQOutputEditor;

/**

  User interface for the NIDAQOutput processor.

  @see NIDAQOutput

*/

class EditorBackground : public Component
{
public:
	EditorBackground(int nAO, int nDO);

private:
	void paint(Graphics& g);
	int nAO;
	int nDO;
};

class AOButton : public ToggleButton, public Timer
{
public:
	AOButton(int id, NIDAQOutput*);

	void setId(int id);
	int getId(); 
	void setEnabled(bool);
	void timerCallback();

	NIDAQOutput* processor;

	friend class NIDAQOutputEditor;

private:
	void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown);

	int id;
	bool enabled;

};

class DOButton : public ToggleButton, public Timer
{
public:
	DOButton(int id, NIDAQOutput*);

	void setId(int id);
	int getId();
	void setEnabled(bool);
	void timerCallback();

	NIDAQOutput* processor;

	friend class NIDAQOutputEditor;

private:
	void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown);

	int id;
	bool enabled;

};

class SourceTypeButton : public TextButton, public Timer
{
public:
	SourceTypeButton(int id, NIDAQOutput* processor, SOURCE_TYPE source);

	void setId(int id);
	int getId();
	void toggleSourceType();
	void timerCallback();

	void update(SOURCE_TYPE sourceType);

	NIDAQOutput* processor;

	friend class NIDAQEditor;

private:

	int id;
	bool enabled;

};

class PopupConfigurationWindow : public Component, public ComboBox::Listener, public Button::Listener
{

public:
    
    /** Constructor */
    PopupConfigurationWindow(NIDAQOutputEditor* editor);

    /** Destructor */
    ~PopupConfigurationWindow() { }

	void comboBoxChanged(ComboBox*);
	void buttonClicked(Button* button) override;

	void paint(Graphics& g) override;

private:

	NIDAQOutputEditor* editor;

    ScopedPointer<Label>  analogLabel;
    ScopedPointer<ComboBox> analogChannelCountSelect;

	ScopedPointer<Label>  digitalLabel;
	ScopedPointer<ComboBox> digitalChannelCountSelect;

	ScopedPointer<Label>  digitalWriteLabel;
	ScopedPointer<ComboBox> digitalWriteSelect;

	ScopedPointer<Label> digitalPortsLabel;
	ScopedPointer<ComboBox> digitalPortsSelect;

	OwnedArray<ToggleButton> digitalPortButtons;

};

class NIDAQOutputEditor : public GenericEditor,
                            public ComboBox::Listener,
                            public Button::Listener
{
public:
    /** Constructor*/
    NIDAQOutputEditor(GenericProcessor* parentNode);

    /** Destructor*/
    ~NIDAQOutputEditor() { }

    /** Called whenever the editor updates UI elements */
    void draw();

    /** Called when device configuration is changed */
    void update(int analogCount, int digitalCount, int digitalWrite);

    /** Called when a combo box selection is changed */
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged);

    /** Respond to button presses */
	void buttonClicked(Button* button) override;

    /** Called when a button gets pressed */
    void buttonEvent(Button* button);

    /** Gets the latest device from the processor*/
    void updateDevice(String deviceName);

    int getTotalAvailableAnalogOutputs() { return processor->getTotalAvailableAnalogOutputs(); };
	int getTotalAvailableDigitalOutputs() { return processor-> getTotalAvailableDigitalOutputs(); };

	int getNumActiveAnalogOutputs() { return processor->getNumActiveAnalogOutputs(); };
	int getNumActiveDigitalOutputs() { return processor->getNumActiveDigitalOutputs(); };

	int getDigitalWriteSize() { return processor->getDigitalWriteSize(); };

	int getNumPorts() { return processor->getNumPorts(); };
	bool getPortState(int idx) { return processor->getPortState(idx); };
	void setPortState(int idx, bool state) { processor->setPortState(idx, state); };

	void saveCustomParametersToXml(XmlElement*) override;
	void loadCustomParametersFromXml(XmlElement*) override;
	
private:

    NIDAQOutput* processor;

    OwnedArray<AOButton> aoButtons;
	OwnedArray<TextButton> sourceTypeButtons;
	OwnedArray<DOButton> doButtons;

	ScopedPointer<ComboBox> deviceSelectBox;
	ScopedPointer<ComboBox> sampleRateSelectBox;
	ScopedPointer<ComboBox> voltageRangeSelectBox;
    //TODO: Potentially use to monitor arbitrary wave generation CPU usage
    //ScopedPointer<FifoMonitor> fifoMonitor;

	ScopedPointer<UtilityButton> configureDeviceButton;

	Array<File> savingDirectories;

	//ScopedPointer<BackgroundLoader> uiLoader;
	ScopedPointer<EditorBackground> background;

	PopupConfigurationWindow* currentConfigWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NIDAQOutputEditor);

};

#endif  // __NIDAQOUTPUTEDITOR_H_28EB4CC9__
