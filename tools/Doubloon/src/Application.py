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
import numpy
import json
import time

import config
from util import *
from State import *
from Events import *
from ExternalAttack import *
from GraphView import *
from ToolbarView import *
from PropertiesView import *
from ExternalAttackConfigFrame import *
from Dump import *
from AssemblyFrame import *



# Main application, holds all views and data, coordinates everything
class Application(wx.App):
    def __init__(self):
        super(Application, self).__init__(False)

        # Application state
        # These are passed by reference to other objects so there is only one shared state across the application
        self.externalAttackState = ExternalAttackState()
        self.graphState = GraphState()

        # Setup external attack object
        self.externalAttack = ExternalAttack(self.externalAttackState)

        #setup assembly view object
        self.dump = Dump(self.externalAttackState)

        # Setup main window
        self.mainFrame = wx.Frame(None)
        self.mainFrame.SetTitle(config.applicationName)
        self.mainFrame.SetClientSize(config.defaultWindowSize)
        self.mainFrame.Center()

        # Setup attack config window
        self.externalAttackConfigFrame = ExternalAttackConfigFrame(self.externalAttackState)

        ## Setup machine code view
        self.assemblyFrame = AssemblyFrame(self.externalAttackState)

        # Create views
        self.graph = GraphView(self.mainFrame, self.graphState)
        self.toolbar = ToolbarView(self.mainFrame)
        self.properties = PropertiesView(self.mainFrame, self.graphState)
        
        # Setup views
        self.positionViews()
        self.updatePropsFromGraph()

        # Bind events
        self.mainFrame.Bind(wx.EVT_CLOSE, self.onClose)
        self.mainFrame.Bind(wx.EVT_SIZE, self.onSize)
        self.Bind(EVT_RUN_EXTERNAL_ATTACK, self.onRunExternalAttack)
        self.Bind(EVT_EXTERNAL_ATTACK_DONE, self.onExternalAttackDone)
        self.Bind(EVT_UPDATE_LOCATION, self.onUpdateLocation)
        self.Bind(EVT_OPEN_EXTERNAL_ATTACK_CONFIG, self.onOpenExternalAttackConfig)
        self.Bind(EVT_OPEN_ASSEMBLY, self.onOpenAssemblyFrame)
        self.Bind(EVT_RUN_DUMP, self.onRunDump)
        self.Bind(EVT_DUMP_DONE, self.onInsertDump)
        self.Bind(EVT_GRAPH_PROPS_UPDATE, self.onGraphPropsUpdate)
        self.Bind(EVT_GRAPH_UPDATE, self.onGraphUpdate)
        self.Bind(EVT_LOAD_DATA, self.onLoadData)
        self.Bind(EVT_SAVE_DATA, self.onSaveData)
        self.Bind(EVT_LOAD_STATE, self.onLoadState)
        self.Bind(EVT_SAVE_STATE, self.onSaveState)
        
        # Shortcut events
        self.Bind(wx.EVT_KEY_DOWN, self.onButton)

    # Start application
    def run(self):
        self.mainFrame.Show()
        self.MainLoop()    


    # On window resize, should resize views
    def onSize(self, event):
        self.positionViews()

    
    # On close, should quit the application 
    def onClose(self, event):
        self.ExitMainLoop()


    # Positions all views in main window
    def positionViews(self):
        size = self.mainFrame.GetClientSize()
        heightOffset = 40
        widthOffset = config.propertiesWidth

        self.toolbar.SetSize(0, 0, size.Width, heightOffset)
        if (config.propertiesAlignment == "right"):
            self.graph.SetSize(0, heightOffset, size.Width - widthOffset, size.Height - heightOffset)
            self.properties.SetSize(size.Width - widthOffset, heightOffset, widthOffset, size.Height - heightOffset)
        else:
            self.graph.SetSize(widthOffset, heightOffset, size.Width - widthOffset, size.Height - heightOffset)
            self.properties.SetSize(0, heightOffset, widthOffset, size.Height - heightOffset)

    # On button press, handle shortcut
    def onButton(self, event):
        if (type(event.GetEventObject()) != wx._core.TextCtrl):
            if (event.GetKeyCode() == wx.WXK_UP):
                self.graph.zoom(1.0/config.graphZoomFactor)
            elif (event.GetKeyCode() == wx.WXK_DOWN):
                self.graph.zoom(config.graphZoomFactor)
            elif (event.GetKeyCode() == wx.WXK_LEFT):
                self.graph.shift(-config.graphShiftFactor)
            elif (event.GetKeyCode() == wx.WXK_RIGHT):
                self.graph.shift(config.graphShiftFactor)
        
        # Keep proppigating event
        event.Skip(True)


    # External attack functions
    
    # Should run the attack
    def onRunExternalAttack(self, event):
        #self.Pro = self.externalAttackState.frMonitor
        self.runExternalAttack()

    # Should run objdump
    def onRunDump(self, event):
        self.runDump()


    # Run the external attack, or cancel it if already running
    def runExternalAttack(self):
        if (self.externalAttack.running):
            self.toolbar.onExternalAttackDone(None)
            self.externalAttack.cancel()
        else:
            self.externalAttack.run()

    # Run objdump
    def runDump(self):
        self.assemblyFrame.clear()
        self.dump.run()
        self.assemblyFrame.open()

    # 
    def onInsertDump(self, event):
        print("try insert result")
        if(not self.dump.inserted):
            if(self.dump.hasResults):
                self.insertDump()
        else:
            print("already inserted, just show")

    # Insert dumped item into listctrl
    def insertDump(self):
        print("--try insert-----")
        if(self.dump.hasResults):
            print("--insert dump result into list-----")
            self.assemblyFrame.insert(self.dump.results)
        if(self.dump.hasMapResults):
            print("---insert map result into list-----")
            self.assemblyFrame.insertSource(self.dump.sourceCode)
            self.assemblyFrame.s2m = self.dump.s2m
            self.assemblyFrame.m2s = self.dump.m2s
        self.assemblyFrame.initMonitorList()
        print("--insert finished--")
    
    # On external attack done
    def onExternalAttackDone(self, event):
        self.toolbar.onExternalAttackDone(None)
        if (self.externalAttack.hasResults):
            if (self.externalAttackState.type == EXTERNAL_ATTACK_TYPE_FRTRACE):
                self.graphState.threshold = self.externalAttackState.frThreshold
            self.graphState.data = self.externalAttack.results
            self.graphState.monitorLocations = self.externalAttackState.frMonitor.split()
            #self.Properties.graphState = self.graphState #update metadata
            self.graphData()

    def onUpdateLocation(self, event):
        print("update location in application")
        self.externalAttack.state.frMonitor = self.assemblyFrame.location
        print(self.externalAttack.state.frMonitor)

    # Open attack config frame
    def onOpenExternalAttackConfig(self, event):
        self.externalAttackConfigFrame.open()
        
    # Open assembly frame
    def onOpenAssemblyFrame(self, event):
        self.assemblyFrame.open()


    # Data functions

    # Ask user for state file and load it
    def onLoadState(self,event):
        with wx.FileDialog(self.mainFrame, "Save state file", wildcard="*."+config.applicationExtension, style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()
            try:
                with open(pathname, "rb") as file:
                    results = json.loads(file.read())
                    self.graphState.fromDictionary(results["graphState"])
                    self.externalAttackState.fromDictionary(results["externalAttackState"])

                    # Update UI and graph with new state
                    self.externalAttackConfigFrame.setConfig()
                    self.graph.setData(clean=False)
                    self.updatePropsFromGraph()
                    self.properties.setGraphProps()
                    self.properties.setStats()
                    self.properties.showPanels()

            except Exception as e:
                errorMessage("Error loading state file", str(e))


    # Ask user for state file to save to
    def onSaveState(self,event):
       with wx.FileDialog(self.mainFrame, "Save state file", wildcard="*."+config.applicationExtension, style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()
            try:
                with open(pathname, 'w') as file:
                    aState = json.dumps(self.externalAttackState.toDictionary())
                    gState = json.dumps(self.graphState.toDictionary())
                    
                    file.write("{\"externalAttackState\": "+aState+", \"graphState\": "+gState+"}")
                    file.close()
                
            except Exception as e:
                errorMessage("Error saving state file", str(e))

    # Ask user for data file and load it
    def onLoadData(self, event):
        with wx.FileDialog(self.mainFrame, "Load data file", style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()
            try:
                with open(pathname, 'rb') as file:
                    lines = file.read().decode('utf-8')

                    if (len(lines) < 2):
                        return

                    startTime = time.time()

                    self.graphState.data = resultsStringToArray(lines)
                    
                    if (config.printMetrics):
                        print("Finished loading data in " + str(time.time() - startTime))
                    self.graphData()
                
            except Exception as e:
                errorMessage("Error loading data file", str(e))
                self.graphState.data = None


    # Ask user for data file to save to
    def onSaveData(self, event):
        with wx.FileDialog(self.mainFrame, "Save data file", wildcard="*.txt", style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT) as fileDialog:
            if (fileDialog.ShowModal() == wx.ID_CANCEL):
                return

            pathname = fileDialog.GetPath()
            try:
                with open(pathname, 'w') as file:
                    data = self.graphState.data

                    for i in range(0, data.shape[0]):
                        for j in range(0, data.shape[1]):
                            if (math.isnan(data[i][j])):
                                file.write("0 ")
                            else:
                                file.write(str(int(data[i][j]))+" ")
                        file.write("\n")

                    file.close()
                
            except Exception as e:
                errorMessage("Error saving data file", str(e))


    # Graph functions

    # On graph update event received (graph drawn)
    def onGraphUpdate(self, event):
        self.updatePropsFromGraph()


    # Graph the data and reset limits and properties
    def graphData(self):
        self.graph.setData()
        self.updatePropsFromGraph()
        self.properties.setStats()
        self.properties.showPanels()


    # Update the graph with received properties
    def onGraphPropsUpdate(self, event):
        self.graph.Refresh()

    
    # Update properties with graph settings
    def updatePropsFromGraph(self):
        self.properties.setGraphProps()
        
