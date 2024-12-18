

# OBJ_NAME = simulator

# $(PROGRAMS): $(PROGRAM=$(.TARGET))
# all: $(PROGRAMS)

# $(PROGRAM): $(PROGRAM).cpp $(OBJS)
# 	$(CC) -o $(.TARGET) -x c++  $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS)  -I. -Isim -o $(OBJ_NAME) `sdl2-config --cflags --libs` -DSIM

CC = g++
COMPILER_FLAGS = -Wall -O0
LINKER_FLAGS = -I. -Isim  `sdl2-config --cflags --libs` -DSIM
# # -DSIM creates a macro named SIM that would allow to override the MCU libraries calls
CCFLAGS = $(COMPILER_FLAGS) -x c++ 

# $(PROGRAM).o: $(PROGRAM).cpp
OBJ_DIR = obj
BIN_DIR = bin

SIMULATOR_BIN = $(BIN_DIR)/simulator
GIF_GENERATOR_BIN = $(BIN_DIR)/gif_generator

SIMULATOR_SRC = sim/main_simulator.cpp
GIF_GENERATOR_SRC = sim/main_gif_generator.cpp

COMMON_SRC = sim/LCD_Driver.cpp \
      sim/pio_encoder.cpp \
      sim/MCP23S17.cpp \
      sim/TinyIRReceiver.cpp \
      sim/toggle_button.cpp \
      sim/RP2040_PWM.cpp \
      sim/arduino.cpp \
	  audio_ampli_mcu/audio_ampli_mcu.ino \
	  audio_ampli_mcu/fonts.cpp \
	  audio_ampli_mcu/persistent_data.cpp \
	  audio_ampli_mcu/options_controller.cpp \
	  audio_ampli_mcu/io_expander.cpp \
	  audio_ampli_mcu/volume_controller.cpp \
	  audio_ampli_mcu/remote_controller.cpp \
	  audio_ampli_mcu/audio_input_controller.cpp\
	  audio_ampli_mcu/state_machine.cpp
COMMON_OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(COMMON_SRC))
# Targets
all: $(SIMULATOR_BIN) $(GIF_GENERATOR_BIN)

$(SIMULATOR_BIN): $(SIMULATOR_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CCFLAGS) $(COMMON_SRC) $(SIMULATOR_SRC) -o $@ $(LINKER_FLAGS) 

$(GIF_GENERATOR_BIN): $(GIF_GENERATOR_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CCFLAGS) $(COMMON_SRC) $(GIF_GENERATOR_SRC) -o $@ $(LINKER_FLAGS) 

# $(OBJ_DIR)/%.o: %.cpp
# 	@mkdir -p $(OBJ_DIR)/sim $(OBJ_DIR)/audio_ampli_mcu
# 	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean

#g++ -Wall -x c++  -lSDL2 -I. -Isim  `sdl2-config --cflags --libs` -DSIM obj/sim/LCD_Driver.o obj/sim/pio_encoder.o obj/sim/toggle_button.o obj/sim/arduino.o audio_ampli_mcu/audio_ampli_mcu.ino obj/audio_ampli_mcu/fonts.o obj/audio_ampli_mcu/options_controller.o obj/audio_ampli_mcu/volume_controller.o obj/audio_ampli_mcu/audio_input_controller.o obj/audio_ampli_mcu/state_machine.o sim/main_simulator.cpp -o bin/simulator