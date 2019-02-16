export ROOT_PATH = $(CURDIR)

SOURCES = $(shell find . \( -path ./external -o -path ./test/stub -o -path "*/node_modules/*" \) -prune -type f -o \( -name '*.cpp' -o -name '*.c' -o -name '*.c++' \) -printf '%P\n')
export OBJECTS = $(foreach f,$(basename $(SOURCES)),build/$(f).o)
export NON_MAIN_OBJECTS = $(filter-out build/test/%,$(filter-out build/apps/%, $(filter-out build/util/%, $(OBJECTS))))
STRIPPED = $(foreach f,$(basename $(SOURCES)),build/stripped/$(f))

FLATBUF_SOURCES = $(wildcard flatbuf/*.fbs)
FLATBUF_OUTPUT = $(FLATBUF_SOURCES:%.fbs=%_generated.h)

APPS_BIN = $(foreach f,$(basename $(wildcard apps/*.cpp)),build/$(f))
TEST_BIN = $(foreach f,$(basename $(wildcard test/test-*.cpp)),build/$(f))
UTIL_BIN = $(foreach f,$(basename $(wildcard util/*.cpp)),build/$(f))

TEST_ASSETS = $(foreach f,$(wildcard test/*.yaml),build/$(f))

SYS_LIBS = nfc freefare yaml-cpp docopt crypto seasocks pthread stdc++fs uv util lua5.3
TEST_LIBS = dl

EXTERNAL_INCS = plog/include uvw/src cppcodec json/single_include sol2

INCDIRS = . $(EXTERNAL_INCS:%=external/%) /usr/include/lua5.3
LIBDIRS = ./external

OPTIMIZE = 2

DEP_C_FLAGS = -MT $@ -MD -MP -MF build/$*.Td

C_FLAGS = $(DEP_C_FLAGS) -pipe \
	$(INCDIRS:%=-I%) $(LIBDIRS:%=-L%) \
	-pedantic -Wextra \
	-Du_int8_t=uint8_t -Du_int16_t=uint16_t \
	-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE

CPP_FLAGS = -fconcepts -std=c++2a

all: $(OBJECTS) $(APPS_BIN)

test: $(TEST_BIN)
	@$(MAKE) stub
	@echo "[test]"
	@for obj in $(TEST_BIN); do LD_PRELOAD=build/test/stub/stub.so ./$$obj; done

util: $(OBJECTS) $(UTIL_BIN)

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
	@g++ -O$(OPTIMIZE) -o $@ $< $(NON_MAIN_OBJECTS) $(LIBDIRS:%=-L%) $(SYS_LIBS:%=-l%)

$(UTIL_BIN): %: %.o $(NON_MAIN_OBJECTS)
	@echo "[ld] $@"
	@mkdir -p build/apps
	@g++ -O$(OPTIMIZE) -o $@ $< $(NON_MAIN_OBJECTS) $(LIBDIRS:%=-L%) $(SYS_LIBS:%=-l%)

$(TEST_BIN): %: %.o build/test/main.o $(NON_MAIN_OBJECTS) | $(TEST_ASSETS)
	@echo "[ld] $@"
	@mkdir -p build/test
	@g++ -Wl,--export-dynamic -O$(OPTIMIZE) $(C_FLAGS) -o $@ $< build/test/main.o $(NON_MAIN_OBJECTS) $(LIBDIRS:%=-L%) $(SYS_LIBS:%=-l%) $(TEST_LIBS:%=-l%)

build/%.o: %.c Makefile $(FLATBUF_OUTPUT)
	@echo "[cc] $<"
	@mkdir -p $(dir $@)
	@gcc -c $(C_FLAGS) -std=c99 -O$(OPTIMIZE) -o $@ $< $(LIBS_C) $(SYS_LIBS:%=-l%)

build/%.o: %.cpp Makefile $(FLATBUF_OUTPUT)
	@echo "[cpp] $<"
	@mkdir -p $(dir $@)
	@g++ -c $(C_FLAGS) $(CPP_FLAGS) -O$(OPTIMIZE) -o $@ $< $(LIBS_C) $(LIBS_CPP) $(SYS_LIBS:%=-l%)

build/%.o: %.c++ Makefile $(FLATBUF_OUTPUT)
	@echo "[cpp] $<"
	@mkdir -p $(dir $@)
	@g++ -c $(C_FLAGS) $(CPP_FLAGS) -O$(OPTIMIZE) -o $@ $< $(LIBS_C) $(LIBS_CPP) $(SYS_LIBS:%=-l%)

clean:
	@echo "[clean]"
	-@rm -rf build/*

flatbuf/%_generated.h: flatbuf/%.fbs Makefile
	@echo "[flatc] $<"
	@flatc -o flatbuf/ --scoped-enums --reflect-names --gen-name-strings --gen-object-api -c $<

stub:
	$(MAKE) -C test/stub -f Makefile.mk

stub/%:
	make -C test/stub -f Makefile.mk $(@:stub/%=%)

.PHONY: clean test external stub
