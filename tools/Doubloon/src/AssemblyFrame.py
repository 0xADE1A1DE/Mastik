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


Author0: Dallas McNeil a1724759@student.adelaide.edu.au
Author1: Feichi Hu feichi@umich.edu
"""


import wx

import config
from util import *
from State import *
from Events import *
from tqdm import tqdm



# Window displaying machine code of chosen executable
class AssemblyFrame(wx.Frame):
    def __init__(self, state):
        wx.Frame.__init__(self, None, wx.ID_ANY, "Assembly viewer")
        # Copy of the common attack state from application
        self.panel = wx.Panel(self, wx.ID_ANY)
        #self.panel1 = wx.Panel(self, wx.ID_ANY)
        #self.panel2 = wx.Panel(self, wx.ID_ANY)
        self.state = state
        self.s2m = None
        self.m2s = None
        self.selectedMIdx = -1
        self.selectedSIdx = -1
        self.selectedList = False
        self.fileIdx = {}
        self.sourceCode = []
        self.searchIdx = []
        self.searchIdxIdx = 0
        self.location = ""
        # Setup list view

        self.fileList = wx.ListCtrl(self.panel, -1, style = wx.LC_REPORT|wx.SUNKEN_BORDER)
        self.fileList.InsertColumn(0,'File', width = 100)


        self.assemblyList = wx.ListCtrl(self.panel, -1, style = wx.LC_REPORT|wx.SUNKEN_BORDER)
        self.assemblyList.InsertColumn(0,'File')
        self.assemblyList.InsertColumn(1,'LineNum')
        self.assemblyList.InsertColumn(2,'Address')
        self.assemblyList.InsertColumn(3,'Function')
        self.assemblyList.InsertColumn(4,'Instr')
        self.assemblyList.InsertColumn(5,'Param', width = 100)
        self.assemblyList.InsertColumn(6,'Comment', width = 100)

        self.sourceCodeList = wx.ListCtrl(self.panel, -1, style = wx.LC_REPORT|wx.SUNKEN_BORDER)
        self.sourceCodeList.InsertColumn(0,'File')
        self.sourceCodeList.InsertColumn(1,'LineNum')
        self.sourceCodeList.InsertColumn(2,'Source Code',width = 500)

        self.monitorList = wx.ListCtrl(self.panel, -1, style = wx.LC_REPORT|wx.SUNKEN_BORDER)
        self.monitorList.InsertColumn(0,'Source File',width = 100)
        self.monitorList.InsertColumn(1,'LineNum',width = 100)
        self.monitorDict = dict()

        self.label = wx.StaticText(self.panel, -1, "Search Code")
        self.searchBox = wx.TextCtrl(self.panel,style = wx.TE_PROCESS_ENTER, size=(200,config.textboxHeight))
        self.searchIdx = []
        self.numResult = -1
        self.prompt = wx.StaticText(self.panel, -1, "Press enter to search / jump to next")


        self.SetTitle("Source Code and Assembly List")
        self.SetClientSize(config.defaultWindowSize)

        # Setup UI
        largeSizer = wx.BoxSizer(wx.VERTICAL)
        sizer = wx.BoxSizer(wx.HORIZONTAL) #bottom
        sizer2 = wx.BoxSizer(wx.HORIZONTAL) # top
        sizer.Add(self.fileList, 0, wx.ALL|wx.EXPAND, 5)
        sizer.Add(self.sourceCodeList, 0, wx.ALL|wx.EXPAND, 5)
        sizer.Add(self.assemblyList, 0, wx.ALL|wx.EXPAND, 5)
        sizer.Add(self.monitorList, 0, wx.ALL|wx.EXPAND, 5)
        sizer2.Add(self.label, 0, wx.ALL, 5)
        sizer2.Add(self.searchBox, 0, wx.ALL, 5)
        sizer2.Add(self.prompt, 0, wx.ALL, 5)
        largeSizer.Add(sizer2, 0, wx.ALL, 5)
        largeSizer.Add(sizer, 1, wx.ALL|wx.EXPAND, 5)
        self.panel.SetSizer(largeSizer)
        #sizer.Fit(self)
        #self.Layout()

        # Sizing
        self.SetClientSize(20+self.panel.GetClientSize().Width,20+self.panel.GetClientSize().Height)
        
        # Event bindings
        self.Bind(wx.EVT_CLOSE, self.onClose)
        self.fileList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.onSelectF)
        self.assemblyList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.onSelectM)
        self.sourceCodeList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.onSelectS)
        self.sourceCodeList.Bind(wx.EVT_LIST_ITEM_RIGHT_CLICK, self.onMonitorS)
        self.monitorList.Bind(wx.EVT_LIST_ITEM_RIGHT_CLICK, self.onDemonitor)
        self.monitorList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.onSelectMonitor)
        self.searchBox.Bind(wx.EVT_TEXT_ENTER, self.onSearch)
        self.searchBox.Bind(wx.EVT_TEXT, self.resetIdx)

        self.setConfig()

    # put items in monitor list into a location string 
    def generateLocationString(self):
        location = ""
        for key in self.monitorDict:
            location += key + " "
        print("--- The monitor locations are ---")
        print(location)
        self.location = location
        evt = updateMonitorLocationEvent()
        wx.PostEvent(self, evt)

    def initMonitorList(self):
        print("--- Init monitor list ---")
        frMonitor = self.state.frMonitor
        print(frMonitor)
        locations = frMonitor.split()
        print(locations)
        for i, location  in enumerate(locations):
            loc = location.split(':')
            print("loc",loc)
            self.monitorList.Append(loc)
            self.monitorDict[location] = self.locateMonitor(loc)

    #find sourceList idx from file and line
    def locateMonitor(self, location):
        print("Locating monitor location in file")
        print(location)
        file = location[0]
        line = location[1]
        for idx in range(self.sourceCodeList.GetItemCount()): 
            item = self.sourceCodeList.GetItem(idx, 0) 
            if item.GetText() == file: 
                lineNum = self.sourceCodeList.GetItem(idx, 1)
                if(lineNum.GetText()==line):
                    print("Target found!!!", file,line,idx)
                    return idx	
        return -1


    # Open window
    def open(self):
        print("Try open assem")
        self.Show()
        self.Maximize(True)


    # Close window on cross out
    def onClose(self, event):
        self.setConfig()
        self.generateLocationString()
        self.Hide()


    # reset search Idx on new content
    def resetIdx(self,event):
        self.searchIdx = []
        self.searchIdxIdx = -1 
        self.prompt.SetLabel("Press enter to search / jump to next")

    # press enter in the search box
    def onSearch(self,event):
        keyWord = self.searchBox.GetValue()
        if self.searchIdxIdx == -1:
            for i in range(len(self.sourceCode)):
                if keyWord in self.sourceCode[i]:
                    self.searchIdx.append(i)
            #self.searchIdx = self.sourceCodeList.FindItem(self.searchIdx, keyWord, partial = True) 
            #add on : Idx == -1
        if self.searchIdx != []: #[] means no search result
            self.searchIdxIdx = self.searchIdxIdx + 1
            if self.searchIdxIdx == self.numResult:
                self.searchIdxIdx = 0
            #print(self.searchIdx)
            self.numResult = len(self.searchIdx)
            self.sourceCodeList.Focus(self.searchIdx[self.searchIdxIdx])
            self.sourceCodeList.Select(self.searchIdx[self.searchIdxIdx])
        else:
            self.numResult = 0
            print("Result not found")
        self.prompt.SetLabel(str(self.numResult) + " results found")


    # select monitor location
    def onSelectMonitor(self, event):
        idx = event.GetIndex()
        file = self.monitorList.GetItem(idx, 0).GetText()
        line = self.monitorList.GetItem(idx, 1).GetText()
        Sidx = self.monitorDict[file+":"+line]
        print(Sidx)
        if(Sidx==-1):
            print("Location not found")
            return
        self.sourceCodeList.Select(-1,on=0)
        self.sourceCodeList.Focus(Sidx)
        self.sourceCodeList.Select(Sidx)


    # right click on item to add it into monitor list
    def onMonitorS(self, event):
        idx = event.GetIndex()
        file = self.sourceCodeList.GetItem(idx, 0).GetText()
        line = self.sourceCodeList.GetItem(idx, 1).GetText()
        if file+":"+line not in self.monitorDict:
            self.monitorList.Append([file,line])
            self.monitorDict[file+":"+line]= idx
    
    #remove monitor location:
    def onDemonitor(self, event):
        idx = event.GetIndex()
        file = self.monitorList.GetItem(idx, 0).GetText()
        line = self.monitorList.GetItem(idx, 1).GetText()
        self.monitorList.DeleteItem(idx)
        try:
            del self.monitorDict[file+":"+line]
        except KeyError:
            pass


    # Select one line  //add to config
    def onSelectF(self, event):
        item = event.GetItem()
        print(item.GetText())
        Fidx = self.fileIdx[item.GetText()]
        self.sourceCodeList.Focus(Fidx)
        self.sourceCodeList.Select(Fidx)


    def onSelectM(self, event): 
        idx = event.GetIndex() 
        Sidx = self.m2s[idx]
        print(idx,Sidx)
        self.sourceCodeList.Unbind(wx.EVT_LIST_ITEM_SELECTED)
        self.sourceCodeList.Select(-1,on=0)
        self.sourceCodeList.Focus(Sidx)
        self.sourceCodeList.Select(Sidx)
        self.sourceCodeList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.onSelectS)

    def onSelectS(self, event):
        idx = event.GetIndex()
        Midx = self.s2m[idx]
        print(idx,Midx)
        self.assemblyList.Unbind(wx.EVT_LIST_ITEM_SELECTED)
        self.assemblyList.Select(-1,on=0)
        self.assemblyList.Focus(Midx)
        self.assemblyList.Select(Midx)
        self.assemblyList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.onSelectM)



    # Limit input of numbers to digits
    def onLimitChar(self, event):
        key = event.GetKeyCode()
        if (key < 255):
            if chr(key).isdigit():
                event.Skip()


    # When attack UI is edited, update attack state
    def onConfig(self, event):
        if (self.IsShown()):
            self.state.frMonitor = self.frMonitor.GetValue()

    # Set UI from attack state
    def setConfig(self):
        self.setting = True
        #self.frMonitor.ChangeValue(self.state.frMonitor)

    #insert dump result into listctrl
    def insert(self, results):
        print("=== Inserting Assembly Lines ===")
        start = -1
        end = -1
        color = 0
        nextColor = 0
        colors = [config.assemblyColorA, config.assemblyColorAB, config.assemblyColorB]
        cacheLine = start/64
        for i in tqdm(range(0,len(results)-1)):
            start = results[i][2]
            end = results[i+1][2] - 1
            startCacheLine = start//64
            endCacheLine = end//64
            #print(startCacheLine, endCacheLine)
            if(cacheLine<startCacheLine):
                cacheLine = startCacheLine
                color = 0 if (nextColor==2) else 2
                nextColor = color
            else:
                color = nextColor
            if(endCacheLine>startCacheLine):
                color = 1
            row = results[i]
            row[2] = hex(row[2])
            idx = self.assemblyList.Append(row)
            self.assemblyList.SetItemBackgroundColour(idx, colors[color])
        print("=== Assembly Insertion Finished ===")
        self.Show()

    # insert source code into list
    def insertSource(self, results):
        try:
            print("=== Inserting Source File Lines ===")
            file = ""
            for i in tqdm(range(len(results))):
                line = results[i]
                self.sourceCode.append(line[2]) # append source code to self.sourceCode
                if file != line[0]:
                    file = line[0]
                    self.fileIdx[file] = i
                    self.fileList.Append([file])
                self.sourceCodeList.Append(line)
            print("=== Inserting Source File Lines ===")
        except Exception as e:
            errorMessage("Error inserting source code", str(e))




    #add one line in listctrl at a time
    def addLine(self, row):
        pos = self.assemblyList.InsertStringItem(0,row[0])
        for i in range(1,7):
            self.assemblyList.SetStringItem(pos, i,row[i])

    def clear(self):
        print("Clear assembly frame")
        self.assemblyList.DeleteAllItems()
        self.sourceCodeList.DeleteAllItems()
        self.fileList.DeleteAllItems()
        self.monitorDict.clear()
        self.monitorList.DeleteAllItems()
        self.s2m = None
        self.m2s = None
        self.fileIdx.clear()
        self.searchIdx.clear()
        self.numResult = -1
        self.searchIdxIdx = 0
        self.selectedMIdx = -1
        self.selectedSIdx = -1
        self.selectedList = False
        self.sourceCode = []
        self.location = ""
      