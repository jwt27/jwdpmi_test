program_exists = $(shell which $(1) > /dev/null && echo $(1))
pick_tool = $(or $(call program_exists, $(join i686-pc-msdosdjgpp-,$(1))), $(1))

CXX := $(or $(shell echo $$CXX), $(call pick_tool, g++))
AR := $(or $(shell echo $$AR), $(call pick_tool, ar))
OBJDUMP := $(or $(shell echo $$OBJDUMP), $(call pick_tool, objdump))
STRIP := $(or $(shell echo $$STRIP), $(call pick_tool, strip))

FDD := $(or $(FDD), /a)

CXXFLAGS += -pipe
CXXFLAGS += -masm=intel
CXXFLAGS += -MD -MP
CXXFLAGS += -O3 -ffast-math
#CXXFLAGS += -O0 -ffast-math
#CXXFLAGS += -flto -flto-odr-type-merging
CXXFLAGS += -ggdb3 -gsplit-dwarf
CXXFLAGS += -floop-nest-optimize -fgraphite-identity
#CXXFLAGS += -march=pentium3 -mfpmath=both
CXXFLAGS += -march=pentium-mmx
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
CXXFLAGS += -fstrict-volatile-bitfields
CXXFLAGS += -D_DEBUG
#CXXFLAGS += -fopt-info-missed
#CXXFLAGS += -fsanitize=undefined -fsanitize-undefined-trap-on-error
#CXXFLAGS += -fsanitize=address

#CXXFLAGS += -save-temps

#LDFLAGS += -Wl,-Map,bin/debug.map
LDFLAGS += -Wno-attributes

INCLUDE := -iquote include -Ilib/libjwdpmi/include
LIBS := -Llib/libjwdpmi/bin -ljwdpmi
LIBJWDPMI := lib/libjwdpmi/bin/libjwdpmi.a

SRCDIR := src
OUTDIR := bin
OBJDIR := obj

SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEP := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d)
EXE := $(SRC:$(SRCDIR)/%.cpp=$(OUTDIR)/%.exe)
EXE_DEBUG := $(SRC:$(SRCDIR)/%.cpp=$(OUTDIR)/%-debug.exe)
ASM := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.asm)
ASMDUMP := $(SRC:$(SRCDIR)/%.cpp=$(OUTDIR)/%.asm)
PREPROCESSED := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.ii)

ifneq ($(findstring vs,$(MAKECMDGOALS)),)
    PIPECMD := 2>&1 | gcc2vs
else 
    PIPECMD :=
endif

.PHONY: all clean vs libjwdpmi asm preprocessed

all: $(EXE) $(EXE_DEBUG) $(ASMDUMP)

preprocessed: $(PREPROCESSED)
	$(MAKE) preprocessed -C lib/libjwdpmi/

asm: $(ASM) $(ASMDUMP)
	$(MAKE) asm -C lib/libjwdpmi/

clean:
	rm -f $(OBJ) $(DEP) $(ASM) $(PREPROCESSED) $(ASMDUMP) $(EXE) $(EXE_DEBUG)
	$(MAKE) clean -C lib/libjwdpmi/

vs: tasks.vs.json launch.vs.json
	@echo "void main(){}" > _temp.cpp
	$(CXX) -dM -E $(CXXFLAGS) _temp.cpp > tools/gcc_defines.h
	@rm _temp.*

tasks.vs.json:
	./tools/generate-vs-tasks.sh

launch.vs.json:
	./tools/generate-vs-launch.sh

export CC CXX AR CXXFLAGS PIPECMD
libjwdpmi:
	cp -u lib/jwdpmi_config.h lib/libjwdpmi/jwdpmi_config.h
	$(MAKE) -C lib/libjwdpmi/

$(LIBJWDPMI): libjwdpmi

$(OUTDIR): 
	mkdir -p $(OUTDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTDIR)/%-debug.exe: $(OBJDIR)/%.o $(LIBJWDPMI) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(PIPECMD)
#	stubedit $@ dpmi=hdpmi32.exe

$(OUTDIR)/%.exe: $(OUTDIR)/%-debug.exe | $(OUTDIR)
	cp $< $@
	$(STRIP) -S $@
	upx --best $@
	touch $@

$(FDD)/%.exe: $(OUTDIR)/%.exe
	-[ -d $(dir $@) ] && rsync -vu --inplace --progress $< $@ # copy to floppy

$(OUTDIR)/%.asm: $(OUTDIR)/%.exe | $(OUTDIR)
	$(OBJDUMP) -M intel-mnemonic --insn-width=10 -C -w -d $< > $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ -MF $(@:.o=.d) $(INCLUDE) -c $< $(PIPECMD)

$(OBJDIR)/%.asm: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -S -o $@ $(INCLUDE) -c $<

$(OBJDIR)/%.ii: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -E -o $@ $(INCLUDE) -c $<

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEP)
endif
