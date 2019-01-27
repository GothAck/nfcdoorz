P_NON_MAIN_OBJECTS = $(foreach o,$(NON_MAIN_OBJECTS),../../$(o))
SYM_FILES = $(P_NON_MAIN_OBJECTS:%=%.syms)

SYM_SH = $(ROOT_PATH)/util/sym.sh

HEADERS = /usr/include/nfc/nfc.h /usr/include/freefare.h

SOURCES = $(wildcard *.c) $(wildcard *.cpp)
OBJECTS = $(foreach f,$(basename $(SOURCES)),$(ROOT_PATH)/build/test/stub/$(f).o)

SHARED = $(OBJECTS:%.o=%.so)

all: $(notdir $(HEADERS)) stubs $(ROOT_PATH)/build/test/stub/stub.so

stubs: $(SHARED)

define gen_h
$(notdir $(1)): $(1) syms.imported
	$$(SYM_SH) syms.imported $$< > $$@

$(1):
endef

$(foreach h,$(HEADERS),$(eval $(call gen_h,$(h))))

$(ROOT_PATH)/build/test/stub/stub.so: $(OBJECTS)
	gcc -shared -o $@ $(OBJECTS)

%.so: %.o
	gcc -shared -o $@ $<

$(ROOT_PATH)/build/test/stub/%.o: %.c
	@echo "[stub/cc] $<"
	@mkdir -p $(dir $@)
	@gcc -fPIC -c $(C_FLAGS) -std=c99 -O$(OPTIMIZE) -o $@ $< $(LIBS_C) $(SYS_LIBS:%=-l%) -ldl

syms.imported: syms.defined syms.undefined Makefile.mk
	comm syms.defined syms.undefined -1 -3 > $@

syms.undefined: $(P_NON_MAIN_OBJECTS) Makefile.mk
	for obj in $(P_NON_MAIN_OBJECTS); do nm -fs --undefined-only $$obj | cut -d'|' -f1 | tail -n+7; done | sort | uniq > $@

syms.defined: $(P_NON_MAIN_OBJECTS) Makefile.mk
	for obj in $(P_NON_MAIN_OBJECTS); do nm -fs --defined-only $$obj | cut -d'|' -f1 | tail -n+7; done | sort | uniq > $@

clean:
	-@rm -f $(notdir $(HEADERS)) syms.imported syms.undefined syms.defined

.RHONY: all clean
