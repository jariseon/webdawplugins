# DAW Plugins for Web Browsers
# Jari Kleimola 2014-15
# swh plugins web port


TARGET = web/swh_phasers.js
# TARGET = web/swh_bode.js

ROOT = ../..
HOST = $(ROOT)/host/cpp
PLUG = $(HOST)/plugformats
INC = -I$(HOST) -I$(PLUG) -I$(HOST)/inc 

SRC = $(HOST)/edawhost.cc $(HOST)/dawhost.cc $(HOST)/dawplugin.cc $(HOST)/utils.cc \
$(HOST)/dawbundle.cc $(PLUG)/ladspa.cc \
src/phasers_1217.c
# src/bode_shifter_1431.c

EXPORTS = "['_createPlugin','_getBuffer','_process','_onMidi','_setParam']"

CFLAGS = $(INC) -DLADSPA -DDLLweb
LDFLAGS = -O2 --llvm-opts 2 --memory-init-file 0

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) -s EXPORTED_FUNCTIONS=$(EXPORTS)
