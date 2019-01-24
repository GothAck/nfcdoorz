export ROOT_PATH = $(CURDIR)

SOURCES = $(shell find . \( -path ./external -o -path ./test/stub \) -prune -type f -o \( -name '*.cpp' -o -name '*.c' \) -printf '%P\n')
export OBJECTS = $(foreach f,$(basename $(SOURCES)),build/$(f).o)
export NON_MAIN_OBJECTS = $(filter-out build/test/%,$(filter-out build/apps/%, $(OBJECTS)))
STRIPPED = $(foreach f,$(basename $(SOURCES)),build/stripped/$(f))

APPS_BIN = $(foreach f,$(basename $(wildcard apps/*.cpp)),build/$(f))
TEST_BIN = $(foreach f,$(basename $(wildcard test/test-*.cpp)),build/$(f))

TEST_ASSETS = $(foreach f,$(wildcard test/*.yaml),build/$(f))

SYS_LIBS = nfc freefare yaml-cpp docopt crypto
TEST_LIBS =

INCDIRS = .
LIBDIRS =

OPTIMIZE = 2

DEP_C_FLAGS = -MT $@ -MD -MP -MF build/$*.Td

C_FLAGS = $(DEP_C_FLAGS) -pipe \
	$(INCDIRS:%=-I%) $(LIBDIRS:%=-L%) \
	-pedantic -Wextra \
	-Du_int8_t=uint8_t -Du_int16_t=uint16_t \
	-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED

CPP_FLAGS = -fconcepts

all: $(OBJECTS) $(APPS_BIN)

test: $(TEST_BIN)
	@$(MAKE) stub
	@echo "[test]"
	@for obj in $(TEST_BIN); do LD_PRELOAD=build/test/stub/stub.so ./$$obj; done

external:
	@$(MAKE) -C external

-include $(patsubst %,%.Td,$(OBJECTS))

build/stripped/%: build/%
	@echo "[strip] $<"
	@mkdir -p build/stripped
	@strip -o $@ $<

build/test/%.o: INCDIRS += ./external/catch2/single_include/catch2


build/test/%.yaml: test/%.yaml
	@echo "[test asset] $<"
	@mkdir -p build/test
	@cp $< $@

$(APPS_BIN): %: %.o $(NON_MAIN_OBJECTS)
	@echo "[ld] $@"
	@mkdir -p build/apps
	@g++ -O$(OPTIMIZE) -o $@ $< $(NON_MAIN_OBJECTS) $(SYS_LIBS:%=-l%)

$(TEST_BIN): %: %.o build/test/main.o $(NON_MAIN_OBJECTS) | $(TEST_ASSETS)
	@echo "[ld] $@"
	@mkdir -p build/test
	@g++ -Wl,--export-dynamic -O$(OPTIMIZE) $(C_FLAGS) -o $@ $< build/test/main.o $(NON_MAIN_OBJECTS) $(SYS_LIBS:%=-l%) $(TEST_LIBS:%=-l%) -l stdc++fs

build/%.o: %.c Makefile
	@echo "[cc] $<"
	@mkdir -p $(dir $@)
	@gcc -c $(C_FLAGS) -std=c99 -O$(OPTIMIZE) -o $@ $< $(LIBS_C) $(SYS_LIBS:%=-l%)

build/%.o: %.cpp Makefile
	@echo "[cpp] $<"
	@mkdir -p $(dir $@)
	@g++ -c $(C_FLAGS) $(CPP_FLAGS) -std=c++2a -O$(OPTIMIZE) -o $@ $< $(LIBS_C) $(LIBS_CPP) $(SYS_LIBS:%=-l%)

clean:
	@echo "[clean]"
	-@rm -rf build/*

stub:
	$(MAKE) -C test/stub -f Makefile.mk

stub/%:
	make -C test/stub -f Makefile.mk $(@:stub/%=%)

.PHONY: clean test external stub
