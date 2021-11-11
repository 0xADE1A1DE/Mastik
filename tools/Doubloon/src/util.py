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
import re


# Show an error dialog with a message
def errorMessage(title, message):   
    print(title + ": " + message)
    dial = wx.MessageDialog(None, message, title,
    wx.OK | wx.ICON_ERROR)
    dial.ShowModal()


# Convert results string to numpy array, used to load in data from file or attack process
def resultsStringToArray(results):
    # TODO: This could be optimised 
    allLines = results.splitlines()
    lines = list(filter(isNotComment, allLines))
    array = numpy.empty((len(lines),len(lines[0].split())))
    
    for i in range(0, len(lines)):
        splitLine = lines[i].split()
        for j in range(0, len(splitLine)):
            value = int(splitLine[j])
            if (value != 0):
                array[i][j] = int(splitLine[j])
            else:
                array[i][j] = float("nan")

    return array


# Determines if a line is a comment or not
def isNotComment(str):
    return str[0] != "#"