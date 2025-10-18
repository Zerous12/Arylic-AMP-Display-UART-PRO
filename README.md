# Arylic-AMP-Display-UART-PRO
0.91" Oled Display for Arylic AMP Devices

**Display Control with ESP32 and 0.91” OLED Display**

This project uses an ESP32S together with a 0.91” OLED Display to show information about the status of an audio amplifier. The code allows receiving commands through UART (Serial) communication to switch between different display modes, such as signal source, volume, bass, treble, and more. In addition, a STANDBY mode function has been implemented, which shows a "STANDBY" message on the screen and blinks every 2 seconds until a command is received to return to the SOURCE mode.

**Components Used**

- ESP32 (a NodeMCU ESP32S was used in this example)
- OLED Display (an I2C OLED display of 128x32 pixels was used)
- Audio Amplifier capable of sending UART commands through its serial port (up2Stream PRO\_V4)

**Connections**

Connect the components following these connections:

- OLED Display:
  - Pin VCC to 5V
  - Pin GND to GND
  - Pin SDA to the SDA pin of the ESP32 (for example, pin 21)
  - Pin SCL to the SCL pin of the ESP32 (for example, pin 22)
- ESP32 Module:
  - VCC to 5V
  - GND to GND
  - RX to the TX of the Amplifier (for example, pin 16)
  - TX to the RX of the Amplifier (for example, pin 17)
- Audio Amplifier:
  - Connect the amplifier’s serial port to the UART of the ESP32 (for example, GND, RX\_PIN, and TX\_PIN)

<img width="1149" height="1335" alt="diagram_ESP_32+UART" src="https://github.com/user-attachments/assets/4f3b0981-9d84-4605-a4e6-08cc4f6a1161" />

**Libraries Used**

The code uses the following libraries for proper operation of the OLED display and UART communication:

- Wire.h: For I2C communication with the OLED display.
- Adafruit\_GFX.h: For handling graphics and text on the OLED display.
- Adafruit\_SSD1306.h: For controlling the OLED display.
- HardwareSerial.h: For UART (Serial) communication with the audio amplifier.

**Functionalities**

- The code allows receiving commands through UART communication and switching between different display modes.
- The display modes include signal source, volume, bass, treble, and a BLANK mode (not yet implemented) to turn off the screen.
- A STANDBY mode has been implemented that shows a "STANDBY" message on the screen and blinks every 2 seconds until a command is received to return to the SOURCE mode.
- The code can process specific commands (SRC, VOL, BAS, TRE, CHN, LED, BTC, VBS, BEP) and update the on-screen information accordingly.

**Usage**

1. Connect the components as indicated in the "Connections" section.
1. Upload the code to the ESP32 using the Arduino IDE.
1. Connect the audio amplifier and make sure it is sending UART commands through the serial port.
1. Observe the information on the screen and control the different display modes by sending the corresponding commands through UART communication.

**Notes**
- Remember that for UART communication between ports, a proper connection between both devices is necessary, as described in the following image.
  
  ![thumbnail_maxresdefault](https://github.com/user-attachments/assets/9f765adf-bf28-4f6a-9c08-a38e642eefdb)
- To customize the names of the audio sources shown on the screen, modify the **mapAudioSource()** function. To add new UART commands or functionalities, extend the **processUARTCommand()** function by adding the command to the filter list and implementing its corresponding logic.

**Credits**

This project was created by Richard Mequert [Zerous] and is inspired by the work of ResinChem Tech and the documentation of the libraries used.

