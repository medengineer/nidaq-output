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

#include "NIDAQOutputEditor.h"
#include <stdio.h>

EditorBackground::EditorBackground(int nAO, int nDO) : nAO(nAO), nDO(nDO) {}

void EditorBackground::paint(Graphics& g)
{

	if (nAO > 0 || nDO > 0)
	{

		/* Draw AO channels */
		int maxChannelsPerColumn = 4;
		int aoChannelsPerColumn = nAO > 0 && nAO < maxChannelsPerColumn ? nAO : maxChannelsPerColumn;
		int doChannelsPerColumn = nDO > 0 && nDO < maxChannelsPerColumn ? nDO : maxChannelsPerColumn;

		float aoChanOffsetX = 15; //pixels
		float aoChanOffsetY = 12; //pixels
		float aoChanWidth = 70;   //pixels
		float aoChanHeight = 22;  //pixels TODO: normalize
		float paddingX = 1.07;
		float paddingY = 1.18;

		for (int i = 0; i < nAO; i++)
		{

			int colIndex = i / aoChannelsPerColumn;
			int rowIndex = i % aoChannelsPerColumn;

			g.setColour(Colours::lightgrey);
			g.drawRoundedRectangle(
				aoChanOffsetX + paddingX * colIndex * aoChanWidth,
				aoChanOffsetY + paddingY * rowIndex * aoChanHeight,
				aoChanWidth, aoChanHeight, 4, 3);


			g.setColour(Colours::darkgrey);

			g.drawRoundedRectangle(
				aoChanOffsetX + paddingX * colIndex * aoChanWidth,
				aoChanOffsetY + paddingY * rowIndex * aoChanHeight,
				aoChanWidth, aoChanHeight, 4, 1);

			/*
			g.drawRoundedRectangle(
			aoChanOffsetX + colIndex * paddingX * aoChanWidth + aoChanWidth - aoChanWidth / 3,
			16 + paddingY * aoChanHeight * rowIndex,
			aoChanWidth / 3 - 4, 14, 1, 0.4);
			*/

			g.setFont(10);
			g.drawText(
				String("AO") + String(i),
				5 + aoChanOffsetX + paddingX * colIndex * aoChanWidth,
				7 + aoChanOffsetY + paddingY * rowIndex * aoChanHeight,
				20, 10, Justification::centredLeft);

			/*
			g.drawText(String("FS"),
			51 + aoChanOffsetX + paddingX * colIndex * aoChanWidth,
			7 + aoChanOffsetY + paddingY * rowIndex * aoChanHeight,
			20, 10, Justification::centredLeft);
			*/

		}

		/* Draw DO lines */
		float doChanOffsetX = aoChanOffsetX + ((nAO % maxChannelsPerColumn == 0 ? 0 : 1) + nAO / aoChannelsPerColumn) * paddingX * aoChanWidth;
		float doChanOffsetY = aoChanOffsetY;
		float doChanWidth = 42;
		float doChanHeight = 22;

		for (int i = 0; i < nDO; i++)
		{

			int colIndex = i / doChannelsPerColumn;
			int rowIndex = i % doChannelsPerColumn;

			g.setColour(Colours::lightgrey);
			g.drawRoundedRectangle(
				doChanOffsetX + paddingX * colIndex * doChanWidth,
				doChanOffsetY + paddingY * rowIndex * doChanHeight,
				doChanWidth, doChanHeight, 4, 3);

			g.setColour(Colours::darkgrey);
			g.drawRoundedRectangle(
				doChanOffsetX + paddingX * colIndex * doChanWidth,
				doChanOffsetY + paddingY * rowIndex * doChanHeight,
				doChanWidth, doChanHeight, 4, 1);

			g.setFont(10);
			if (i >= 10)
				g.setFont(8);
			g.drawText(
				"DO" + String(i),
				5 + doChanOffsetX + paddingX * colIndex * doChanWidth,
				7 + doChanOffsetY + paddingY * rowIndex * doChanHeight,
				20, 10, Justification::centredLeft);

		}

		//FIFO monitor label
		float settingsOffsetX = doChanOffsetX + ((nDO % maxChannelsPerColumn == 0 ? 0 : 1) + nDO / doChannelsPerColumn) * paddingX * doChanWidth + 5;
		g.setColour(Colours::darkgrey);
		g.setFont(10);
		
		g.drawText(String("DEVICE"), settingsOffsetX, 13, 100, 10, Justification::centredLeft);
		g.drawText(String("SAMPLE RATE"), settingsOffsetX, 45, 100, 10, Justification::centredLeft);
		g.drawText(String("AO VOLTAGE RANGE"), settingsOffsetX, 77, 100, 10, Justification::centredLeft);

		/*
		g.drawText(String("USAGE"), settingsOffsetX, 77, 100, 10, Justification::centredLeft);
		g.setFont(8);
		g.drawText(String("0"), settingsOffsetX, 100, 50, 10, Justification::centredLeft);
		g.drawText(String("100"), settingsOffsetX + 65, 100, 50, 10, Justification::centredLeft);
		g.drawText(String("%"), settingsOffsetX + 33, 100, 50, 10, Justification::centredLeft);
		*/


	}

}

