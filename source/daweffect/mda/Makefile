# DAW Plugins for Web Browsers
# Jari Kleimola 2014-15
# mda plugins web port


TARGET = web/mda_leslie.js


ROOT = ../..
HOST = $(ROOT)/host/cpp
PLUG = $(HOST)/plugformats
VST  = $(HOST)/inc/vst2x
INC = -I$(HOST) -I$(PLUG) -I$(HOST)/inc -I$(VST)

SRC = $(HOST)/edawhost.cc $(HOST)/dawhost.cc $(HOST)/dawplugin.cc $(HOST)/utils.cc \
$(PLUG)/vst.cc $(VST)/audioeffect.cpp $(VST)/audioeffectx.cpp \
src/mdaLeslie.cpp

EXPORTS = "['_createPlugin','_getBuffer','_process','_onMidi','_setParam']"

CFLAGS = $(INC) -DVST -DENTRY=2
LDFLAGS = -O2 --llvm-opts 2 --memory-init-file 0

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) -s EXPORTED_FUNCTIONS=$(EXPORTS)
