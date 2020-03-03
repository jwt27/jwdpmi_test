program_exists = $(shell which $(1) 2> /dev/null)
pick_tool = $(or $(call program_exists, i386-pc-msdosdjgpp-$(1)), $(1))

CXX := $(or $(shell echo $$CXX), $(call pick_tool,g++))
AR := $(or $(shell echo $$AR), $(call pick_tool,ar))
OBJDUMP := $(or $(shell echo $$OBJDUMP), $(call pick_tool,objdump))
STRIP := $(or $(shell echo $$STRIP), $(call pick_tool,strip))

CXXFLAGS += -pipe
CXXFLAGS += -masm=intel
CXXFLAGS += -flto -flto-odr-type-merging -flto-compression-level=0
CXXFLAGS += -ggdb3 #-gsplit-dwarf
CXXFLAGS += -floop-nest-optimize -fgraphite-identity
CXXFLAGS += -std=gnu++2a -fconcepts
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wno-attributes
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
#CXXFLAGS += -fopt-info-missed
#CXXFLAGS += -fsanitize=undefined -fsanitize-undefined-trap-on-error
#CXXFLAGS += -save-temps

ifneq ($(ARCH),)
    CXXFLAGS += -march=$(ARCH)
else
    #CXXFLAGS += -march=i386
    #CXXFLAGS += -march=pentium
    CXXFLAGS += -march=pentium-mmx
    #CXXFLAGS += -march=k6-3
    #CXXFLAGS += -march=pentium3 -mfpmath=both
    #CXXFLAGS += -march=athlon-xp -mfpmath=both
endif

ifneq ($(DEBUG),0)
    CXXFLAGS += -D_DEBUG -Og -ggdb3
else
    CXXFLAGS += -DNDEBUG -O3 -ffast-math
endif

#LDFLAGS += -Wl,-Map,bin/debug.map
LDFLAGS += -Wno-attributes -Wl,--script=ldscripts/i386go32.x

INCLUDE := -iquote include -Ilib/libjwdpmi/include
LIBS := -Llib/libjwdpmi/bin -ljwdpmi
LIBJWDPMI := lib/libjwdpmi/bin/libjwdpmi.a

SRC := $(wildcard src/*.cpp)
TARGETS := $(SRC:src/%.cpp=%)
OBJ := $(TARGETS:%=obj/%.o)
DEP := $(TARGETS:%=obj/%.d)
EXE := $(TARGETS:%=bin/%.exe)
EXE_DEBUG := $(TARGETS:%=bin/%-debug.exe)
ASM := $(TARGETS:%=obj/%.asm)
ASMDUMP := $(TARGETS:%=bin/%.asm)
DWO := $(TARGETS:%=obj/%.dwo)
PREPROCESSED := $(TARGETS:%=obj/%.ii)

ifneq ($(findstring vs,$(MAKECMDGOALS)),)
    PIPECMD := 2>&1 | gcc2vs
else 
    PIPECMD :=
endif

make-target = $(1): bin/$(1).exe bin/$(1)-debug.exe bin/$(1).asm $$(FDD)/$(1).exe
$(foreach target, $(TARGETS), $(eval $(call make-target,$(target))))

.PHONY: all clean vs libjwdpmi asm preprocessed $(TARGETS)

all: $(TARGETS)

preprocessed: $(PREPROCESSED)
	$(MAKE) preprocessed -C lib/libjwdpmi/

asm: $(ASM) $(ASMDUMP)
	$(MAKE) asm -C lib/libjwdpmi/

clean:
	rm -f $(OBJ) $(DEP) $(ASM) $(PREPROCESSED) $(ASMDUMP) $(DWO) $(EXE) $(EXE_DEBUG)
	$(MAKE) clean -C lib/libjwdpmi/

vs: tasks.vs.json launch.vs.json tools/gcc_defines.h

tools/gcc_defines.h: makefile
	@echo | $(CXX) $(CXXFLAGS) -dM -E -x c++ - > $@

tasks.vs.json:
	./tools/generate-vs-tasks.sh

launch.vs.json:
	./tools/generate-vs-launch.sh

export CC CXX AR CXXFLAGS PIPECMD
libjwdpmi:
	cp -u lib/jwdpmi_config.h lib/libjwdpmi/jwdpmi_config.h
	$(MAKE) -C lib/libjwdpmi/

$(LIBJWDPMI): libjwdpmi

bin:
	mkdir -p bin

obj:
	mkdir -p obj

bin/%-debug.exe: obj/%.o $(LIBJWDPMI) | bin
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(PIPECMD)
#	stubedit $@ dpmi=hdpmi32.exe

bin/%.exe: bin/%-debug.exe | bin
	$(STRIP) -w -R .gnu.lto_* -S $< -o $@
	upx --best $@
	touch $@

$(FDD)/%.exe: bin/%.exe
	-[ ! -z $(FDD) ] && [ -d $(FDD) ] && rm -f $(addsuffix .exe,$(addprefix $(FDD)/,$(TARGETS))) && rsync -vu --inplace --progress $< $@ # copy to floppy

bin/%.asm: bin/%-debug.exe | bin
	$(OBJDUMP) -M intel-mnemonic --insn-width=10 -C -w -d $< > $@

obj/%.o: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -o $@ -MP -MD $(INCLUDE) -c $< $(PIPECMD)

obj/%.asm: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -S -o $@ $(INCLUDE) -c $<

obj/%.ii: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -E -o $@ $(INCLUDE) -c $<

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEP)
endif
