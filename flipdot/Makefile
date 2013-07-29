SOURCES=examples/flip_pipe.c examples/fliptest.c examples/flipclear.c examples/flipclearto0.c examples/flipclearto1.c

CPPFLAGS=-I.
CFLAGS=-O3 -flto -Wall -std=gnu99 -pedantic -funroll-loops -fno-common
LDFLAGS=-lbcm2835 -flto -Wl,--relax,--gc-sections

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=$(SOURCES:.c=)

all: $(EXECUTABLES)

clean:
	-rm $(EXECUTABLES) $(OBJECTS) flipdot.o

flipdot.o: flipdot.c flipdot.h
	$(CC) $(CFLAGS) -ffunction-sections -c -DNOSLEEP flipdot.c -o flipdot.o

$(EXECUTABLES): % : %.o flipdot.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.d: %.c
	$(CC) $(CPPFLAGS) -MM -MT $(<:.c=.o) -MP -MF $@ $<

-include $(SOURCES:.c=.d)
