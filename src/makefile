OBJS = sim/main.cpp \
      sim/LCD_Driver.cpp \
      sim/pio_encoder.cpp \
      sim/toggle_button.cpp \
      sim/arduino.cpp \
	  audio_ampli_mcu/audio_ampli_mcu.ino \
	  audio_ampli_mcu/fonts.cpp \
	  audio_ampli_mcu/options_controller.cpp \
	  audio_ampli_mcu/volume_controller.cpp \
	  audio_ampli_mcu/audio_input_controller.cpp\
	  audio_ampli_mcu/state_machine.cpp

OBJ_NAME = simulator

CC = g++

COMPILER_FLAGS = -Wall

LINKER_FLAGS = -lSDL2

# -DSIM creates a macro that would allow to override the MCU libraries calls
all : $(OBJS)
	$(CC) -x c++ $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS)  -I. -Isim -o $(OBJ_NAME) `sdl2-config --cflags --libs` -DSIM