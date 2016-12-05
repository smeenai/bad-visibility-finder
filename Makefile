# Set these appropriately
LLVM_SRC := $$HOME/llvm/llvm
LLVM_BUILD := $$HOME/llvm/build/Release
LIBCXX_SRC := $(LLVM_SRC)/projects/libcxx

LLVM_CONFIG := $(LLVM_BUILD)/bin/llvm-config
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs)
LLVM_SYSTEM_LIBS := $(shell $(LLVM_CONFIG) --system-libs)

CLANG_INCLUDES := \
    -I$(LLVM_SRC)/tools/clang/include \
    -I$(LLVM_BUILD)/tools/clang/include

LIBCXX_INCLUDE_DIR := $(LIBCXX_SRC)/include

CLANG_LIBS := \
    -lclangAST \
    -lclangAnalysis \
    -lclangBasic \
    -lclangDriver \
    -lclangEdit \
    -lclangFrontend \
    -lclangLex \
    -lclangParse \
    -lclangSema \
    -lclangSerialization \
    -lclangTooling

ifeq (,$(findstring unknown option,$(shell ld --start-group 2>&1)))
    CLANG_LIBS_START := -Wl,--start-group
    CLANG_LIBS_END := -Wl,--end-group
endif

CPPFLAGS := $(CLANG_INCLUDES)
CXXFLAGS := $(LLVM_CXXFLAGS) -fno-strict-aliasing # GCC complains about strict aliasing violations
LDFLAGS := $(LLVM_LDFLAGS)
LDLIBS := $(CLANG_LIBS_START) $(CLANG_LIBS) $(CLANG_LIBS_END) $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS)

all: run-bad-visibility-finder

# I can't figure out how to get response files to work :(
run-bad-visibility-finder: bad-visibility-finder libcxx_header_includes.cpp system_includes.rsp
	./$< libcxx_header_includes.cpp -- -I$(LIBCXX_INCLUDE_DIR) $(shell cat system_includes.rsp)

bad-visibility-finder: bad-visibility-finder.o
	$(CXX) $(LDFLAGS) -o $@ $< $(LDLIBS)

bad-visibility-finder.o: bad-visibility-finder.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

libcxx_header_includes.cpp:
	find $(LIBCXX_INCLUDE_DIR) -maxdepth 1 -type f \
	    -not -name '*.*' -not -name '__*' -exec basename {} \; | \
	    sed -Ee 's/(.*)/#include <\1>/' > $@

# Super hacky
system_includes.rsp:
	$(CC) -E -v - < /dev/null 2>&1 | \
	    awk '/#include <\.\.\.> search starts here:/{ headers = 1; next } /End of search list/{ headers = 0 } headers' | \
	    sed -Ene '/framework directory/!s/^ *(.*)/-isystem \1/p' | \
	    paste -sd ' ' - > system_includes.rsp

check: bad-visibility-finder
	$(LLVM_BUILD)/bin/llvm-lit --param filecheck_path=$(LLVM_BUILD)/bin/FileCheck -sv test

clean:
	rm -rf *.o *.dSYM bad-visibility-finder libcxx_header_includes.cpp system_includes.rsp test/Output
