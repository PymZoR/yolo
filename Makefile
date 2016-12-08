CFLAGS = 
SRC    = hrtimer.c gpio_led_fu.c key_input_fu.c metronome_tui_thread.c
LIB    = -lpthread -lrt

all: nc

nc:
	gcc $(CFLAGS) -o out_native $(SRC) $(LIB)
cc:
	arm-linux-gnueabihf-gcc $(CFLAGS) -DBB=1 -o out_target $(SRC) $(LIB)

clean:
	rm -rf out_*
