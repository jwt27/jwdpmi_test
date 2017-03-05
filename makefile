CC := gcc
CXX := g++
CXXFLAGS += -pipe
CXXFLAGS += -masm=intel
CXXFLAGS += -MD -MP
CXXFLAGS += -O3 -flto=24 -flto-odr-type-merging
CXXFLAGS += -std=gnu++17
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wno-attributes
# CXXFLAGS += -Wdisabled-optimization -Winline 
# CXXFLAGS += -Wsuggest-attribute=pure 
# CXXFLAGS += -Wsuggest-attribute=const
# CXXFLAGS += -Wsuggest-final-types -Wsuggest-final-methods 
CXXFLAGS += -Wsuggest-override
# CXXFLAGS += -Wpadded
# CXXFLAGS += -Wpacked
# CXXFLAGS += -fno-omit-frame-pointer
CXXFLAGS += -gdwarf-4
CXXFLAGS += -funwind-tables -fasynchronous-unwind-tables
CXXFLAGS += -fnon-call-exceptions 
CXXFLAGS += -mcld
CXXFLAGS += -mpreferred-stack-boundary=4
# CXXFLAGS += -save-temps

LDFLAGS += -Wl,-Map,bin/debug.map

INCLUDE := -iquote include -Ilib/libjwdpmi/include
LIBS := -Llib/libjwdpmi/bin -ljwdpmi

OUTPUT := dpmitest.exe

SRCDIR := src
OUTDIR := bin
OBJDIR := obj
SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEP := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d)
VPATH := .:$(SRCDIR)

ifeq ($(MAKECMDGOALS),vs)
    PIPECMD := 2>&1 | gcc2vs
else 
    PIPECMD :=
endif

.PHONY: all clean vs 

all: $(OBJDIR) $(OUTDIR) $(OUTDIR)/$(OUTPUT)

clean:
	-rm -rf obj/*
    
vs: all
	@echo "void main(){}" > _temp.cpp
	$(CXX) -dM -E $(CXXFLAGS) _temp.cpp > tools/gcc_defines.h
	@rm _temp.*

export CC CXX CXXFLAGS PIPECMD
libjwdpmi:
	$(MAKE) -C lib/libjwdpmi/

$(OUTDIR): 
	-mkdir $(OUTDIR)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OUTDIR)/$(OUTPUT): $(OBJ) libjwdpmi
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(LIBS) $(PIPECMD)
	objdump -M intel-mnemonic --insn-width=10 -C -w -d $@ > $(OUTDIR)/main.asm
#	stubedit $@ dpmi=hdpmi32.exe

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -MF $(@:.o=.d) $(INCLUDE) -c $< $(PIPECMD)

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEP)
endif
