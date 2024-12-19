# Audio Amplifier MCU
Arduino code for the MCU that controls an audio amplifier. It supports a volume knob, audio input selection and a basic LCD display. A Raspberry Pi Pico is used for the MCU.

# Compile project with Arduino CLI
   1. Install [Arduino CLI](https://arduino.github.io/arduino-cli/1.0/installation/)
   2. Create config: `arduino-cli config init`
   3. The previous step should create a `~/.arduino15/arduino-cli.yaml` file. Edit the config to include the Pi2040 index:
```yaml
board_manager:
    additional_urls:
        - https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

```
  4. Install the board:
```bash
arduino-cli core update-index
arduino-cli core install rp2040:rp2040
```
 5. Install libraries
```bash
arduino-cli lib install rp2040-encoder-library@0.1.2 InputDebounce@1.6.0 MCP23S17@0.5.1 RP2040_PWM@1.7.0 IRremote@4.4.1
```
 6. Compile the project, this will generate a `audio_ampli_mcu.ino.uf2` that can be drag and drop on the Usb storage of the Pi Pico
```bash
arduino-cli compile -b rp2040:rp2040:rpipico  path/to/git/repo/audio_ampli_mcu/src/audio_ampli_mcu/
```
 7. You can upload it by passing the `--upload` option and specifying the port `--port COM5`.