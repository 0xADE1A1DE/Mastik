"""
Copyright 2018 Dallas McNeil

This file is part of Mastik.
Mastik is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mastik is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mastik.  If not, see <http://www.gnu.org/licenses/>.


Author: Dallas McNeil a1724759@student.adelaide.edu.au
"""


import wx
import math

import config
from util import *
from Events import *
from State import *
from matplotlib import pyplot as plt
import numpy as np



# Provide graph settings
class PropertiesView(wx.ScrolledWindow):
    def __init__(self, parent, graphState):
        super(PropertiesView, self).__init__(parent, style=wx.VSCROLL)

        # TODO: View should scroll vertically but current isn't set up to do so

        # Copy of the common graph state from application
        self.graphState = graphState

        # Panels and sizers
        self.statsPanel = wx.Panel(self)
        self.propsPanel = wx.Panel(self)
        self.maskPanel = wx.Panel(self)

        psizer = wx.GridBagSizer(6,5)
        ssizer = wx.GridBagSizer(5,5)
        msizer = wx.GridBagSizer(5,5)
        self.propsSizer = psizer
        self.statsSizer = ssizer
        self.maskSizer = msizer

        # Statistics UI
        self.statsCount = wx.StaticText(self.statsPanel, -1, "Result count: 1000000")
        ssizer.Add(self.statsCount, pos=(0,0))
        self.statsActive = wx.StaticText(self.statsPanel, -1, "Active: 100%")
        ssizer.Add(self.statsActive, pos=(1,0))

        # Properties UI
        line = wx.StaticLine(self.propsPanel)
        psizer.Add(line, pos=(0, 0), span=(1,3), flag=wx.EXPAND|wx.TOP)

        if (config.experimental):
            self.graphTypeText = wx.StaticText(self.propsPanel, -1, "Type:")
            psizer.Add(self.graphTypeText, pos=(1,0))
            self.graphChoice = wx.Choice(self.propsPanel, choices=["Line", "Map"])
            psizer.Add(self.graphChoice, pos=(1,1), span=(1,3))
            self.graphChoice.Bind(wx.EVT_CHOICE, self.onGraphProps)

        xLimit = wx.StaticText(self.propsPanel, -1, "X limits:")
        psizer.Add(xLimit, pos=(2,0))
        self.xMinText = wx.TextCtrl(self.propsPanel, size=(60,config.textboxHeight))
        psizer.Add(self.xMinText, pos=(2,1), flag=wx.TOP|wx.LEFT)
        self.xMinText.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.xMinText.Bind(wx.EVT_KILL_FOCUS, self.onGraphProps)
        self.xMaxText = wx.TextCtrl(self.propsPanel, size=(60,config.textboxHeight))
        psizer.Add(self.xMaxText, pos=(2,2), flag=wx.TOP|wx.LEFT)
        self.xMaxText.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.xMaxText.Bind(wx.EVT_KILL_FOCUS, self.onGraphProps)

        yLimit = wx.StaticText(self.propsPanel, -1, "Y limits:")
        psizer.Add(yLimit, pos=(3,0))
        self.yMinText = wx.TextCtrl(self.propsPanel, size=(60,config.textboxHeight))
        psizer.Add(self.yMinText, pos=(3,1), flag=wx.TOP|wx.LEFT)
        self.yMinText.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.yMinText.Bind(wx.EVT_KILL_FOCUS, self.onGraphProps)
        self.yMaxText = wx.TextCtrl(self.propsPanel, size=(60,config.textboxHeight))
        psizer.Add(self.yMaxText, pos=(3,2), flag=wx.TOP|wx.LEFT)
        self.yMaxText.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.yMaxText.Bind(wx.EVT_KILL_FOCUS, self.onGraphProps)

        threshold = wx.StaticText(self.propsPanel, -1, "Threshold:")
        psizer.Add(threshold, pos=(4,0))
        self.thresholdText = wx.TextCtrl(self.propsPanel, size=(100,config.textboxHeight))
        psizer.Add(self.thresholdText, pos=(4,1), span=(1,2), flag=wx.TOP|wx.LEFT)
        self.thresholdText.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.thresholdText.Bind(wx.EVT_KILL_FOCUS, self.onGraphProps)

        self.binaryCheckbox = wx.CheckBox(self.propsPanel, label="Threshold binary")
        psizer.Add(self.binaryCheckbox, pos=(5, 0), span=(1,3), flag=wx.TOP|wx.LEFT)
        self.Bind(wx.EVT_CHECKBOX, self.onGraphProps, self.binaryCheckbox)
        
        self.interpolateCheckbox = wx.CheckBox(self.propsPanel, label="Interpolate missing values")
        psizer.Add(self.interpolateCheckbox, pos=(6, 0), span=(1,3), flag=wx.TOP|wx.LEFT)        
        self.Bind(wx.EVT_CHECKBOX, self.onGraphProps, self.interpolateCheckbox)

        if (config.experimental):
            self.exportButton = wx.Button(self.propsPanel, label = "Export")
            psizer.Add(self.exportButton, pos=(7, 0), span=(1,4), flag=wx.TOP|wx.LEFT)        
            self.Bind(wx.EVT_BUTTON, self.onExport, self.exportButton)
        
        self.maskCheckboxes = []

        # Setup sizers and panels
        self.statsPanel.SetSizer(ssizer)
        ssizer.Fit(self.statsPanel)
        self.propsPanel.SetSizer(psizer)
        psizer.Fit(self.propsPanel)
        self.maskPanel.SetSizer(self.maskSizer)
        self.maskSizer.Fit(self.maskPanel)

        self.statsPanel.SetSize(10, 10, config.propertiesWidth - 20, self.statsPanel.GetClientSize().Height)
        self.propsPanel.SetSize(10, 15 + self.statsPanel.GetClientSize().Height, config.propertiesWidth - 20, self.propsPanel.GetClientSize().Height)
        self.maskPanel.SetSize(10, 20 + self.statsPanel.GetClientSize().Height + self.propsPanel.GetClientSize().Height, config.propertiesWidth - 20, self.maskPanel.GetClientSize().Height)
        
        self.setGraphProps()
        self.hidePanels()

    
    # Calculate and set statistics UI
    def setStats(self):
        count = 0
        activeCount = 0
        for i in range(0, self.graphState.data.shape[0]):
            for j in range(0, self.graphState.data.shape[1]):
                if (not math.isnan(self.graphState.data[i][j])):
                    count = count + 1
                if (self.graphState.data[i][j] <= self.graphState.threshold):
                    activeCount = activeCount + 1

        self.statsCount.SetLabelText(str(count) + " results")
        self.statsActive.SetLabelText(str(int(activeCount/count*10000)/100) + "% active")


    # Limit x and y limit inputs to numbers
    def onLimitChar(self, event):
        key = event.GetKeyCode()
        if (chr(key).isdigit() or key == wx.WXK_LEFT or key == wx.WXK_RIGHT or key == wx.WXK_BACK or key == wx.WXK_RETURN or key == wx.WXK_ESCAPE or key == wx.WXK_TAB):
            event.Skip(True)
        if (key == wx.WXK_RETURN):
            self.onGraphProps(None)

    # On a properties change, update graph state
    def onGraphProps(self, event):
        if (self.propsPanel.IsShown()):
            if (self.xMinText.GetValue() == ""):
                self.xMinText.ChangeValue("0")
            if (self.xMaxText.GetValue() == ""):
                self.xMaxText.ChangeValue("0")
            if (self.yMinText.GetValue() == ""):
                self.yMinText.ChangeValue("0")
            if (self.yMaxText.GetValue() == ""):
                self.yMaxText.ChangeValue("0")
            if (self.thresholdText.GetValue() == ""):
                self.thresholdText.ChangeValue("0")

            self.graphState.threshold = int(self.thresholdText.GetValue())
            self.graphState.thresholdBinary = self.binaryCheckbox.IsChecked()
            self.graphState.interpolateMissing = self.interpolateCheckbox.IsChecked()
            self.graphState.xLimit = [int(self.xMinText.GetValue()), int(self.xMaxText.GetValue())]
            self.graphState.yLimit = [int(self.yMinText.GetValue()), int(self.yMaxText.GetValue())]
            
            if (config.experimental):
                self.graphState.type = self.graphChoice.GetSelection()

            # Set graph mask 
            for i in range(0,len(self.maskCheckboxes),1):
                self.graphState.mask[i] = self.maskCheckboxes[i].IsChecked()

            evt = graphPropsUpdateEvent()
            wx.PostEvent(self, evt)
            self.setStats()


    # Set graph properties UI elements
    def setGraphProps(self):
        self.binaryCheckbox.SetValue(self.graphState.thresholdBinary)
        self.interpolateCheckbox.SetValue(self.graphState.interpolateMissing)
        self.thresholdText.ChangeValue(str(self.graphState.threshold))
        self.xMinText.ChangeValue(str(self.graphState.xLimit[0]))
        self.xMaxText.ChangeValue(str(self.graphState.xLimit[1]))
        self.yMinText.ChangeValue(str(self.graphState.yLimit[0]))
        self.yMaxText.ChangeValue(str(self.graphState.yLimit[1]))  

        if (config.experimental):
            self.graphChoice.SetSelection(self.graphState.type)

        if (len(self.graphState.mask) != len(self.maskCheckboxes)):
            # Remove mask checkboxes
            self.maskSizer.Clear()
            for i in range(0,len(self.maskCheckboxes),1):
                self.maskCheckboxes[-1].Destroy()
                self.maskCheckboxes.pop()

            line = wx.StaticLine(self.maskPanel)
            self.maskSizer.Add(line, pos=(0, 0), flag=wx.EXPAND|wx.TOP)

            # Add mask checkboxes
            for i in range(0,len(self.graphState.mask),1):
                if len(self.graphState.monitorLocations) > 0:
                    self.maskCheckboxes.append(wx.CheckBox(self.maskPanel, label=self.graphState.monitor[i]))
                else:
                    self.maskCheckboxes.append(wx.CheckBox(self.maskPanel, label=("Set " + str(i+1))))
                self.maskSizer.Add(self.maskCheckboxes[-1], pos=(i+1, 0))        
                self.Bind(wx.EVT_CHECKBOX, self.onGraphProps, self.maskCheckboxes[-1])
                self.maskCheckboxes[i].SetValue(self.graphState.mask[i])
                self.maskCheckboxes[i].SetBackgroundColour(config.graphColours[i%len(config.graphColours)])
                
        else:
            # Set mask checkboxes
            for i in range(0,len(self.maskCheckboxes),1):
                self.maskCheckboxes[i].SetValue(self.graphState.mask[i])

        self.maskPanel.Layout()
        self.maskSizer.Fit(self.maskPanel)
        self.maskPanel.SetSize(10, 20 + self.statsPanel.GetClientSize().Height + self.propsPanel.GetClientSize().Height, config.propertiesWidth - 20, self.maskPanel.GetClientSize().Height)

    # Export graph to matplot
    def onExport(self, event):
        # TODO: Export is buggy and requires testing. 
        step = 20
        base = 3
        numLoc = len(self.graphState.monitorLocations)
        data = np.array(self.graphState.data).transpose()
        fig = plt.figure()
        ax = fig.add_subplot(111)
        if self.graphState.thresholdBinary:
            plt.ylim(self.graphState.threshold - step * (numLoc+base),self.graphState.threshold + step * (numLoc+base))
            for i in range(data.shape[0]):
                for j in range(data.shape[1]):
                    if data[i][j] >= self.graphState.threshold:
                        data[i][j] = step*(base+i) + self.graphState.threshold
                    else:
                        data[i][j] = -step*(base+i) + self.graphState.threshold

        #plt.ylim(0, 3* self.graphState.threshold)
        plt.xlabel('time slot')
        plt.ylabel('access time')
        cm = plt.get_cmap('gist_rainbow')
        ax.set_prop_cycle(color = [cm(1.*i/numLoc) for i in range(numLoc)])
        #plt.plot([1,2],[3,4], 'r--', label = 'mul')
        for i in range(numLoc):
            ax.plot(data[i],'-',label = self.graphState.monitorLocations[i])

        #fig.savefig('moreColors.png')
        plt.legend()
        plt.show()

    # Hide all panels
    def hidePanels(self):
        self.propsPanel.Hide()
        self.statsPanel.Hide()
        self.maskPanel.Hide()


    # Show all panels
    def showPanels(self):
        self.statsPanel.Show()
        self.propsPanel.Show()
        self.maskPanel.Show()
        self.statsPanel.Raise()
        self.propsPanel.Raise()
        self.maskPanel.Raise()
        self.Layout()
