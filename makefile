program_exists = $(shell which $(1) > /dev/null && echo $(1))
pick_tool = $(or $(call program_exists, $(join i586-pc-msdosdjgpp-,$(1))), $(1))

CXX := $(or $(shell echo $$CXX), $(call pick_tool, g++))
AR := $(or $(shell echo $$AR), $(call pick_tool, ar))
OBJDUMP := $(or $(shell echo $$OBJDUMP), $(call pick_tool, objdump))
STRIP := $(or $(shell echo $$STRIP), $(call pick_tool, strip))

FDD := $(or $(FDD), /a)

CXXFLAGS += -pipe
CXXFLAGS += -masm=intel
CXXFLAGS += -MD -MP
#CXXFLAGS += -O3 -ffast-math
CXXFLAGS += -Og -ggdb3 -ffast-math
#CXXFLAGS += -flto -flto-odr-type-merging
CXXFLAGS += -floop-nest-optimize -fgraphite-identity
CXXFLAGS += -march=pentium3 -mfpmath=both
#CXXFLAGS += -march=pentium
CXXFLAGS += -std=gnu++17 -fconcepts
CXXFLAGS += -Wall -Wextra
# CXXFLAGS += -Wdisabled-optimization -Winline 
# CXXFLAGS += -Wsuggest-attribute=pure 
# CXXFLAGS += -Wsuggest-attribute=const
# CXXFLAGS += -Wsuggest-final-types -Wsuggest-final-methods 
CXXFLAGS += -Wsuggest-override
# CXXFLAGS += -Woverloaded-virtual
# CXXFLAGS += -Wpadded
# CXXFLAGS += -Wpacked
#CXXFLAGS += -fno-omit-frame-pointer
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

OUTPUT := dpmitest-debug.exe
OUTPUT_PACKED := dpmitest.exe
OUTPUT_DUMP := main.asm

SRCDIR := src
OUTDIR := bin
OBJDIR := obj
SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEP := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d)
ASM := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.asm)
PREPROCESSED := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.ii)

ifneq ($(findstring vs,$(MAKECMDGOALS)),)
    PIPECMD := 2>&1 | gcc2vs
else 
    PIPECMD :=
endif

.PHONY: all clean vs libjwdpmi

all: $(OUTDIR)/$(OUTPUT_PACKED) $(OUTDIR)/$(OUTPUT_DUMP) $(FDD)/$(OUTPUT_PACKED)

preprocessed: $(PREPROCESSED)
	$(MAKE) preprocessed -C lib/libjwdpmi/

asm: $(ASM)
	$(MAKE) asm -C lib/libjwdpmi/

clean:
	rm -f $(OBJ) $(DEP) $(OUTDIR)/$(OUTPUT)
	$(MAKE) clean -C lib/libjwdpmi/

vs:
	@echo "void main(){}" > _temp.cpp
	$(CXX) -dM -E $(CXXFLAGS) _temp.cpp > tools/gcc_defines.h
	@rm _temp.*

export CC CXX AR CXXFLAGS PIPECMD
libjwdpmi:
	cp -u lib/jwdpmi_config.h lib/libjwdpmi/jwdpmi_config.h
	$(MAKE) -C lib/libjwdpmi/

$(OUTDIR): 
	mkdir -p $(OUTDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTDIR)/$(OUTPUT): $(OBJ) libjwdpmi | $(OUTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(LIBS) $(PIPECMD)
#	stubedit $@ dpmi=hdpmi32.exe

$(OUTDIR)/$(OUTPUT_PACKED): $(OUTDIR)/$(OUTPUT) | $(OUTDIR)
	cp $< $@
	$(STRIP) -S $@
	upx --best $@

$(FDD)/$(OUTPUT_PACKED): $(OUTDIR)/$(OUTPUT_PACKED)
	-[ -d $(dir $@) ] && rsync -vu --inplace --progress $< $@ # copy to floppy

$(OUTDIR)/$(OUTPUT_DUMP): $(OUTDIR)/$(OUTPUT) | $(OUTDIR)
	$(OBJDUMP) -M intel-mnemonic --insn-width=10 -C -w -d $< > $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ -MF $(@:.o=.d) $(INCLUDE) -c $< $(PIPECMD)

$(OBJDIR)/%.asm: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -S -o $@ $(INCLUDE) -c $< $(PIPECMD)

$(OBJDIR)/%.ii: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -E -o $@ $(INCLUDE) -c $<

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEP)
endif
