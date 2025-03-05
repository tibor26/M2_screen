# M2 Screen Firmware

Firmware for the M2 Screen using STM32U5F9ZJJ6Q chip.

The Screen board communicates with the Control Board using SPI. It reads the user inputs from the encoder and the button

Inputs:

- encoder
- button

Interfaces:

- SPI for communication with the Control board
- Mipi for the TFT screen
- QSPI for the external Flash (not used yet)

### Submodules
The repository use git submodule for the UI (in `GUI/M2_ui`) and LVGL (in `GUI/lvgl`). When the repository is downloaded or cloned those folders will be empty. To set up the submodules use the command `git submodule update --init --recursive`.

Or the folders can be downloaded manually:
- M2_ui from https://bitbucket.org/nama-product/m2_ui
- LVGL version v9.2.0 from https://github.com/lvgl/lvgl/releases/tag/v9.2.0 (make sure to rename the folder to `lvgl`)

### Timers
- Timer 1 is used for the screen backlight using channel 4 (800 kHz)
- Timer 2 is used to schedule LVGL update (runs at 4kHz)
- Timer 6 is used to measure some code runtime duration (1 MHz)
- Timer 7 is used for delay functions (1 MHz)

### Display driver
The display is initialized using the `tft_initialize()` function. The color depth is set to 16 bit (2 bytes) using command `3Ah` set to `0x05`.
The function also sets up the Mipi peripheral.

The display update is done with by writting in the screen sized framebuffer. The buffer is then automatically sent to the sreen using Mipi.

### Compiller optimizations
To get a better performance with LVGL the code needs to be compilled using the `-O3` setting to enable the maximum optimization level.

### LVGL
Using LVGL version 9.2.0

Display is set using one screen sized buffer (400 x 400 x 2 = 313 kiB) with `LV_DISPLAY_RENDER_MODE_DIRECT`

`lv_tick_inc(1)` is called in `SysTick_Handler()`

`lv_timer_handler()` is called in `main()`. The Timer 2 is used to wait for the next call.

### UI
UI designed using EEZ Studio.

Images set to RGB565.

Screens:
- Nama logo
- Blender modes roller
- Milk Maker modes roller
- Blender manual mode with arc to select the speed
- Blender program
- Milk maker program
