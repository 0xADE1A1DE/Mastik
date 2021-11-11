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
import config as cfg
from util import *
from Events import *



# Toolbar view with common application functions
class ToolbarView(wx.ToolBar):
    def __init__(self, parent):
        super(ToolbarView, self).__init__(parent, -1)
        self.SetToolBitmapSize(wx.Size(32,32))

        # Setup toolbar items
        self.runTool = self.AddTool(101, "Run", wx.Bitmap(cfg.getResource(cfg.RES_ICON_PLAY)))
        self.Bind(wx.EVT_TOOL, self.onRunExternalAttack, self.runTool)
        
        self.configTool = self.AddTool(102, "Config", wx.Bitmap(cfg.getResource(cfg.RES_ICON_CONFIG))) 
        self.Bind(wx.EVT_TOOL, self.onOpenExternalAttackConfig, self.configTool)
        
        if (cfg.dwarfSupport and cfg.experimental):
            self.AddSeparator()
            self.assemblyTool = self.AddTool(482, "Open Assembly", wx.Bitmap(wx.Bitmap(cfg.getResource(cfg.RES_ICON_X86)))) 
            self.Bind(wx.EVT_TOOL, self.onOpenAssemblyFrame, self.assemblyTool)

        self.AddSeparator()

        self.loadStateTool = self.AddTool(201, "Load State", wx.Bitmap(cfg.getResource(cfg.RES_ICON_OPEN))) 
        self.Bind(wx.EVT_TOOL, self.onLoadState, self.loadStateTool)
        
        self.saveStateTool = self.AddTool(202, "Save State", wx.Bitmap(cfg.getResource(cfg.RES_ICON_SAVE))) 
        self.Bind(wx.EVT_TOOL, self.onSaveState, self.saveStateTool)
        
        self.AddSeparator()

        self.loadDataTool = self.AddTool(301, "Load Data", wx.Bitmap(cfg.getResource(cfg.RES_ICON_IMPORT))) 
        self.Bind(wx.EVT_TOOL, self.onLoadData, self.loadDataTool)
        
        self.saveDataTool = self.AddTool(302, "Save Data", wx.Bitmap(cfg.getResource(cfg.RES_ICON_EXPORT))) 
        self.Bind(wx.EVT_TOOL, self.onSaveData, self.saveDataTool)
    
        self.Realize()


    # Raise run attack event and switch icon to cancel
    def onRunExternalAttack(self, event):
        evt = runExternalAttackEvent()
        wx.PostEvent(self, evt)
        self.DeleteTool(101)
        self.runTool = self.InsertTool(0, 101, "Cancel", wx.Bitmap(cfg.getResource(cfg.RES_ICON_STOP))) 
        self.Realize()
        self.Bind(wx.EVT_TOOL, self.onRunExternalAttack, self.runTool)

    
    # When attack is done/cancelled, switch icon to run
    def onExternalAttackDone(self, event):
        self.DeleteTool(101)
        self.runTool = self.InsertTool(0, 101, "Run", wx.Bitmap(cfg.getResource(cfg.RES_ICON_PLAY))) 
        self.Realize()
        self.Bind(wx.EVT_TOOL, self.onRunExternalAttack, self.runTool)

        
    # Raise config attack event to open config frame
    def onOpenExternalAttackConfig(self, event):
        evt = openExternalAttackConfigEvent()
        wx.PostEvent(self, evt)


    # Notify application to load data file
    def onLoadData(self, event):
        evt = loadDataEvent()
        wx.PostEvent(self, evt)


    # Notify application to save data file
    def onSaveData(self, event):
        evt = saveDataEvent()
        wx.PostEvent(self, evt)

    
    # Notify application to load state file
    def onLoadState(self, event):
        evt = loadStateEvent()
        wx.PostEvent(self, evt)


    # Notify application to save state file
    def onSaveState(self, event):
        evt = saveStateEvent()
        wx.PostEvent(self, evt)
        
    # Open assembly view
    def onOpenAssemblyFrame(self, event):
        evt = openAssemblyFrameEvent()
        wx.PostEvent(self, evt)
    
