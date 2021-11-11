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


import numpy



# Attack types
EXTERNAL_ATTACK_TYPE_GENERAL = 0
EXTERNAL_ATTACK_TYPE_FRTRACE = 1

# Graph types
GRAPH_TYPE_LINE = 0
GRAPH_TYPE_MAP = 1


# Represents the state of external attacks
# Includes attack configuration, victim details and SSH settings
class ExternalAttackState:
    def __init__(self):

        # Default configuration for attacks. These values can be modified

        # General attack
        self.attackProgram = ""
        self.attackArgs = ""

        # FR-trace attack
        self.frProgram = "FR-trace"
        self.frSlotlen = 5000
        self.frMaxSamples = 100000
        self.frThreshold = 100
        self.frIdle = 500
        self.frHeader = False # Unused
        self.frMonitor = ""
        self.frEvict = ""
        self.frPDACount = 0
        self.frPDA = ""

        # Common details
        self.type = EXTERNAL_ATTACK_TYPE_GENERAL
        self.victimProgram = ""
        self.victimArgs = ""
        self.victimRun = False

        # SSH settings
        self.sshRun = False
        self.sshHostname = ""
        self.sshPort = 22
        self.sshUsername = ""
        self.sshPassword = ""

    
    # Convert relevent configuration values to dictionary, to be saved
    def toDictionary(self): 
        dict = {}
        dict["attackProgram"] = self.attackProgram
        dict["attackArgs"] = self.attackArgs

        dict["frProgram"] = self.frProgram 
        dict["frSlotlen"] = self.frSlotlen
        dict["frMaxSamples"] = self.frMaxSamples
        dict["frThreshold"] = self.frThreshold
        dict["frIdle"] = self.frIdle
        dict["frHeader"] = self.frHeader
        dict["frMonitor"] = self.frMonitor
        dict["frEvict"] = self.frEvict
        dict["frPDACount"] = self.frPDACount
        dict["frPDA"] = self.frPDA

        dict["type"] = self.type
        dict["victimProgram"] = self.victimProgram
        dict["victimArgs"] = self.victimArgs
        dict["victimRun"] = self.victimRun 

        dict["sshRun"] = self.sshRun
        dict["sshHostname"] = self.sshHostname
        dict["sshPort"] = self.sshPort
        dict["sshUsername"] = self.sshUsername

        # Don't save password
        #dict["sshPassword"] = self.sshPassword

        return dict
        

    # Set configuration from dictionary
    def fromDictionary(self, dict):
        self.attackProgram = dict["attackProgram"]
        self.attackArgs = dict["attackArgs"]

        self.frProgram = dict["frProgram"]
        self.frSlotlen = dict["frSlotlen"]
        self.frMaxSamples = dict["frMaxSamples"]
        self.frThreshold = dict["frThreshold"]
        self.frIdle = dict["frIdle"]
        self.frHeader = dict["frHeader"]
        self.frMonitor = dict["frMonitor"]
        self.frEvict = dict["frEvict"]
        self.frPDACount = dict["frPDACount"]
        self.frPDA = dict["frPDA"]

        self.type = dict["type"]
        self.victimProgram = dict["victimProgram"]
        self.victimArgs = dict["victimArgs"]
        self.victimRun = dict["victimRun"]

        self.sshRun = dict["sshRun"]
        self.sshHostname = dict["sshHostname"]
        self.sshPort = dict["sshPort"]
        self.sshUsername = dict["sshUsername"]



# Represents the state of the graph and data
class GraphState:
    def __init__(self):

        # Data is None or a numpy array
        self.data = None
        
        # Graph type
        self.type = GRAPH_TYPE_LINE
        
        # Labels for monitored locations
        self.monitorLocations = []

        # Graph drawing properties
        self.threshold = 100
        self.xLimit = [0,0]
        self.yLimit = [0,0]
        
        # Graph filtering and options
        self.mask = [] # True or false vector for each set of data
        self.highlightUnderThreshold = False # Unused
        self.interpolateMissing = True
        self.thresholdBinary = False


    # Convert relevent settings and data to dictionary, to be saved
    def toDictionary(self): 
        dict = {}

        if (self.data is None):
            dict["data"] = self.data
        else:
            dict["data"] = self.data.tolist()
        
        dict["threshold"] = self.threshold
        dict["xLimit"] = self.xLimit
        dict["yLimit"] = self.yLimit
        dict["mask"] = self.mask
        dict["highlightUnderThreshold"] = self.highlightUnderThreshold
        dict["interpolateMissing"] = self.interpolateMissing 
        dict["thresholdBinary"] = self.thresholdBinary
        dict["type"] = self.type

        return dict


    # Set settings and data from dictionary
    def fromDictionary(self, dict):
        if (dict["data"] is None):
            self.data = dict["data"]
        else:
            self.data = numpy.array(dict["data"], dtype=float)
        
        self.threshold = dict["threshold"]
        self.xLimit = dict["xLimit"]
        self.yLimit = dict["yLimit"]
        self.mask = dict["mask"]
        self.highlightUnderThreshold = dict["highlightUnderThreshold"]
        self.interpolateMissing = dict["interpolateMissing"]
        self.thresholdBinary = dict["thresholdBinary"]

        if ("type" in dict):
            self.type = dict["type"]

        return dict
        
