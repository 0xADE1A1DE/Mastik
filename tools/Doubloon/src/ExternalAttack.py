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


import subprocess
import time
import threading
import numpy
import shlex
import paramiko
import wx

import config
from util import *
from State import *
from Events import *



# Manages external attack and victim programs
# Attacks that rely on an external binary (such as FR-trace)
class ExternalAttack(wx.EvtHandler):
    def __init__(self, state):
        super(ExternalAttack, self).__init__()

        # Copy of the common attack state from application
        self.state = state

        # Attack variables
        self.running = False
        self.hasResults = False
        self.cancelled = False
        self.attackProccess = None
        self.victimProccess = None
        self.sshClient = None

        # Attack results
        self.rawResults = ""
        self.results = None


    # Create new thread and do runThreaded
    def run(self):
        # Check paramaters
        if (self.state.type == EXTERNAL_ATTACK_TYPE_GENERAL):
            if (self.state.attackProgram == ""):
                errorMessage("Error starting attack", "Attack program not specified")
                return
        if (self.state.type == EXTERNAL_ATTACK_TYPE_FRTRACE):
            if (self.state.frProgram == ""):
                errorMessage("Error starting attack", "FR-trace program not specified")
                return

        if (self.state.victimRun and self.state.victimProgram == ""):
            errorMessage("Error starting attack", "Victim program not specified")
            return


        try:
            self.thread = threading.Thread(target=self.runThreaded)
            self.thread.start()
        except Exception as e:
            errorMessage("Error starting attack", str(e))
            

    # Perform an attack
    def runThreaded(self):
        print("=== Running external attack ===")

        try:
            # If remote, setup the ssh connection if it doesn't already exist
            if (not self.sshClient and self.state.sshRun):
                print("Connecting to ssh host...")
                self.sshClient = paramiko.SSHClient()
                self.sshClient.set_missing_host_key_policy(paramiko.client.AutoAddPolicy())
                self.sshClient.connect(self.state.sshHostname, port=self.state.sshPort, username=self.state.sshUsername, password=self.state.sshPassword)
                print("Connected to ssh host")

            self.hasResults = False

            # Setup attack program paramaters 
            attackAllArgs = []
            if (self.state.type == EXTERNAL_ATTACK_TYPE_GENERAL):
                attackAllArgs = [self.state.attackProgram]
                attackAllArgs.extend(shlex.split(self.state.attackArgs))
            elif (self.state.type == EXTERNAL_ATTACK_TYPE_FRTRACE):
                attackAllArgs.append(self.state.frProgram)
                attackAllArgs.append("-s")
                attackAllArgs.append(str(self.state.frSlotlen))
                attackAllArgs.append("-c")
                attackAllArgs.append(str(self.state.frMaxSamples))
                attackAllArgs.append("-h")
                attackAllArgs.append(str(self.state.frThreshold))
                attackAllArgs.append("-i")
                attackAllArgs.append(str(self.state.frIdle))

                # TODO: No handling for header information yet
                # if (self.state.frHeader):
                #    attackAllArgs.append("-H")

                attackAllArgs.append("-f")
                attackAllArgs.append(self.state.victimProgram)
                attackAllArgs.append("-p")
                attackAllArgs.append(str(self.state.frPDACount))
                
                attackAddresses = self.state.frMonitor.split()
                for i in range(0,len(attackAddresses),1):
                    attackAllArgs.append("-m")
                    attackAllArgs.append(attackAddresses[i])

                attackAddresses = self.state.frEvict.split()
                for i in range(0,len(attackAddresses),1):
                    attackAllArgs.append("-e")
                    attackAllArgs.append(attackAddresses[i])

                attackAddresses = self.state.frPDA.split()
                for i in range(0,len(attackAddresses),1):
                    attackAllArgs.append("-t")
                    attackAllArgs.append(attackAddresses[i])
                
            victimAllArgs = [self.state.victimProgram]
            victimAllArgs.extend(shlex.split(self.state.victimArgs))

            print("Running attack program...")

            self.running = True
            if (self.state.sshRun):
                # Remote attack, run attack and victim if needed 
                attackCommand = "\"" + attackAllArgs[0] + "\""
                for i in range(1,len(attackAllArgs),1):
                    attackCommand = attackCommand + " " + attackAllArgs[i]
                
                attackStdin, attackStdout, attackStderr = self.sshClient.exec_command(attackCommand)
                if (self.state.victimRun): 
                    # Pause for a moment to allow attacker to start 
                    time.sleep(0.1)
                    print("Running victim program...")
                    victimCommand = "\"" + self.state.victimProgram  + "\" " + self.state.victimArgs
                    victimStdin, victimStdout, victimStderr = self.sshClient.exec_command(victimCommand)
                    
                # Wait until done
                while (attackStdout.channel.exit_status_ready()):
                    time.sleep(0.1)
                    if (self.cancelled):
                        print("Attack cancelled")
                        self.cancelled = False
                        return

                self.rawResults = attackStdout.read().decode('utf-8')
                
                # Show errors for debugging
                print("Attack program finished")
                print("Attack STDERR:")
                print(attackStderr.read().decode("utf-8"))
                if (self.state.victimRun):
                    print("Victim STDERR:")
                    print(victimStderr.read().decode("utf-8"))

            else:
                # Run both proccesses
                self.attackProccess = subprocess.Popen(attackAllArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                if (self.state.victimRun):
                    print("Running victim program...")
                    self.victimProccess = subprocess.Popen(victimAllArgs)
                
                # Wait until done
                attackStdout, attackStderr = self.attackProccess.communicate()

                if (self.state.victimRun):
                    self.victimProccess.terminate()

                if (self.cancelled):
                    print("Attack cancelled")
                    self.cancelled = False
                    return

                self.rawResults = attackStdout.decode("utf-8")

                # Show errors for debugging
                print("Attack program finished")
                print("Attack STDERR:")
                print(attackStderr.decode("utf-8"))
                #if (self.state.victimRun):
                #    print("Victim STDERR:")
                #    print(victimStderr.read().decode("utf-8"))

            # Process results if possible
            if (len(self.rawResults) > 0):
                print("Proccessing attack results...")
                startTime = time.time()

                self.results = resultsStringToArray(self.rawResults)

                if (config.printMetrics):
                    print("Finished processing in " + str(time.time() - startTime))

                self.hasResults = True
            else:
                print("No results")
            
            self.running = False

            self.attackProccess = None
            self.victimProccess = None
            print("=== Attack Finished ===")

        except Exception as e:
            errorMessage("Error running attack", str(e))
            
            self.hasResults = False
            self.cancelled = False
            self.running = False
            if (not self.attackProccess is None):
                self.attackProccess.terminate()
            if (not self.victimProccess is None):
                self.victimProccess.terminate()
            if (self.sshClient):
                self.sshClient.close()
                self.sshClient = None

        evt = externalAttackDoneEvent()
        wx.PostEvent(self, evt)
        

    # Cancel the attack running
    def cancel(self):
        print("Cancelling attack...")
        try:
            if (self.running):
                self.hasResults = False
                self.running = False
                self.cancelled = True
                if (self.state.sshRun):
                    if (self.sshClient):
                        self.sshClient.close()
                        print("Closed ssh connection")
                    self.sshClient = None
                else:
                    self.attackProccess.terminate()
                    if (self.state.victimRun):
                        self.victimProccess.terminate()

            print("Cancelled")
        except Exception as e:
            errorMessage("Error cancelling attack", str(e))

         