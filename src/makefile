

CC = g++
COMPILER_FLAGS = -Wall -O0
# -DSIM creates a macro named SIM that would allow to override the MCU libraries calls
LINKER_FLAGS = -I. -Isim  `sdl2-config --cflags --libs` -DSIM
CCFLAGS = $(COMPILER_FLAGS) -x c++ 

# $(PROGRAM).o: $(PROGRAM).cpp
OBJ_DIR = obj
BIN_DIR = bin

SIMULATOR_BIN = $(BIN_DIR)/simulator
GIF_GENERATOR_BIN = $(BIN_DIR)/gif_generator

SIMULATOR_SRC = sim/main_simulator.cpp
GIF_GENERATOR_SRC = sim/main_gif_generator.cpp

COMMON_SRC = sim/lcd_simulator.cpp \
      sim/pio_encoder.cpp \
      sim/MCP23S17.cpp \
      sim/TinyIRReceiver.cpp \
      sim/toggle_button.cpp \
      sim/RP2040_PWM.cpp \
      sim/arduino.cpp \
      sim/SPI.cpp \
	  audio_ampli_mcu/audio_ampli_mcu.ino \
	  audio_ampli_mcu/draw_primitives.cpp \
	  audio_ampli_mcu/LCD_Driver.cpp \
	  audio_ampli_mcu/persistent_data.cpp \
	  audio_ampli_mcu/options_controller.cpp \
	  audio_ampli_mcu/io_expander.cpp \
	  audio_ampli_mcu/volume_controller.cpp \
	  audio_ampli_mcu/remote_controller.cpp \
	  audio_ampli_mcu/audio_input_controller.cpp\
	  audio_ampli_mcu/options_view.cpp \
	  audio_ampli_mcu/interaction_handler.cpp \
	  audio_ampli_mcu/main_menu_view.cpp \
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

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
