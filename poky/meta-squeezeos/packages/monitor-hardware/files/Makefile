SOURCES := monitor_msp430.c

all:    monitor_msp430

monitor_msp430:    $(SOURCES)
	$(CC) -Wall $(CFLAGS) -DUSE_HOSTCC $(SOURCES) -o monitor_msp430 

clean:
	rm -f *.o monitor_msp430
