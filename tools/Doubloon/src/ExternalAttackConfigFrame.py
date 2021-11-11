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

import config
from util import *
from State import *
from Events import *



# Window with options to configure external attack
class ExternalAttackConfigFrame(wx.Frame):
    def __init__(self, state):
        super(ExternalAttackConfigFrame, self).__init__(None, style=wx.DEFAULT_FRAME_STYLE - (wx.MINIMIZE_BOX | wx.MAXIMIZE_BOX | wx.RESIZE_BORDER))
                
        # Copy of the common attack state from application
        self.state = state

        # Setup window
        self.SetTitle("External attack config")
        self.SetClientSize((480,320))

        # Setup UI
        sizer = wx.GridBagSizer(5,5)
        self.panel = wx.Panel(self)
        self.sizer = sizer

        self.notebook = wx.Notebook(self.panel)
        sizer.Add(self.notebook, pos=(1,1), span=(1,3))
        
        # General attack UI
        generalPanel = wx.Panel(self.notebook)
        generalSizer = wx.GridBagSizer(5,5)

        label = wx.StaticText(generalPanel, -1, "Attack program:")
        generalSizer.Add(label, pos=(1,1))
        self.attackProgram = wx.TextCtrl(generalPanel, size=(350,config.textboxHeight))
        generalSizer.Add(self.attackProgram, pos=(1,2), flag=wx.TOP|wx.LEFT)
        self.browseAttack = wx.Button(generalPanel, label="Browse")
        generalSizer.Add(self.browseAttack, pos=(1,3), flag=wx.TOP|wx.RIGHT)

        label = wx.StaticText(generalPanel, -1, "Attack arguments:")
        generalSizer.Add(label, pos=(2,1))
        self.attackArgs = wx.TextCtrl(generalPanel, size=(350,config.textboxHeight))
        generalSizer.Add(self.attackArgs, pos=(2,2), span=(1,2), flag=wx.TOP|wx.LEFT)
        
        generalPanel.SetSizer(generalSizer)
        generalSizer.Fit(generalPanel)
        self.notebook.InsertPage(0, generalPanel, "General", select=True)

        # FR-Trace attack UI
        frPanel = wx.Panel(self.notebook)
        frSizer = wx.GridBagSizer(5,5)
        
        label = wx.StaticText(frPanel, -1, "FR-trace program:")
        frSizer.Add(label, pos=(1,1))
        self.frProgram = wx.TextCtrl(frPanel, size=(350,config.textboxHeight))
        frSizer.Add(self.frProgram, pos=(1,2), flag=wx.TOP|wx.LEFT)
        self.browseFR = wx.Button(frPanel, label="Browse")
        frSizer.Add(self.browseFR, pos=(1,3), flag=wx.TOP|wx.RIGHT)

        label = wx.StaticText(frPanel, -1, "Monitor locations:")
        frSizer.Add(label, pos=(2,1))
        self.frMonitor = wx.TextCtrl(frPanel, size=(350,config.textboxHeight))
        frSizer.Add(self.frMonitor, pos=(2,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "Evict locations:")
        frSizer.Add(label, pos=(3,1))
        self.frEvict = wx.TextCtrl(frPanel, size=(350,config.textboxHeight))
        frSizer.Add(self.frEvict, pos=(3,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "PDA locations:")
        frSizer.Add(label, pos=(4,1))
        self.frPDA = wx.TextCtrl(frPanel, size=(350,config.textboxHeight))
        frSizer.Add(self.frPDA, pos=(4,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "Slot length:")
        frSizer.Add(label, pos=(5,1))
        self.frSlotlen = wx.TextCtrl(frPanel, size=(150,config.textboxHeight))
        frSizer.Add(self.frSlotlen, pos=(5,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "Max samples:")
        frSizer.Add(label, pos=(6,1))
        self.frMaxSamples = wx.TextCtrl(frPanel, size=(150,config.textboxHeight))
        frSizer.Add(self.frMaxSamples, pos=(6,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "Threshold:")
        frSizer.Add(label, pos=(7,1))
        self.frThreshold = wx.TextCtrl(frPanel, size=(150,config.textboxHeight))
        frSizer.Add(self.frThreshold, pos=(7,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "Idle count:")
        frSizer.Add(label, pos=(8,1))
        self.frIdle = wx.TextCtrl(frPanel, size=(150,config.textboxHeight))
        frSizer.Add(self.frIdle, pos=(8,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(frPanel, -1, "PDA count:")
        frSizer.Add(label, pos=(9,1))
        self.frPDACount = wx.TextCtrl(frPanel, size=(150,config.textboxHeight))
        frSizer.Add(self.frPDACount, pos=(9,2), flag=wx.TOP|wx.LEFT)

        # self.frHeader = wx.CheckBox(frPanel, label="Include header")
        # frSizer.Add(self.frHeader, pos=(10,2), span=(1,3))
     
        frPanel.SetSizer(frSizer)
        frSizer.Fit(frPanel)
        self.notebook.InsertPage(1, frPanel, "FR-trace")

        # Standard UI
        self.victimRun = wx.CheckBox(self.panel, label="Run victim program")
        sizer.Add(self.victimRun, pos=(2,2), span=(1,3))

        label = wx.StaticText(self.panel, -1, "Victim program:")
        sizer.Add(label, pos=(3,1))
        self.victimProgram = wx.TextCtrl(self.panel, size=(400,config.textboxHeight))
        sizer.Add(self.victimProgram, pos=(3,2), flag=wx.TOP|wx.LEFT)
        self.browseVictim = wx.Button(self.panel, label="Browse")
        sizer.Add(self.browseVictim, pos=(3,3), flag=wx.TOP|wx.RIGHT)

        label = wx.StaticText(self.panel, -1, "Victim arguments:")
        sizer.Add(label, pos=(4,1))
        self.victimArgs = wx.TextCtrl(self.panel, size=(400,config.textboxHeight))
        sizer.Add(self.victimArgs, pos=(4,2), span=(1,2), flag=wx.TOP|wx.LEFT)
        
        self.sshRun = wx.CheckBox(self.panel, label="Run remotely (SSH)")
        sizer.Add(self.sshRun, pos=(5,2), span=(1,3))

        label = wx.StaticText(self.panel, -1, "SSH hostname:")
        sizer.Add(label, pos=(6,1))
        self.sshHostname = wx.TextCtrl(self.panel, size=(200,config.textboxHeight))
        sizer.Add(self.sshHostname, pos=(6,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(self.panel, -1, "SSH port:")
        sizer.Add(label, pos=(7,1))
        self.sshPort = wx.TextCtrl(self.panel, size=(200,config.textboxHeight))
        sizer.Add(self.sshPort, pos=(7,2), flag=wx.TOP|wx.LEFT)
        
        label = wx.StaticText(self.panel, -1, "SSH username:")
        sizer.Add(label, pos=(8,1))
        self.sshUsername = wx.TextCtrl(self.panel, size=(200,config.textboxHeight))
        sizer.Add(self.sshUsername, pos=(8,2), flag=wx.TOP|wx.LEFT)

        label = wx.StaticText(self.panel, -1, "SSH password:")
        sizer.Add(label, pos=(9,1))
        self.sshPassword = wx.TextCtrl(self.panel, size=(200,config.textboxHeight), style=wx.TE_PASSWORD)
        sizer.Add(self.sshPassword, pos=(9,2), flag=wx.TOP|wx.LEFT)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        # Sizing
        self.SetClientSize(20+self.panel.GetClientSize().Width,20+self.panel.GetClientSize().Height)
        
        # Event bindings
        self.Bind(wx.EVT_CLOSE, self.onClose)
        self.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.onNotebookChange, self.notebook)

        self.frProgram.Bind(wx.EVT_TEXT, self.onConfig)
        self.browseFR.Bind(wx.EVT_BUTTON, self.onBrowseFR)
        self.frMonitor.Bind(wx.EVT_TEXT, self.onConfig)
        self.frEvict.Bind(wx.EVT_TEXT, self.onConfig)
        self.frPDA.Bind(wx.EVT_TEXT, self.onConfig)
        self.frSlotlen.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.frSlotlen.Bind(wx.EVT_TEXT, self.onConfig)
        self.frMaxSamples.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.frMaxSamples.Bind(wx.EVT_TEXT, self.onConfig)
        self.frThreshold.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.frThreshold.Bind(wx.EVT_TEXT, self.onConfig)
        self.frIdle.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.frIdle.Bind(wx.EVT_TEXT, self.onConfig)
        self.frPDACount.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.frPDACount.Bind(wx.EVT_TEXT, self.onConfig)
        # self.frHeader.Bind(wx.EVT_CHECKBOX, self.onConfig)
        
        self.attackProgram.Bind(wx.EVT_TEXT, self.onConfig)
        self.browseAttack.Bind(wx.EVT_BUTTON, self.onBrowseGeneral)
        self.attackArgs.Bind(wx.EVT_TEXT, self.onConfig)

        self.victimProgram.Bind(wx.EVT_TEXT, self.onConfig)
        self.victimArgs.Bind(wx.EVT_TEXT, self.onConfig)
        self.browseVictim.Bind(wx.EVT_BUTTON, self.onBrowseVictim)
        self.victimRun.Bind(wx.EVT_CHECKBOX, self.onConfig)
        self.sshRun.Bind(wx.EVT_CHECKBOX, self.onConfig)
        self.sshHostname.Bind(wx.EVT_TEXT, self.onConfig)
        self.sshPort.Bind(wx.EVT_TEXT, self.onConfig)
        self.sshPort.Bind(wx.EVT_CHAR, self.onLimitChar)
        self.sshUsername.Bind(wx.EVT_TEXT, self.onConfig)
        self.sshPassword.Bind(wx.EVT_TEXT, self.onConfig)

        self.setConfig()


    # Ask the user for a file and fill in the text box
    def onBrowseGeneral(self, event):
        with wx.FileDialog(self, "Select program", wildcard="*", style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()

            # Fill in the corresponding objects textbox
            self.attackProgram.ChangeValue(pathname)
            self.onConfig(None)

    def onBrowseFR(self, event):
        with wx.FileDialog(self, "Select program", wildcard="*", style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()

            # Fill in the corresponding objects textbox
            self.frProgram.ChangeValue(pathname)
            self.onConfig(None)

    def onBrowseVictim(self, event):
        with wx.FileDialog(self, "Select program", wildcard="*", style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()

            # Fill in the corresponding objects textbox
            self.victimProgram.ChangeValue(pathname)
            self.onConfig(None)

    # Open window
    def open(self):
        self.setConfig()
        self.Show()


    # Close window on cross out
    def onClose(self, event):
        self.onConfig(None)
        self.Hide()


    # Limit input of numbers to digits
    def onLimitChar(self, event):
        key = event.GetKeyCode()
        if (chr(key).isdigit() or key == wx.WXK_LEFT or key == wx.WXK_RIGHT or key == wx.WXK_BACK or key == wx.WXK_RETURN or key == wx.WXK_ESCAPE or key == wx.WXK_TAB):
            event.Skip(True)


    # When attack UI ExternalAttackState is edited, update attack state
    def onConfig(self, event):
        if (self.IsShown()):
            self.state.attackProgram = self.attackProgram.GetValue()
            self.state.attackArgs = self.attackArgs.GetValue()

            if (self.frSlotlen.GetValue() == ""):
                self.frSlotlen.ChangeValue("0")
            if (self.frMaxSamples.GetValue() == ""):
                self.frMaxSamples.ChangeValue("0")
            if (self.frThreshold.GetValue() == ""):
                self.frThreshold.ChangeValue("0")
            if (self.frIdle.GetValue() == ""):
                self.frIdle.ChangeValue("0")
            if (self.frPDACount.GetValue() == ""):
                self.frPDACount.ChangeValue("0")
            if (self.sshPort.GetValue() == ""):
                self.sshPort.ChangeValue("22")

            self.state.frProgram = self.frProgram.GetValue()
            self.state.frMonitor = self.frMonitor.GetValue()
            self.state.frEvict = self.frEvict.GetValue()
            self.state.frPDA = self.frPDA.GetValue()
            self.state.frSlotlen = int(self.frSlotlen.GetValue())
            self.state.frMaxSamples = int(self.frMaxSamples.GetValue())
            self.state.frThreshold = int(self.frThreshold.GetValue())
            self.state.frIdle = int(self.frIdle.GetValue())
            self.state.frPDACount = int(self.frPDACount.GetValue())
            # self.state.frHeader = self.frHeader.IsChecked()

            self.state.victimProgram = self.victimProgram.GetValue()
            self.state.victimArgs = self.victimArgs.GetValue()
            self.state.victimRun = self.victimRun.IsChecked()
            self.state.sshRun = self.sshRun.IsChecked()
            self.state.sshHostname = self.sshHostname.GetValue()
            self.state.sshPort = int(self.sshPort.GetValue())
            self.state.sshUsername = self.sshUsername.GetValue()
            self.state.sshPassword = self.sshPassword.GetValue()


    # If notebook page change, type of attack changed
    def onNotebookChange(self, event):
        self.state.type = self.notebook.GetSelection()
        self.onConfig(None)


    # Set UI from attack state
    def setConfig(self):
        self.setting = True
        self.notebook.SetSelection(self.state.type)
        self.attackProgram.ChangeValue(self.state.attackProgram)
        self.attackArgs.ChangeValue(self.state.attackArgs)

        self.frProgram.ChangeValue(self.state.frProgram)
        self.frMonitor.ChangeValue(self.state.frMonitor)
        self.frEvict.ChangeValue(self.state.frEvict)
        self.frPDA.ChangeValue(self.state.frPDA)
        self.frSlotlen.ChangeValue(str(self.state.frSlotlen))
        self.frMaxSamples.ChangeValue(str(self.state.frMaxSamples))
        self.frThreshold.ChangeValue(str(self.state.frThreshold))
        self.frIdle.ChangeValue(str(self.state.frIdle))
        self.frPDACount.ChangeValue(str(self.state.frPDACount))
        # self.frHeader.SetValue(self.state.frHeader)
            
        self.victimRun.SetValue(self.state.victimRun)
        self.victimProgram.ChangeValue(self.state.victimProgram)
        self.victimArgs.ChangeValue(self.state.victimArgs)
        self.sshRun.SetValue(self.state.sshRun)
        self.sshHostname.ChangeValue(self.state.sshHostname)
        self.sshPort.ChangeValue(str(self.state.sshPort))
        self.sshUsername.ChangeValue(self.state.sshUsername)
        self.sshPassword.ChangeValue(self.state.sshPassword)



      