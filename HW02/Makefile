STARTER_DIR=/comp/111/assignments/scheduler
CC=gcc
LD=$(CC)
CPPFLAGS=-g -std=gnu11 -Wpedantic -Wall -Wextra #-DDEBUG
CFLAGS=-I.
LDFLAGS=
LDLIBS=
OBJECTS=process.o event_queue.o simulation.o
PROGRAMS=sched_rr sched_stcf sched_stride

all: $(PROGRAMS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

sched_rr: sched_rr.o $(OBJECTS)
	$(LD) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

sched_stcf: sched_stcf.o $(OBJECTS)
	$(LD) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

sched_stride: sched_stride.o $(OBJECTS)
	$(LD) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

.PHONY:
clean:
	rm -f *.o $(PROGRAMS)
