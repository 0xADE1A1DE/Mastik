# Doubloon
 

## About
Doubloon is a tool for conducting and analysing CPU cache side-channel attacks. Doubloon provides a UI for configuring attacks and victim programs with support for remote attacks through SSH. Attacks are ran quickly and results are immediately graphed. Doubloon provides an easy interface to analyse and filter results with a live visualisation. Attack configurations and results can be saved and loaded or just the data can be imported and exported to view.

Currently Doubloon only supports Flush+Reload attacks.


## Required dependencies
- Python 3 (tested with 3.7)
- wxPython 4 (available through pip3 or other sources, https://wxpython.org/pages/downloads/index.html)
- numpy (available through pip3)
- paramiko (available through pip3)


## Setup
Run the 'setup' script to check if all dependencies are installed.


## Run
Use the 'run' script to start the program


## Credit
- https://icons8.com for toolbar icons 


## Quick start guide

### Configure attack
Click the gears in the toolbar to open the attack configuration window. Select the type of attack you would like to use and fill in the attack parameters. 'General' is for any attack where 'FR-trace' uses the FR-trace binary from Mastik. Optionally a victim program can run as well and the attack can be ran remotely through SSH. The programs will need to be on the host computer. This menu can stay open and the configuration modified at any time.

### Running an attack
Use the play button in the toolbar to run the attack program. Once it finishes the results will be displayed. The attack can be stopped at anytime by the cancel button. If a victim program is specified, it will run as well.

### Graph
The graph presents the results of the attack. The arrow keys can be used to zoom and pan the graph. A subsection can be viewed though a click and drag selection. The scroll bar at the top shows the current selection and can be dragged to pan. 

### Graph options
The graph limits are shown and can be modified. The Y limits can only be changed here. The attack threshold can be modified as well. Missing values can be interpolated or not be drawn. Values can be displayed as a binary value depending if they are above or below the threshold. Certain sets of results can be disabled or enabled as well.

### Saving
The application state, which includes attack configuration, attack results and graph options, can be saved and loaded later. Use the save icon to create a file with this information (SSH password not included) and use the folder icon to load state from a file. Only the attack results can be exported or imported using the export and import buttons in the toolbar.

### Customisation
Some default values and options can be modified in 'src/config.py' and 'src/State.py' where indicated. Options like default attack parameters and many graph properties can be changed.

