# PicoMite for Bobricius' PICOMPUTER
<br>
<img width="600" height="450" alt="IMG_Picomputer" src="https://github.com/user-attachments/assets/1c327f12-428c-4841-b9d6-982cc56d3df3" /><br>

INSTALL
-------

After flashing the .uf2 file, connect to PicoMite using the USB TTY, and enter these options :
```
OPTION SYSTEM SPI GP18,GP19,GP4
OPTION COLOURCODE ON
OPTION CPUSPEED (KHz) 200000
OPTION LCDPANEL CONSOLE 7,,,,NOSCROLL
OPTION DISPLAY 30, 53
OPTION LCDPANEL ST7789_320, RLANDSCAPE,GP16,GP17,GP21,GP20
OPTION SDCARD GP13, GP10, GP11, GP12
OPTION AUDIO GP0,GP1', ON PWM CHANNEL 0
OPTION PLATFORM PICOMPUTER
OPTION DEFAULT FONT 7, 1
```

KEYBOARD
--------
<br>SHIFT-DOWN : select LowerCase (then the shift key acts more like an ALT one, to select symbols)
<br>SHIFT-UP : select UpperCase (then the shift key acts more like an ALT one, to select symbols)
<br>SHIFT-RIGHT : select Symbols (then the shift key acts more like an CTRL one, to select control charaters)

RECOMPILING
-----------
Follow the excellent recommendations from https://github.com/madcock/PicoMiteAllVersions<br>
but beware that the list of files to be overridden in the sdk has changed (do NOT replace flash.c and float_sci_m33_vfp.S anymore).

NOTES
--------
- there are a few GPIO collision, but harmless : Picomputer uses GPIO0 for audio, thus reserving GPIO1 for the other channel. I still can read the keyboard matrix connected to it.
- GPIO4 is supposed to be the screen MISO, but is not connected. Connecting it is useless since it won't make the SCROLL faster. I still can read the keyboard matrix connected to it.


(_original readme follows..._)

# PicoMiteRP2350
This contains files to build MMbasic 6.01.00b7 to run on both RP2040 and RP2350<br>
Compile with GCC 13.3.1 arm-none-eabi<br>

<b style="color:red;"> Build with sdk V2.2.0 but replace gpio.c, gpio.h with the ones included here<br></b>

Change CMakeLists.txt line 4 to determine which variant to build<br>
<br>
RP2040<br>
set(COMPILE PICO)<br>
set(COMPILE VGA)<br>
set(COMPILE PICOUSB)<br>
set(COMPILE VGAUSB)<br>
set(COMPILE WEB)<br>
<br>
RP2350<br>
set(COMPILE PICORP2350)<br>
set(COMPILE VGARP2350)<br>
set(COMPILE PICOUSBRP2350)<br>
set(COMPILE VGAUSBRP2350)<br>
set(COMPILE HDMI)<br>
set(COMPILE HDMIUSB)<br>
set(COMPILE WEBRP2350)<br>
<br>
Any of the RP2350 variants or the RP2040 variants can be built by simply changing the set(COMPILE aaaa)<br>
However, to swap between a rp2040 build and a rp2350 build (or visa versa) needs a different build directory.
The process for doing this is as follows:<br>
Close VSCode<br>
Rename the current build directory - e.g. build -> buildrp2040<br>
Rename the inactive build directory - e.g. buildrp2350 -> build<br>
edit CMakeLists.txt to choose a setting for the other chip and save it - e.g.  set(COMPILE PICO) -> set(COMPILE PICORP2350)<br>
Restart VSCode<br>

