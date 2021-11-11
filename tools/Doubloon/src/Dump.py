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
from tqdm import tqdm
import wx
import re
import platform

import config
from util import *
from State import *
from Events import *


# Manages external attack and victim programs
# Attacks that rely on an external binary (such as FR-trace)
class Dump(wx.EvtHandler):
    def __init__(self, state):
        super(Dump, self).__init__()

        # Copy of the common attack state from application
        self.state = state

        # Attack variables
        self.hasResults = False
        self.hasMapResults = False
        self.cancelled = False
        self.inserted = False
        self.mapCancelled = False
        self.Proccess = None
        self.sshClient = None

        # results
        self.map = dict()
        self.rawResults = ""
        self.rawMapResults = ""
        self.results = None
        self.mapResults = None
        self.sourceCode = []
        self.s2m = dict()
        self.m2s = dict()


    # Create new thread and do runThreaded
    def run(self):
        self.s2m.clear()
        self.m2s.clear()
        self.sourceCode.clear()
        self.rawMapResults = ""
        self.rawResults = ""
        self.results = None
        self.mapResults = None
        self.map.clear()

        try:
            self.mapthread = threading.Thread(target=self.runMapThreaded)
            self.mapthread.start()
            self.mapthread.join()
            self.dumpthread = threading.Thread(target=self.runThreaded)
            self.dumpthread.start()
            self.dumpthread.join()
        except Exception as e:
            errorMessage("Error starting dump binary", str(e))
            

    # Perform an Dump
    def runThreaded(self):
        print("=== Running objdump ===")

        try:
            # If remote, setup the ssh connection if it doesn't already exist
            self.hasResults = False

            # Setup attack program paramaters 
            dumpArgs = []
                # TODO: No handling for header information yet
                # if (self.state.frHeader):
                #    attackAllArgs.append("-H")

            #disassemble
            dumpArgs.append("objdump")
            dumpArgs.append("-lS")
            dumpArgs.append("-j")
            dumpArgs.append(".text")

            dumpArgs.append(self.state.victimProgram)
                

            print("Running objdump")

            self.ran = True
            # Run dump proccesses
            self.Proccess = subprocess.Popen(dumpArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            
            # Wait until done
            dumpOut, dumpError = self.Proccess.communicate()

            if (self.cancelled):
                print("Dump cancelled")
                self.cancelled = False
                return

            self.rawResults = dumpOut.decode("utf-8")

            # Show errors for debugging
            print("Dump program finished")
            print(dumpError.decode("utf-8"))

            # Process results if possible
            if (len(self.rawResults) > 0):
                print("Proccessing dump results...")
                startTime = time.time()

                self.results = self.handleDump(self.rawResults)

                if (config.printMetrics):
                    print("Finished processing in " + str(time.time() - startTime))

                self.hasResults = True
            else:
                print("No results")
            
            self.running = False
            self.Proccess = None
            print("=== Dump Finished ===")

        except Exception as e:
            errorMessage("Error running dump", str(e))
            
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

        evt = dumpDoneEvent()
        wx.PostEvent(self, evt)
        
    # objdump for source code lines to machine code address projection
    def runMapThreaded(self):
        if platform.system() == "Darwin":
            print("Cannot run objdump map")
            self.hasMapResults = False
            evt = mapDoneEvent()
            wx.PostEvent(self, evt)
            return


        print("=== Running objdump decodedline===")

        try:
            self.hasMapResults = False
            # Setup map program paramaters 
            dumpArgs = []
                # TODO: No handling for header information yet
                # if (self.state.frHeader):
                #    attackAllArgs.append("-H")
            #disassemble
            dumpArgs.append("objdump")
            dumpArgs.append("--dwarf=decodedline")

            dumpArgs.append(self.state.victimProgram)
            self.ran_map = True
            # Run dump proccesses
            self.mapProccess = subprocess.Popen(dumpArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            
            # Wait until done
            dumpOut, dumpError = self.mapProccess.communicate()

            #self.mapProccess.terminate()

            if (self.cancelled):
                print("Map cancelled")
                self.cancelled = False
                return

            self.mapRawResults = dumpOut.decode("utf-8")

            # Show errors for debugging
            print("=== Map program finished ===")
            #print("Map STDERR:")
            print(dumpError.decode("utf-8"))

            # Process results if possible
            if (len(self.mapRawResults) > 0):
                print("--Proccessing map results...")
                startTime = time.time()
                #print(self.rawResults)
                self.mapResults = self.handleMap(self.mapRawResults)
                if (config.printMetrics):
                    print("Finished processing in " + str(time.time() - startTime))

                self.hasMapResults = True
            else:
                print("No map results")
            
            self.mapRunning = False
            self.mapProccess = None
            print("=== Map Finished ===")

        except Exception as e:
            errorMessage("Error running dump", str(e))
            
            self.hasMapResults = False
            self.Mapcancelled = False
            self.mapRunning = False
            if (not self.attackProccess is None):
                self.attackProccess.terminate()
            if (not self.victimProccess is None):
                self.victimProccess.terminate()
            if (self.sshClient):
                self.sshClient.close()
                self.sshClient = None
        evt = mapDoneEvent()
        wx.PostEvent(self, evt)

    # Cancel the dump 
    def cancel(self):
        print("Cancelling objdump...")
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
                    self.Proccess.terminate()
                    self.mapProccess.terminate()
            print("Cancelled")
        except Exception as e:
            errorMessage("Error cancelling dump", str(e))





    #convert dumped file listctrl format
    def handleDump(self, raw):
        '''
        This is a code section looks like

        0000000000014de2 <init_compress>:
        init_compress():
        /home/abc/Desktop/gnupg-1.4.13/g10/compress.c:51
        int compress_filter_bz2( void *opaque, int control,
                    IOBUF a, byte *buf, size_t *ret_len);

        static void
        init_compress( compress_filter_context_t *zfx, z_stream *zs )
        {
        14de2:	55                   	push   %rbp
        14de3:	48 89 e5             	mov    %rsp,%rbp
        14de6:	48 83 ec 20          	sub    $0x20,%rsp
        14dea:	48 89 7d e8          	mov    %rdi,-0x18(%rbp)
        14dee:	48 89 75 e0          	mov    %rsi,-0x20(%rbp)
        '''
        '''
        read dump result
        1. append to self.sourceCode
        2. match address to line
        3. 
        '''
        try:
            print("=== Handle dump results ===")
            aregex = re.compile('^[0-9a-fA-F]+ <(.*)>:$') # regex for function name
            lregex = re.compile('^(?:/.*)?/([^/]+\.c):([0-9]+)(?: \(discriminator [0-9]+\))?$') # regex for /home/abc/Desktop/gnupg-1.4.13/g10/compress.c:73 (discriminator 1)
            mregex = re.compile('^[ \t]+[0-9a-f]+:[ \t]+[0-9a-f]+') #regex for assembly line
            functions = re.compile('\n\n(?=^[0-9a-fA-F]+ <.*>:$)',re.M).split(raw)
            #functions = raw.split('\n\n')
            result = []
            for idx, func in tqdm(enumerate(functions),desc="All functions in the binary"):
                lines = func.splitlines()
                if(idx>100):
                    break #debug
                if(aregex.match(lines[0])): #this func is a fucntion disassembly
                    sym_name = aregex.match(lines[0]).group(1)
                    #print("func",sym_name)
                    i = 2
                    while(i<len(lines)):
                        fileName = "f not found"
                        lineNum = -1
                        if(lregex.match(lines[i])):
                            fileName = lregex.match(lines[i]).group(1)
                            lineNum = lregex.match(lines[i]).group(2)
                            i += 1
                            sourceIdx = -1
                            machineIdx = -1
                            while i<len(lines) and (not (mregex.match(lines[i]))) :
                                if(sourceIdx<0):
                                    sourceIdx = len(self.sourceCode)
                                self.sourceCode.append([fileName,lineNum,lines[i]])
                                self.s2m[len(self.sourceCode)-1] = len(result)
                                i += 1
                            #append any source code
                            while (i<len(lines) and mregex.match(lines[i])):
                                line = lines[i]
                                row = line.split('\t')
                                if(len(row)<3):
                                    i += 1
                                    continue
                                #print('row',row)
                                row = row[0:-2]+row[-1].split(maxsplit=2)
                                row[0] = int(row[0][0:-1],16) # convert address string into hex
                                row.insert(1, sym_name)
                                while(len(row)<5):
                                    row.append("")
                                #row[0] = hex(row[0]) # future modification
                                row.insert(0,fileName)
                                row.insert(1,lineNum)
                                #row: sym_name, address, assembly, file, line 
                                #new row: file, line, sym_name, address, assembly
                                result.append(row)
                                self.m2s[len(result)-1] = sourceIdx
                                i += 1
                        else:
                            break
            self.results = result
            #print(self.results)
            #print(self.sourceCode)
        except Exception as e:
            errorMessage("error handling dump", str(e))
        return result
         

    def handleMap(self, raw):
        print("=== handling map result! ===")
        '''
            CU: inftrees.c:
        File name                            Line number    Starting address    View
        inftrees.c                                   108            0x101bfe
        inftrees.c                                   108            0x101c66
        inftrees.c                                   132            0x101c75
        inftrees.c                                   136            0x101c7c
        inftrees.c                                   137            0x101d4c
        inftrees.c                                   139            0x101d5a
        inftrees.c                                   140            0x101d78
        inftrees.c                                   141            0x101d81
        inftrees.c                                   143            0x101d8f
        inftrees.c                                   144            0x101d9d
        '''
        filename = re.compile('^CU: (.*\.c:)')
        files = raw.split('\n\n')
        self.map
        for file in files:
            lines = file.splitlines()
            if(filename.match(lines[0])):
                name = filename.match(lines[0]).group(1)
                self.map[name] = dict()
                for line in lines[2:]:
                    #print(line)
                    items = re.compile('\s+').split(line)
                    #print(items)
                    lineNum = int(items[1], 10)
                    startLine = int(items[2], 16)
                    self.map[name][lineNum] = startLine


