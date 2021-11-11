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
import numpy
import math
import time

import config
from util import *
from Events import *
from State import *



# Graphs data and manages interactions
class GraphView(wx.Window):
    def __init__(self, parent, state):
        super(GraphView, self).__init__(parent)

        # Copy of the common graph state from application
        self.state = state

        # Other drawing properties 
        self.xLength = 0

        # Dragging variables
        self.initialX = 0
        self.finalX = 1
        self.dragging = False

        # Setup UI
        self.scrollBar = wx.ScrollBar(self)
        self.SetBackgroundStyle(wx.BG_STYLE_PAINT)
        self.scrollBar.SetScrollbar(0, 10000, 10000, 1)
        self.scrollBar.SetThumbPosition(0)

        # Bind events
        self.Bind(wx.EVT_PAINT, self.onPaint)
        self.Bind(wx.EVT_SCROLL, self.onScroll)

        # Graph selection events
        self.Bind(wx.EVT_LEFT_DOWN, self.onMouseDown)
        self.Bind(wx.EVT_MOTION, self.onMouseMove)
        self.Bind(wx.EVT_LEFT_UP, self.onMouseUp)
    

    # Set the graph data, resets limits
    # Use clean=False to disable resetting limits and masks
    def setData(self, clean=True):
        self.xLength = self.state.data.shape[0]

        if (clean):
            self.state.mask = [True]*self.state.data.shape[1]
            self.state.xLimit = [0, self.xLength-1]
            self.state.yLimit = config.graphDefaultYLimits

        self.scrollBar.SetScrollbar(0, self.xLength, self.xLength, 1)
        
        self.Refresh()


    # Set graph limits
    def setLimits(self, xLimMin, xLimMax, yLimMin, yLimMax):
        self.state.xLimit = [xLimMin, xLimMax]
        self.state.yLimit = [yLimMin, yLimMax]

        self.cleanLimits()
        
        self.scrollBar.SetScrollbar(0, self.state.xLimit[1] - self.state.xLimit[0], self.xLength - 1, 1)
        self.scrollBar.SetThumbPosition(self.state.xLimit[0])


    # Zoom in or out of graph by factor of current selection
    def zoom(self, factor):
        if (self.state.xLimit[1] - self.state.xLimit[0] > config.graphMinXSize or factor > 1):
            centre = (self.state.xLimit[1] + self.state.xLimit[0]) / 2
            self.state.xLimit[0] = int(centre - ((centre - self.state.xLimit[0]) * factor))
            self.state.xLimit[1] = int(centre - ((centre - self.state.xLimit[1]) * factor))
            self.scrollBar.SetScrollbar(0, self.state.xLimit[1] - self.state.xLimit[0], self.xLength - 1, 1)
            self.scrollBar.SetThumbPosition(self.state.xLimit[0])

            self.Refresh()
            
            evt = graphUpdateEvent()
            wx.PostEvent(self, evt)


    # Move graph left or right by factor of current selection, negative for left, positive for right
    def shift(self, factor):
        diff = self.state.xLimit[1] - self.state.xLimit[0]
        if (factor <= 0):
            self.state.xLimit[0] = int(self.state.xLimit[0] + (diff * factor))
            if (self.state.xLimit[0] < 0):
                self.state.xLimit[0] = 0
            self.state.xLimit[1] = self.state.xLimit[0] + diff
        else:
            self.state.xLimit[1] = int(self.state.xLimit[1] + (diff * factor))
            if (self.state.xLimit[1] > self.xLength):
                self.state.xLimit[1] = self.xLength
            self.state.xLimit[0] = self.state.xLimit[1] - diff

        self.scrollBar.SetScrollbar(0, self.state.xLimit[1] - self.state.xLimit[0], self.xLength - 1, 1)
        self.scrollBar.SetThumbPosition(self.state.xLimit[0])

        self.Refresh()
                
        evt = graphUpdateEvent()
        wx.PostEvent(self, evt)


    # Shift graph on scroll
    def onScroll(self, event):
        diff = self.state.xLimit[1] - self.state.xLimit[0]
        self.state.xLimit[0] = event.GetPosition()
        self.state.xLimit[1] = self.state.xLimit[0] + diff
        self.Refresh()
        
        evt = graphUpdateEvent()
        wx.PostEvent(self, evt)


    # Shortcuts for graph 
    def onButton(self, event):
        if (event.GetKeyCode() == wx.WXK_UP):
            self.zoom(1.0/config.graphZoomFactor)
        elif (event.GetKeyCode() == wx.WXK_DOWN):
            self.zoom(config.graphZoomFactor)
        elif (event.GetKeyCode() == wx.WXK_LEFT):
            self.shift(-config.graphShiftFactor)
        elif (event.GetKeyCode() == wx.WXK_RIGHT):
            self.shift(config.graphShiftFactor)


    # On mouse down, handle start of selection 
    def onMouseDown(self, event):
        self.dragging = True
        self.initialX = event.GetPosition()[0]
        self.finalX = event.GetPosition()[0]
        self.Refresh()


    # On mouse move, handle selection
    def onMouseMove(self, event):
        if (self.dragging):
            self.finalX = event.GetPosition()[0]
            self.Refresh()


    # On mouse up, handle end of selection
    def onMouseUp(self, event):
        self.dragging = False
        self.finalX = event.GetPosition()[0]
        if (self.initialX == self.finalX):
            return

        if (self.initialX > self.finalX):
            temp = self.finalX 
            self.finalX = self.initialX
            self.initialX = temp

        xRange = self.state.xLimit[1] - self.state.xLimit[0]
        factor = xRange / self.GetClientSize()[0]

        self.state.xLimit[1] = int(factor * self.finalX) + self.state.xLimit[0]
        self.state.xLimit[0] = int(factor * self.initialX) + self.state.xLimit[0]
 
        self.scrollBar.SetScrollbar(0, self.state.xLimit[1] - self.state.xLimit[0], self.xLength - 1, 1)
        self.scrollBar.SetThumbPosition(self.state.xLimit[0])

        evt = graphUpdateEvent()
        wx.PostEvent(self, evt)

        self.Refresh()


    # Limit the limits to correct values
    def cleanLimits(self):
        updateProps = False
        if (self.state.xLimit[1] - self.state.xLimit[0] < config.graphMinXSize):
            self.state.xLimit[1] = self.state.xLimit[0] + config.graphMinXSize
            updateProps = True
        if (self.state.yLimit[1] - self.state.yLimit[0] < config.graphMinYSize):
            self.state.yLimit[1] = self.state.yLimit[0] + config.graphMinYSize
            updateProps = True

        if (self.state.xLimit[0] < 0):
            self.state.xLimit[0] = 0
            updateProps = True
        if (self.state.xLimit[1] > self.xLength):
            self.state.xLimit[1] = self.xLength - 1
            updateProps = True
        if (self.state.yLimit[0] < 0):
            self.state.yLimit[0] = 0
            updateProps = True

        self.scrollBar.SetScrollbar(0, self.state.xLimit[1] - self.state.xLimit[0], self.xLength - 1, 1)
        self.scrollBar.SetThumbPosition(self.state.xLimit[0])

        if (updateProps):
            evt = graphUpdateEvent()
            wx.PostEvent(self, evt)


    # Draw the graph based on settings
    def onPaint(self, event):
        # Measure the time
        startTime = time.time()

        # Update UI elements
        self.scrollBar.SetPosition(wx.Point(0,0))
        self.scrollBar.SetSize(wx.Size(self.GetClientSize()[0],self.scrollBar.GetClientSize()[1]))

        # Graph variables
        marginX = (30, 0)
        marginY = (16, 30)
        width, height = self.GetClientSize()
        width -= marginX[0] + marginX[1]
        height -= marginY[0] + marginY[1]

        # Common drawing variables
        dc = wx.AutoBufferedPaintDC(self)
        brush = wx.Brush(config.graphBackground, style=wx.BRUSHSTYLE_SOLID)
        pen = wx.Pen(config.graphForeground)
        dc.SetBackgroundMode(wx.SOLID)
        dc.SetBackground(brush)
        dc.Clear()
        
        # Check if data to graph
        if (self.state.data is None):
            dc.TextBackground = wx.Colour(0,0,0,0)
            dc.TextForeground = config.graphForeground
            str1 = "No data"
            dc.DrawText(str1, (width/2) - (dc.GetTextExtent(str1)[0]/2), (height/2))
            return

        # Cleanup limits
        self.cleanLimits()

        # Other drawing variables
        xScale = float(width) / float(self.state.xLimit[1] - self.state.xLimit[0])
        yScale = float(height) / float(self.state.yLimit[1] - self.state.yLimit[0])
        cluster = math.ceil((self.state.xLimit[1] - self.state.xLimit[0]) / config.graphMaxResolution)
        xGrid = int(math.pow(10,math.floor(math.log10(self.state.xLimit[1] - self.state.xLimit[0]))))
        yGrid = int(math.pow(10,math.floor(math.log10(self.state.yLimit[1] - self.state.yLimit[0]))))

        if (self.state.type == GRAPH_TYPE_LINE):
            # Line graph

            # Draw grid (X lines)
            pen.SetColour(config.graphGrid)
            pen.SetWidth(2)
            dc.SetPen(pen)
            for x in range(math.floor(self.state.xLimit[0] / xGrid) * xGrid, self.state.xLimit[1], xGrid):
                xVal = ((x - self.state.xLimit[0])*xScale) + marginX[0]
                if (xVal >= marginX[0]):
                    dc.DrawLine(xVal, marginY[0], xVal, height + marginY[0])

            pen.SetWidth(1)
            dc.SetPen(pen)
            if (xGrid == 1):
                xGrid = 10
            for x in range(math.floor(self.state.xLimit[0] / xGrid) * xGrid, self.state.xLimit[1], int(xGrid / 10)):
                xVal = ((x - self.state.xLimit[0])*xScale) + marginX[0]
                if (xVal >= marginX[0]):
                    dc.DrawLine(xVal, marginY[0], xVal, height + marginY[0])
                
            # Draw Grid (Y lines) and threshold line
            pen.SetWidth(2)
            dc.SetPen(pen)
            if (self.state.thresholdBinary):
                dc.DrawLine(marginX[0], (height / 2) + marginY[0], width + marginX[0], (height / 2)+marginY[0])
            else:
                yVal = (height - ((self.state.threshold - self.state.yLimit[0])*yScale)) + marginY[0]
                dc.DrawLine(marginX[0], yVal, width + marginX[0], yVal)
                pen.SetWidth(1)
                dc.SetPen(pen)
                
                for y in range(math.floor(self.state.yLimit[0] / yGrid) * yGrid, self.state.yLimit[1], yGrid):
                    yVal = (height - ((y - self.state.yLimit[0])*yScale)) + marginY[0]
                    if (yVal >= marginY[0]):
                        dc.DrawLine(marginX[0], yVal, width + marginX[0], yVal)
                
            # Draw each set
            for j in range(0, self.state.data.shape[1], 1):
                if (not self.state.mask[j]):
                    continue

                pen.SetColour(config.graphColours[j % len(config.graphColours)])
                lastPoint = [0,float("nan")]

                # Draw results for set
                for i in range(self.state.xLimit[0], self.state.xLimit[1]+1, cluster):
                    selection = self.state.data[i:i + cluster,j]
                    if (selection.size == 0):
                        lastPoint = thisPoint
                        continue
                        
                    thisPoint = [i, float(numpy.nanmin(selection))]

                    if (self.state.thresholdBinary):
                        if (math.isnan(thisPoint[1])):
                            pass
                        elif (thisPoint[1] > self.state.threshold):
                            thisPoint[1] = 1
                        else:
                            thisPoint[1] = 0

                    if (math.isnan(lastPoint[1])):
                        lastPoint = thisPoint
                        continue

                    if (math.isnan(thisPoint[1])):
                        if (not self.state.interpolateMissing):
                            lastPoint = thisPoint
                        continue
                    
                    if (lastPoint[1] < self.state.threshold and thisPoint[1] < self.state.threshold and self.state.highlightUnderThreshold and not self.state.thresholdBinary):
                        pen.SetWidth(2)
                    else:
                        pen.SetWidth(2)
                    
                    # Make connecting lines thinner for binary
                    if (lastPoint[1] != thisPoint[1] and self.state.thresholdBinary):
                        pen.SetWidth(1)

                    dc.SetPen(pen)
                    if (self.state.thresholdBinary):
                        offset = (0.5 / (self.state.data.shape[1] + 1)) * (j + 1)
                        dc.DrawLine(((lastPoint[0] - self.state.xLimit[0])*xScale)+marginX[0],
                        (height * (1 - (offset + (lastPoint[1] * (1 - (2 * offset))))))+marginY[0],
                        ((thisPoint[0] - self.state.xLimit[0])*xScale)+marginX[0],
                        (height * (1 - (offset + (thisPoint[1] * (1 - (2 * offset))))))+marginY[0])
                    else:
                        dc.DrawLine(((lastPoint[0] - self.state.xLimit[0])*xScale)+marginX[0],
                        (height - ((lastPoint[1] - self.state.yLimit[0])*yScale))+marginY[0],
                        ((thisPoint[0] - self.state.xLimit[0])*xScale)+marginX[0],
                        (height - ((thisPoint[1] - self.state.yLimit[0])*yScale))+marginY[0])
                    lastPoint = thisPoint

            # Draw margin background
            brush.SetColour(config.graphBackground)
            dc.SetBrush(brush)
            pen.SetWidth(0)
            pen.SetColour(config.graphBackground)
            dc.SetPen(pen)

            dc.DrawRectangle(0,0,marginX[0], height+marginY[0])
            dc.DrawRectangle(0,height+marginY[0],width+marginX[0], marginY[1])
            dc.DrawRectangle(marginX[0],0,width+marginX[1], marginY[0])
            dc.DrawRectangle(width+marginX[0],marginY[0],marginX[1], height+marginY[1])

            brush.SetColour(wx.Colour(0,0,0,0))
            dc.SetBrush(brush)
            pen.SetWidth(2)
            pen.SetColour(config.graphBorder)
            dc.SetPen(pen)
            dc.DrawRectangle(marginX[0],marginY[0],width,height)
                
            # Draw numbers
            dc.TextBackground = wx.Colour(0,0,0,0)
            dc.TextForeground = config.graphForeground
            for x in range(math.floor(self.state.xLimit[0] / xGrid) * xGrid, self.state.xLimit[1], xGrid):
                xVal = ((x - self.state.xLimit[0])*xScale) + 2 + marginX[0]
                if (xVal >= marginX[0]):
                    dc.DrawText(str(x), xVal, (height-16)+marginY[0])

            # Draw threshold value text
            if (self.state.thresholdBinary):
                dc.DrawText("Above "+str(self.state.threshold), marginX[0], (height / 2) + marginY[0] - 16)
                dc.DrawText("Below "+str(self.state.threshold), marginX[0], (height / 2) + marginY[0])
            else:
                yVal = (height - ((self.state.threshold - self.state.yLimit[0])*yScale)) + marginY[0]
                dc.DrawText("Threshold: " + str(self.state.threshold), marginX[0], yVal)

                for y in range(math.floor(self.state.yLimit[0] / yGrid) * yGrid, self.state.yLimit[1], yGrid):
                    yVal = (height - ((y - self.state.yLimit[0])*yScale)) + marginY[0]
                    if (yVal >= marginY[0] and y != self.state.threshold):
                        dc.DrawText(str(y), marginX[0], yVal)
            
        
            # Draw selection, if applicable
            if (self.dragging):
                pen.SetColour(config.graphForeground)
                fill = config.graphForeground
                fill = wx.Colour(fill.red, fill.green, fill.blue, 25)
                brush.SetColour(fill)
                dc.SetBrush(brush)
                dc.SetPen(pen)
                dc.DrawRectangle(self.initialX, marginY[0], (self.finalX - self.initialX), height)

            # Draw axis labels
            str1 = "Time slots"
            dc.DrawText(str(str1), marginX[0] + (width/2) - (dc.GetTextExtent(str1)[0]/2), height + marginY[0] + 6)
            str2 = "Access time (cycles)"
            dc.DrawRotatedText(str("Access time (cycles)"), 6, marginY[0] + (height/2) + (dc.GetTextExtent(str2)[0]/2), 90)


        elif (self.state.type == GRAPH_TYPE_MAP):
            # Graph type

            # Change graph variables
            marginX = (30, 30)
            marginY = (16, 46)
            width, height = self.GetClientSize()
            width -= marginX[0] + marginX[1]
            height -= marginY[0] + marginY[1]

            xCount = int((self.state.xLimit[1] - self.state.xLimit[0]+1) / cluster)
            yCount = int(self.state.data.shape[1])
            
            imageData = [0] * (xCount*yCount*3)
            nextByte = 0
            
            # Draw each set
            for y in range(0, yCount, 1):

                lastPoint = [0,float("nan")]

                # Draw results for set
                for x in range(self.state.xLimit[0], self.state.xLimit[1]+1, cluster):

                    selection = self.state.data[x:x + cluster,y]
                    
                    if (selection.size == 0):
                        lastPoint = thisPoint
                        continue
                        
                    thisPoint = [x, float(numpy.nanmin(selection))]
                    
                    if (self.state.thresholdBinary):
                        if (math.isnan(thisPoint[1])):
                            pass
                        elif (thisPoint[1] > self.state.threshold):
                            thisPoint[1] = 1
                        else:
                            thisPoint[1] = 0

                    if (math.isnan(lastPoint[1])):
                        lastPoint = thisPoint
                        imageData[nextByte] = 0
                        imageData[nextByte+1] = 0
                        imageData[nextByte+2] = 0
                        nextByte = nextByte + 3
                        continue

                    if (math.isnan(thisPoint[1])):
                        if (not self.state.interpolateMissing):
                            lastPoint = thisPoint
                        imageData[nextByte] = 0
                        imageData[nextByte+1] = 0
                        imageData[nextByte+2] = 0
                        nextByte = nextByte + 3
                        continue
                    
                    if (lastPoint[1] < self.state.threshold and thisPoint[1] < self.state.threshold and self.state.highlightUnderThreshold and not self.state.thresholdBinary):
                        pen.SetWidth(3)
                    else:
                        pen.SetWidth(2)
                    
                    value = (thisPoint[1] - self.state.yLimit[0]) / (self.state.yLimit[1] - self.state.yLimit[0])
                    if (self.state.thresholdBinary):
                        value = thisPoint[1]
                    
                    imageData[nextByte] = 0
                    imageData[nextByte+1] = int(max(min(value*255,255),0))
                    imageData[nextByte+2] = 0
                    nextByte = nextByte + 3

                    lastPoint = thisPoint
 

            image = wx.Image(xCount, yCount, bytes(imageData))
            image = image.Scale(width, height)
            dc.DrawBitmap(image.ConvertToBitmap(), marginX[0], marginY[0])

            # Draw grid (X lines)
            dc.TextBackground = wx.Colour(0,0,0,0)
            dc.TextForeground = config.graphForeground
            pen.SetColour(config.graphGrid)
            pen.SetWidth(2)
            dc.SetPen(pen)
            for x in range(math.floor(self.state.xLimit[0] / xGrid) * xGrid, self.state.xLimit[1], xGrid):
                xVal = ((x-self.state.xLimit[0])*(width/xCount)) + marginX[0]
                if (xVal >= marginX[0] and xVal <= marginX[0] + width):
                    dc.DrawLine(xVal, marginY[0], xVal, height + marginY[0])
                    dc.DrawText(str(x), xVal, (height)+marginY[0])

            if (xGrid == 1):
                xGrid = 10
            pen.SetColour(config.graphGrid)
            pen.SetWidth(1)
            dc.SetPen(pen)
            for x in range(math.floor(self.state.xLimit[0] / xGrid) * xGrid, self.state.xLimit[1], int(xGrid / 10)):
                xVal = ((x-self.state.xLimit[0])*(width/xCount)) + marginX[0]
                if (xVal >= marginX[0] and xVal <= marginX[0] + width):
                    dc.DrawLine(xVal, marginY[0], xVal, height + marginY[0])

            brush.SetColour(wx.Colour(0,0,0,0))
            dc.SetBrush(brush)
            pen.SetWidth(2)
            pen.SetColour(config.graphBorder)
            dc.SetPen(pen)
            dc.DrawRectangle(marginX[0],marginY[0],width,height)
                
            # Draw selection, if applicable
            if (self.dragging):
                pen.SetColour(config.graphForeground)
                fill = config.graphForeground
                fill = wx.Colour(fill.red, fill.green, fill.blue, 25)
                brush.SetColour(fill)
                dc.SetBrush(brush)
                dc.SetPen(pen)
                dc.DrawRectangle(self.initialX, marginY[0], (self.finalX - self.initialX), height)


        if (config.printMetrics):
            print("Drew in " + str(time.time() - startTime))        
    
        