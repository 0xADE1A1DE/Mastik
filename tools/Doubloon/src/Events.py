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


import wx.lib.newevent



# Application owns everything, it doesn't need to send events to children, it can just calls their methods
# Most events will be owned objects (Toolbar, Attack, ...) informing Application to do something
# e.g Button in Toolbar for loading data, send event to application which handles asking and loading data

# Toolbar informs Application to run attack
runExternalAttackEvent, EVT_RUN_EXTERNAL_ATTACK = wx.lib.newevent.NewEvent()

# Attack informs Application attack is done
externalAttackDoneEvent, EVT_EXTERNAL_ATTACK_DONE = wx.lib.newevent.NewEvent()

# Assembly frame informs application FRmonitor location has changed
updateMonitorLocationEvent, EVT_UPDATE_LOCATION = wx.lib.newevent.NewEvent()

# Dump informs assembly viewer its done
dumpDoneEvent, EVT_DUMP_DONE = wx.lib.newevent.NewEvent()
mapDoneEvent, EVT_MAP_DONE = wx.lib.newevent.NewEvent()

# Toolbar opens assembly viewer
openAssemblyFrameEvent, EVT_OPEN_ASSEMBLY = wx.lib.newevent.NewEvent()

# perform dump while opening assembly viewer
openAssemblyFrameEvent, EVT_RUN_DUMP = wx.lib.newevent.NewEvent()

# Toolbar informs Application to open ExternalAttackConfigFrame
openExternalAttackConfigEvent, EVT_OPEN_EXTERNAL_ATTACK_CONFIG = wx.lib.newevent.NewEvent()

# PropertiesView informs Application graph properties are updated
graphPropsUpdateEvent, EVT_GRAPH_PROPS_UPDATE = wx.lib.newevent.NewEvent()

# GraphView informs Application it updated itself (drawn)
graphUpdateEvent, EVT_GRAPH_UPDATE = wx.lib.newevent.NewEvent()

# Toolbar informs Application to ask user for data to load
loadDataEvent, EVT_LOAD_DATA = wx.lib.newevent.NewEvent()

# Toolbar informs Application to save data
saveDataEvent, EVT_SAVE_DATA = wx.lib.newevent.NewEvent()

# Toolbar informs Application to ask user for state to load
loadStateEvent, EVT_LOAD_STATE = wx.lib.newevent.NewEvent()

# Toolbar informs Application to save state
saveStateEvent, EVT_SAVE_STATE = wx.lib.newevent.NewEvent()