AOButton::AOButton(int id_, NIDAQOutput* processor_) : id(id_), processor(processor_), enabled(true)
{
	startTimer(500);
}

void AOButton::setId(int id_)
{
	id = id_;
}

int AOButton::getId()
{
	return id;
}

void AOButton::setEnabled(bool enable)
{
	enabled = enable;
    processor->setAnalogEnable(id, enable);
}

void AOButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
	if (isMouseOver && enabled)
		g.setColour(Colours::antiquewhite);
	else
		g.setColour(Colours::darkgrey);
	g.fillEllipse(0, 0, 15, 15);

	if (enabled && processor->isOutputAvailable())
	{
		if (isMouseOver)
			g.setColour(Colours::lightgreen);
		else
			g.setColour(Colours::forestgreen);
	}
	else
	{
		if (isMouseOver)
			g.setColour(Colours::lightgrey);
		else
			g.setColour(Colours::lightgrey);
	}
	g.fillEllipse(3, 3, 9, 9);
}

void AOButton::timerCallback()
{

}

DOButton::DOButton(int id_, NIDAQOutput* processor_) : id(id_), processor(processor_), enabled(true)
{
	startTimer(500);
}

void DOButton::setId(int id_)
{
	id = id_;
}

int DOButton::getId()
{
	return id;
}

void DOButton::setEnabled(bool enable)
{
	enabled = enable;
    processor->setDigitalEnable(id, enable);
}

void DOButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{

	if (isMouseOver && enabled)
		g.setColour(Colours::antiquewhite);
	else
		g.setColour(Colours::darkgrey);
	g.fillRoundedRectangle(0, 0, 15, 15, 2);

	if (enabled && processor->isOutputAvailable())
	{
		if (isMouseOver)
			g.setColour(Colours::lightgreen);
		else
			g.setColour(Colours::forestgreen);
	} 
	else 
	{
		if (isMouseOver)
			g.setColour(Colours::lightgrey);
		else
			g.setColour(Colours::lightgrey);
	}
	g.fillRoundedRectangle(3, 3, 9, 9, 2);
}

void DOButton::timerCallback()
{

}

SourceTypeButton::SourceTypeButton(int id_, NIDAQOutput* processor_, SOURCE_TYPE source) : id(id_), processor(processor_)
{

	update(source);

}

void SourceTypeButton::setId(int id_)
{
	id = id_;
}

int SourceTypeButton::getId()
{
	return id;
}

