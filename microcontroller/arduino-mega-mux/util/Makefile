SOURCES := mux.c
OBJECTS := $(SOURCES:%.c=%)

all: $(OBJECTS)

%: %.c
	gcc $< -o $@ -Wall

clean:
	rm $(OBJECTS)
