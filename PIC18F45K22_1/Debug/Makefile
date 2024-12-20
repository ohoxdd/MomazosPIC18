###############################################################################
# Makefile for project 
###############################################################################

## General options
PROJECT = 
MCU = PIC18F45K22
COMPILER = "MPLAB XC8"
TARGET = Debug
SHELL = C:\windows\system32\cmd.exe

## Tools general options
ASMFLAGS=-C --errformat="Error at file %%f line %%l column %%c: (%%n) %%s" --warnformat="Warning at file %%f line %%l column %%c: (%%n) %%s" --msgformat="Message at file %%f line %%l column %%c: (%%n) %%s" -G --chip=18F45K22
CCFLAGS=--pass1 --errformat="Error at file %%f line %%l column %%c: (%%n) %%s" --warnformat="Warning at file %%f line %%l column %%c: (%%n) %%s" --msgformat="Message at file %%f line %%l column %%c: (%%n) %%s" -G --chip=18F45K22
LDFLAGS=--output=mcof -G --chip=18F45K22

## Processing Tools
CC=xc8.exe
ASM=xc8.exe
LD=xc8.exe
LIB=xc8.exe
#  C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/Debug/Debug.cof: TOOL=LD
#  Post: rm startup.as
#  Post: rm Debug.as
#     C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/Debug/main.p1: TOOL=CC
#        C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/main.c
#     C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/Debug/GLCD.p1: TOOL=CC
#        C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/GLCD.c
#     C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/Debug/test.p1: TOOL=CC
#        C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/test.c
#     C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/Debug/utils.p1: TOOL=CC
#        C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/utils.c
#     C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/Debug/initPIC.p1: TOOL=CC
#        C:/Users/hector.fernandez.de.sevilla/Documents/CI/MomazosPIC18/PIC18F45K22_1/initPIC.c

# Build tree:
all:	Debug

Debug:	Debug.cof

Debug.cof:	main.p1 GLCD.p1 test.p1 utils.p1 initPIC.p1 
	$(LD) $(LDFLAGS)  -O"Debug.cof" "main.p1" "GLCD.p1" "test.p1" "utils.p1" "initPIC.p1"
	rm startup.as
	rm Debug.as

main.p1:	../main.c 
	$(CC) $(CCFLAGS)  -O"main.p1" "../main.c"

GLCD.p1:	../GLCD.c 
	$(CC) $(CCFLAGS)  -O"GLCD.p1" "../GLCD.c"

test.p1:	../test.c 
	$(CC) $(CCFLAGS)  -O"test.p1" "../test.c"

utils.p1:	../utils.c 
	$(CC) $(CCFLAGS)  -O"utils.p1" "../utils.c"

initPIC.p1:	../initPIC.c 
	$(CC) $(CCFLAGS)  -O"initPIC.p1" "../initPIC.c"

# tidy - delete all temporary files which are not involved in the target generation
tidy:
	rm -rf Debug.hxl
	rm -rf Debug.lst
	rm -rf Debug.sym
	rm -rf Debug.obj
	rm -rf Debug.sdb
	rm -rf Debug.as
	rm -rf ./startup.obj
	rm -rf ./startup.rlf
	rm -rf ./startup.as
	rm -rf ./startup.lst
	rm -rf ./funclist.
	rm -rf Debug.hex
	rm -rf main.pre
	rm -rf main.lst
	rm -rf GLCD.pre
	rm -rf GLCD.lst
	rm -rf test.pre
	rm -rf test.lst
	rm -rf utils.pre
	rm -rf utils.lst
	rm -rf initPIC.pre
	rm -rf initPIC.lst

# cleanup - delete all generated files
clean:	tidy
	rm -rf Debug.cof
	rm -rf main.p1
	rm -rf GLCD.p1
	rm -rf test.p1
	rm -rf utils.p1
	rm -rf initPIC.p1