void SourceTypeButton::update(SOURCE_TYPE sourceType)
{
	switch (sourceType) {
	case SOURCE_TYPE::RSE:
		setButtonText("RSE"); return;
	case SOURCE_TYPE::NRSE:
		setButtonText("NRSE"); return;
	case SOURCE_TYPE::DIFF:
		setButtonText("DIFF"); return;
	case SOURCE_TYPE::PSEUDO_DIFF:
		setButtonText("PDIF"); return;
	default:
		break;
	}
}

void SourceTypeButton::timerCallback()
{

}

NIDAQOutputEditor::NIDAQOutputEditor(GenericProcessor* parentNode)
    : GenericEditor(parentNode)

{

    processor = (NIDAQOutput*)getProcessor();

    desiredWidth = 240;

	int nAO;
	int nDO;

	if (processor->getDeviceName() == "Simulated Device")
	{
		nAO = 8;
		nDO = 8;
	}
	else
	{
		nAO = processor->getNumActiveAnalogOutputs();
		nDO = processor->getNumActiveDigitalOutputs();
	}

	int maxChannelsPerColumn = 4;
	int aoChannelsPerColumn = nAO > 0 && nAO < maxChannelsPerColumn ? nAO : maxChannelsPerColumn;
	int doChannelsPerColumn = nDO > 0 && nDO < maxChannelsPerColumn ? nDO : maxChannelsPerColumn;

	aoButtons.clear();
	sourceTypeButtons.clear();

	int xOffset = 0;

	// Draw analog inputs 
	for (int i = 0; i < nAO; i++)
	{

		int colIndex = i / aoChannelsPerColumn;
		int rowIndex = i % aoChannelsPerColumn + 1;
		xOffset = colIndex * 75 + 40;
		int y_pos = 5 + rowIndex * 26;

		AOButton* a = new AOButton(i, processor);
		a->setBounds(xOffset, y_pos, 15, 15);
		a->addListener(this);
		addAndMakeVisible(a);
		aoButtons.add(a);

		SOURCE_TYPE sourceType = processor->getSourceTypeForOutput(i);
		LOGD("Got source type for input ", i, ": ", sourceType);

		SourceTypeButton* b = new SourceTypeButton(i, processor, sourceType);
		b->setBounds(xOffset+17, y_pos-2, 27, 17);
		b->addListener(this);
		addAndMakeVisible(b);
		sourceTypeButtons.add(b);

	}

	doButtons.clear();

	// Draw digital inputs
	for (int i = 0; i < nDO; i++)
	{

		int colIndex = i / doChannelsPerColumn;
		int rowIndex = i % doChannelsPerColumn + 1;
		xOffset = ((nAO % maxChannelsPerColumn == 0 ? 0 : 1) + nAO / aoChannelsPerColumn) * 75 + 38 + colIndex * 45;
		int y_pos = 5 + rowIndex * 26;

		DOButton* b = new DOButton(i, processor);
		b->setBounds(xOffset, y_pos, 15, 15);
		b->addListener(this);
		addAndMakeVisible(b);
		doButtons.add(b);

	}

	xOffset = xOffset + 25 + 30 * (nDO == 0);

	deviceSelectBox = new ComboBox("DeviceSelectBox");
	deviceSelectBox->setBounds(xOffset, 39, 85, 20);
    deviceSelectBox->addListener(this);
    Array<NIDAQDevice*> devices = processor->getDevices();
	for (int i = 0; i < devices.size(); i++)
	{
		deviceSelectBox->addItem(devices[i]->productName, i + 1);
	}
    deviceSelectBox->setSelectedId(processor->getDeviceIndex() + 1, dontSendNotification);
    addAndMakeVisible(deviceSelectBox.get());

	if (processor->getDevices().size() == 1)	// disable device selection if only one device is available
		deviceSelectBox->setEnabled(false);

	sampleRateSelectBox = new ComboBox("SampleRateSelectBox");
	sampleRateSelectBox->setBounds(xOffset, 70, 85, 20);
	Array<NIDAQ::float64> sampleRates = processor->getSampleRates();
	for (int i = 0; i < sampleRates.size(); i++)
	{
		sampleRateSelectBox->addItem(String(sampleRates[i]) + " S/s", i + 1);
	}
	sampleRateSelectBox->setSelectedItemIndex(processor->getSampleRateIndex(), false);
	sampleRateSelectBox->addListener(this);
	addAndMakeVisible(sampleRateSelectBox);

	voltageRangeSelectBox = new ComboBox("VoltageRangeSelectBox");
	voltageRangeSelectBox->setBounds(xOffset, 101, 85, 20);
	Array<SettingsRange> voltageRanges = processor->getVoltageRanges();
	for (int i = 0; i < voltageRanges.size(); i++)
	{
		String rangeString = String(voltageRanges[i].min) + " - " + String(voltageRanges[i].max) + " V";
		voltageRangeSelectBox->addItem(rangeString, i + 1);
	}
	voltageRangeSelectBox->setSelectedItemIndex(processor->getVoltageRangeIndex(), false);
	voltageRangeSelectBox->addListener(this);
	addAndMakeVisible(voltageRangeSelectBox);

	//fifoMonitor = new FifoMonitor(thread);
	//fifoMonitor->setBounds(xOffset + 2, 105, 70, 12);
	//addAndMakeVisible(fifoMonitor);

	configureDeviceButton = new UtilityButton("...", Font("Small Text", 12, Font::plain));
	configureDeviceButton->setBounds(xOffset + 60, 25, 24, 12);
	configureDeviceButton->addListener(this);
	configureDeviceButton->setAlpha(0.5f);
	addAndMakeVisible(configureDeviceButton);
	
	desiredWidth = xOffset + 100;

	background = new EditorBackground(nAO, nDO);
	background->setBounds(0, 15, 1000, 150);
	addAndMakeVisible(background);
	background->toBack();
	background->repaint();

	//TODO: Why this line casuses crash in editor->update in v6?
	//setDisplayName("NIDAQmx-(" + processor->getProductName() + ")");
}

void NIDAQOutputEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == deviceSelector.get())
    {
        int deviceIndex = deviceSelector->getSelectedItemIndex();
        processor->setDevice(processor->getDevices()[deviceIndex]->getName());
        CoreServices::updateSignalChain(this);
    }
}

void NIDAQOutputEditor::buttonClicked(Button* button)
{
	buttonEvent(button);
}

void NIDAQOutputEditor::buttonEvent(Button* button)
{

    /*
	if (aoButtons.contains((AOButton*)button))
	{
		((AOButton*)button)->setEnabled(thread->toggleAIChannel(((AOButton*)button)->getId()));
		repaint();
	}
	else if (doButtons.contains((DOButton*)button))
	{
		((DOButton*)button)->setEnabled(thread->toggleDIChannel(((DOButton*)button)->getId()));
		repaint();
	}
	else if (sourceTypeButtons.contains((SourceTypeButton*)button))
	{
		int currentButtonId = ((SourceTypeButton*)button)->getId();
		thread->toggleSourceType(currentButtonId);
		SOURCE_TYPE next = thread->getSourceTypeForInput(currentButtonId);
		((SourceTypeButton*)button)->update(next);
		repaint();
	}
	else if (button == configureDeviceButton)
	{
		if (!thread->isThreadRunning())
		{

			currentConfigWindow = new PopupConfigurationWindow(this);

			CallOutBox& myBox
				= CallOutBox::launchAsynchronously(std::unique_ptr<Component>(currentConfigWindow), 
					button->getScreenBounds(),
					nullptr);

			myBox.setDismissalMouseClicksAreAlwaysConsumed(true);
			
			return;

		}
	}
    */
}

void NIDAQOutputEditor::updateDevice(String deviceName)
{
    for (int i = 0; i < deviceSelectBox->getNumItems(); i++)
    {
        if (deviceSelectBox->getItemText(i).equalsIgnoreCase(deviceName))
            deviceSelectBox->setSelectedId(deviceSelectBox->getItemId(i), dontSendNotification);
    }
}
