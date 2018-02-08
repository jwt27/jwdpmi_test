program_exists = $(shell which $(1) > /dev/null && echo $(1))
pick_tool = $(or $(call program_exists, $(join i586-pc-msdosdjgpp-,$(1))), $(1))

CXX := $(or $(shell echo $$CC), $(call pick_tool, g++))
AR := $(or $(shell echo $$AR), $(call pick_tool, ar))
OBJDUMP := $(or $(shell echo $$AR), $(call pick_tool, objdump))
CXXFLAGS += -pipe
CXXFLAGS += -masm=intel
CXXFLAGS += -MD -MP
CXXFLAGS += -O3 -flto -flto-odr-type-merging
CXXFLAGS += -march=pentium3 -ffast-math -mfpmath=both
#CXXFLAGS += -march=pentium-mmx -ffast-math
CXXFLAGS += -std=gnu++17
CXXFLAGS += -Wall -Wextra
# CXXFLAGS += -Wdisabled-optimization -Winline 
# CXXFLAGS += -Wsuggest-attribute=pure 
# CXXFLAGS += -Wsuggest-attribute=const
# CXXFLAGS += -Wsuggest-final-types -Wsuggest-final-methods 
CXXFLAGS += -Wsuggest-override
# CXXFLAGS += -Woverloaded-virtual
# CXXFLAGS += -Wpadded
# CXXFLAGS += -Wpacked
# CXXFLAGS += -fno-omit-frame-pointer
CXXFLAGS += -ggdb
CXXFLAGS += -fnon-call-exceptions -fasynchronous-unwind-tables
CXXFLAGS += -mcld
CXXFLAGS += -mpreferred-stack-boundary=4
CXXFLAGS += -mstackrealign
CXXFLAGS += -fstrict-volatile-bitfields
CXXFLAGS += -D_DEBUG
#CXXFLAGS += -save-temps

#LDFLAGS += -Wl,-Map,bin/debug.map

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

ifneq ($(findstring vs,$(MAKECMDGOALS)),)
    PIPECMD := 2>&1 | gcc2vs
else 
    PIPECMD :=
endif

.PHONY: all clean vs 

all: $(OUTDIR)/$(OUTPUT)

clean:
	rm -f $(OBJ) $(DEP) $(OUTDIR)/$(OUTPUT)
	$(MAKE) clean -C lib/libjwdpmi/

vs:
	@echo "void main(){}" > _temp.cpp
	$(CXX) -dM -E $(CXXFLAGS) _temp.cpp > tools/gcc_defines.h
	@rm _temp.*

export CC CXX CXXFLAGS PIPECMD
libjwdpmi:
	cp -u lib/jwdpmi_config.h lib/libjwdpmi/jwdpmi_config.h
	$(MAKE) -C lib/libjwdpmi/

$(OUTDIR): 
	mkdir -p $(OUTDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTDIR)/$(OUTPUT): $(OBJ) libjwdpmi | $(OUTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(LIBS) $(PIPECMD)
#	cp lib/libjwdpmi/jwdpmi_config.h lib/libjwdpmi/jwdpmi_config_default.h
	$(OBJDUMP) -M intel-mnemonic --insn-width=10 -C -w -d $@ > $(OUTDIR)/main.asm
#	stubedit $@ dpmi=hdpmi32.exe
	upx --best $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ -MF $(@:.o=.d) $(INCLUDE) -c $< $(PIPECMD)

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEP)
endif
