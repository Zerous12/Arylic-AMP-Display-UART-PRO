/* Arylic-AMP-Display-UART-PRO
   *0.91" Oled Display for Arylic AMP Devices
   *Interface to Arylic Amp PRO v4
   *Version: 4.0 
   * Last Update: 18/10/2025
   * New features: Code optimization, better memory management, priority logic improvements.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

//OLED Configuration (Change if you use a different OLED display size)
#define SCREEN_WIDTH 128      // OLED screen width, in pixels
#define SCREEN_HEIGHT 32     // OLED screen height, in pixels
#define SCREEN_ADDR 0x3C    // 7-bit I2C address

//Serial communication assignment between AMP > ESP > OLED (Change if you use different pins)
#define SDA_PIN 21         // SDA Data pin for SSD1306 display
#define SCL_PIN 22        // SCL Clock pin for SSD1306 display
#define RX_PIN 16        // Serial2 RX to Amp TX
#define TX_PIN 17       // Serial2 TX to Amp RX
#define LED_PIN 12     // Control for backlit VU meter

//Display modes (dispMode)
#define SOURCE 1
#define VOLUME 2
#define CHN 3
#define BAS 4   
#define TRE 5
#define BLANK 6

//GLOBAL VARIABLES FOR PROGRAM SETTINGS
byte dispMode = SOURCE;
byte prevdispMode = 1;
unsigned long dispModeTemp_timer = 0;
unsigned long currentMillis = 0;
unsigned long standbyBlinkInterval = 2000;        // STANDBY blink interval (seconds)
const unsigned long DISP_UPDATE_INTERVAL = 1000; // Here you adjust the update time between display modes.
bool dispModeTempSource = true;

// Variables to measure MUT:1 command time; According to input mode changes
const unsigned long MUTE_DELAY = 700;    // Delay in seconds (you can adjust this value according to your needs)
bool muteMode = false;                  // Variable to store MUTE state
bool receivedMuteOnCommand = false;
unsigned long muteOnTime = 0;

// Variables for non-blocking timers (eliminate delays)
unsigned long paramDisplayStartTime = 0;
const unsigned long PARAM_DISPLAY_DURATION = 500;  // 500ms to show parameters
unsigned long notificationStartTime = 0;
const unsigned long NOTIFICATION_DURATION = 2000;  // 2 seconds for notifications
bool showingParameter = false;
bool showingNotification = false;
String currentNotification = "";
String lastParameterType = "";  // To maintain consecutive parameter flow
unsigned long lastParameterTime = 0;
const unsigned long PARAM_FLOW_TIMEOUT = 1500;  // 1.5 seconds to consider parameter flow

// Variables to handle commands lost by timing
unsigned long lastHighPriorityTime = 0;
const unsigned long HIGH_PRIORITY_GRACE_PERIOD = 100;  // 100ms after critical command

//Variables to store processed data and UART states
bool initialDataReceived = false;    // variable to store STA startup
bool bluetoothConnected = false;    // Variable to store Bluetooth connection state
bool virtualBassEnabled = false;   // Variable to store Virtual Bass state
bool beepEnabled = false;         // Variable to store Beep state
bool standbyMode = false;        // Variable to store STANDBY state
int dispBass = 0;
int dispTreble = 0;
int dispVolume = 0;
String dispChannel = "";
String dispSource = "INIT"; //recover previous source state before Main Loop startup

// Variables for initialization timeout
unsigned long initStartTime = 0;
const unsigned long INIT_TIMEOUT = 3000;  // 3 seconds timeout for initialization
bool timeoutReached = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Display configuration
HardwareSerial uart(2);   // Use of Serial2 hardware interface

/*---------------------------------------------------------------------------------------------------
                        Centralized function to map audio sources
---------------------------------------------------------------------------------------------------*/
String mapAudioSource(const String& rawSource) {
  // Remove semicolon if it exists
  String cleanSource = rawSource;
  cleanSource.replace(";", "");
  
  // Map sources to display names
  if (cleanSource == "BT") {
    return "BT";
  } else if (cleanSource == "LINE-IN") {
    return "LINE";
  } else if (cleanSource == "USB") {
    return "USB";
  } else if (cleanSource == "USBDAC") {
    return "DAC";
  } else if (cleanSource == "NET") {
    return "WIFI";
  } else if (cleanSource == "LINE-IN2") {
    return "LINE2";
  } else if (cleanSource == "OPT") {
    return "OPTL";
  } else if (cleanSource == "COAX") {
    return "COAX";
  } else if (cleanSource == "HDMI") {
    return "HDMI";
  } else {
    return cleanSource; // Use original value if not mapped
  }
}

/*---------------------------------------------------------------------------------------------------
                        Function to process complete STA command
---------------------------------------------------------------------------------------------------*/
void processSTA(const String& staData) {
  // Only extract the source (first value before the first comma)
  // Format: source,mute,volume,treble,bass,net,internet,playing,led,upgrading;
  
  int firstComma = staData.indexOf(',');
  if (firstComma > 0) {
    String source = staData.substring(0, firstComma);
    
    Serial.println("Source detected in STA: " + source);
    
    // Use centralized mapping function
    dispSource = mapAudioSource(source);
    
    Serial.println("Source updated to: " + dispSource);
    showSource(); // Show source on screen
    
  } else {
    Serial.println("Error: Could not extract source from STA command");
  }
}

/*---------------------------------------------------------------------------------------------------
                        Function to process received UART commands
---------------------------------------------------------------------------------------------------*/
void processUARTCommand(const String& commandType, const String& commandValue) {
   // Here you can add logic to process UART commands
  // and update display variables accordingly
  
  // SIMPLIFIED PRIORITY SYSTEM
  bool isHighPriorityCommand = (commandType == "SRC" || commandType == "STA" || 
                                commandType == "MUT" || commandType == "SYS");
  bool isParameterCommand = (commandType == "VOL" || commandType == "BAS" || 
                             commandType == "TRE" || commandType == "CHN");
  
  if (isHighPriorityCommand) {
    // Cancel temporary displays and register critical command time
    showingParameter = false;
    showingNotification = false;
    lastParameterType = "";
    lastHighPriorityTime = millis();
    Serial.println("High priority command: " + commandType);
  }
  
  // For parameter commands, ALWAYS update flow
  if (isParameterCommand) {
    lastParameterTime = millis();
    lastParameterType = commandType;
    Serial.println("Parameter command: " + commandType + ":" + commandValue);
  }
     
  if (commandType == "SRC") {
      // Use centralized mapping function
      dispSource = mapAudioSource(commandValue);
      showSource();
   
  } else if (commandType == "STA") {
    // NEW: Process complete state command STA:source,mute,volume,treble,bass,net,internet,playing,led,upgrading;
    processSTA(commandValue);
    
  } else if (commandType == "VOL") {
    // Map volume value to "MAX" if = 100
    dispVolume = commandValue.toInt();
    if (dispVolume == 100) {
      showNumberParamTwo("VOLUME","MAX");
    // Map volume value between {0-99}
    } else {
      showNumberParam("Volume", dispVolume);
    }
  
  } else if (commandType == "BAS") {
    // Get BASS value from received command
    dispBass = commandValue.toInt();
    // Update dispBass variable with bass value
    showNumberParam("Bass", dispBass);
  
  } else if (commandType == "TRE") {
    // Get TREBLE value from received command
    dispTreble = commandValue.toInt();
    // Update dispTreble variable with treble value
    showNumberParam("Treble", dispTreble);
  
  } else if (commandType == "CHN") {
    if (commandValue == "L;") {
        dispChannel = "LEFT";
    } else if (commandValue == "R;") {
        dispChannel = "RIGHT";
    } else if (commandValue == "S;") {
        dispChannel = "STEREO";
    } else {
        // Unrecognized channel value, show "UNK"
        dispChannel = "UNK";
    }
    // Update dispChannel variable and display it on screen
    showNumberParamTwo("CHN", dispChannel);
    
    //Notify LED action    
  } else if (commandType == "LED") {
    if (commandValue == "1;") {
    showNotification("LED ON");
    // Turn on VU meter LED (External)
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED turned on");
    } else if (commandValue == "0;") {
    // Turn off VU meter LED (External)
    showNotification("LED OFF");
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED turned off");
    }
    //Notify BTC action
  } else if (commandType == "BTC") {
    if (commandValue == "1;") {
      bluetoothConnected = true;
      showNotification("Connected");
    } else if (commandValue == "0;") {
      bluetoothConnected = false;
      showNotification("Disconnect");
    } 
    //Notify VBS action
  } else if (commandType == "VBS") {
    if (commandValue == "1;") {
      virtualBassEnabled = true;
      showNotification("VBS ON");
    } else if (commandValue == "0;") {
      virtualBassEnabled = false;
      showNotification("VBS OFF");
    }
    //Notify BEP action
    //BEP command cannot be changed by IR
  } else if (commandType == "BEP") {
    if (commandValue == "1;") {
      beepEnabled = true;
      showNotification("BEEP ON");
    } else if (commandValue == "0;") {
      beepEnabled = false;
      showNotification("BEEP OFF");
    }
  } //Change priority STANDBY state
    if (commandType == "SYS" && commandValue == "STANDBY;") {
    standbyMode = true;
    showStandby();
  }
  //Handle when system exits STANDBY
  if (commandType == "SYS" && commandValue == "ON;") {
    standbyMode = false;
    dispMode = SOURCE; // Return to SOURCE mode when turning on
    Serial.println("System powered on - Exiting standby");
    showSource();
  }

  //Handle AUD command (Audio settings or similar)
  if (commandType == "AUD") {
    // For now just log that we received it
    Serial.println("AUD command received: " + commandValue);
    // You could add specific logic here if you need to process this command
  }
  
  //Handle LPM command (Loop Mode)
  if (commandType == "LPM") {
    Serial.println("LPM command received: " + commandValue);
    // For now just log - you could add notification if desired
  }
  
  //Handle NAM command (Name/Device name)
  if (commandType == "NAM") {
    Serial.println("NAM command received: " + commandValue);
    // For now just log - you could show the name if desired
  }

  //Notify MUTE action
    if (commandType == "MUT" && commandValue == "1;") {
        muteMode = true;
        receivedMuteOnCommand = true;
        muteOnTime = millis(); // Record the time when MUT:1; was received
    } else if (commandType == "MUT" && commandValue == "0;") {
        // If MUT:0; is received before delay, set muteMode = false and don't show anything on screen
        muteMode = false;
        receivedMuteOnCommand = false;
        // IMPROVEMENT: DO NOT force display immediately to avoid glitches
        // Main loop will handle showing appropriate display
        Serial.println("MUTE deactivated - display will update in main loop");
    }
}

// Function to switch to next display mode
void switchDisplayMode() {
  dispMode++;
  if (dispMode > BLANK) {
    dispMode = SOURCE;
  }
}

// NEW FUNCTION: Request amplifier status
void requestAmplifierStatus() {
  Serial.println("Requesting amplifier status...");
  // Send commands to request current status
  uart.println("STA");      // Request general status
  delay(100);
  uart.println("SRC");      // Request current source
  delay(100);
  uart.println("VOL");      // Request current volume
  delay(100);
}

/*---------------------------------------------------------------------------------------------------
                                    Setup Configurations
---------------------------------------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);
  Serial.println("Starting..");

  // Initialize UART communication
  uart.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  uart.setTimeout(2000);
  
  // Initialize timer for timeout
  initStartTime = millis();
  
  Serial.println("Waiting for amplifier data...");
  Serial.println("Timeout in 3 seconds if no data received");
  
//Process data received in first seconds, getting previous state data from AMP
  while (!initialDataReceived && !timeoutReached) {
    // Check timeout
    if (millis() - initStartTime > INIT_TIMEOUT) {
      timeoutReached = true;
      Serial.println("Timeout reached - No communication with amplifier");
      dispSource = "Lost COM";
      initialDataReceived = true; // Allow program to continue
      break;
    }
    
    if (uart.available() > 0) {
      String receivedData = uart.readStringUntil('\n');
      receivedData.trim();
      
      Serial.println("Initial data received: " + receivedData);

      // Check if received data starts with "STA:" (initial state command)
      if (receivedData.startsWith("STA:")) {
        // Extract commandValue after "STA:"
        String commandValue = receivedData.substring(4);

        // Extract only source (first value before comma)
        int firstComma = commandValue.indexOf(',');
        if (firstComma > 0) {
          String source = commandValue.substring(0, firstComma);
          dispSource = mapAudioSource(source);
        } else {
          // If no comma, use complete value
          dispSource = mapAudioSource(commandValue);
        }
        
        initialDataReceived = true;
        Serial.println("Initial state detected: " + dispSource);
      }
      // IMPROVEMENT: Accept any valid command as initialization
      else if (receivedData.indexOf(':') > 0) {
        // It's a valid command with COMMAND:VALUE format
        String commandType = receivedData.substring(0, receivedData.indexOf(':'));
        String commandValue = receivedData.substring(receivedData.indexOf(':') + 1);
        
        // Process received command
        processUARTCommand(commandType, commandValue);
        
        // If we receive SRC, use as initial source
        if (commandType == "SRC") {
          initialDataReceived = true;
          Serial.println("Source detected in initialization: " + dispSource);
        }
        // If we receive STA, it's perfect - we have complete state
        else if (commandType == "STA") {
          initialDataReceived = true;
          Serial.println("Complete state received in initialization");
        }
        // If we receive any other valid command, assume it's working
        else if (commandType == "VOL" || commandType == "BAS" || commandType == "TRE" || 
                 commandType == "CHN" || commandType == "LED" || commandType == "BTC" ||
                 commandType == "VBS" || commandType == "BEP" || commandType == "MUT" ||
                 commandType == "SYS") {
          if (dispSource == "INIT") {
            dispSource = "UNK"; // Unknown source but amplifier working
          }
          initialDataReceived = true;
          Serial.println("Valid command detected - Amplifier active: " + commandType);
        }
      }
    }
    
    delay(100); // Small pause to not saturate processor
  }
  
// Initialize pins
  pinMode(LED_PIN, OUTPUT);
// Initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR);
  display.clearDisplay();

  // Configure text size and style
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
  display.println("Starting..");
  display.display();
  delay(2000);
  display.clearDisplay();
  // Notification of main loop startup
  Serial.println("Initializing main loop...");
  uart.flush();
}
/*---------------------------------------------------------------------------------------------------
                                       Main Loop
---------------------------------------------------------------------------------------------------*/
void loop() {
    // Read data from UART port
    if (uart.available() > 0) {
        String receivedData = uart.readStringUntil('\n');
        receivedData.trim(); // Remove whitespace and newline characters

      // Print received data to Serial Monitor
        Serial.println("Data received: " + receivedData);

        // Check if received command has valid format: {commandType}:{value}
        int separatorIndex = receivedData.indexOf(':');
        if (separatorIndex > 0 && separatorIndex < receivedData.length() - 1) {
            String commandType = receivedData.substring(0, separatorIndex);
            String commandValue = receivedData.substring(separatorIndex + 1);

            // ADDITIONAL FILTER: Only process known commands
            if (commandType == "SRC" || commandType == "VOL" || commandType == "BAS" || 
                commandType == "TRE" || commandType == "CHN" || commandType == "LED" || 
                commandType == "BTC" || commandType == "VBS" || commandType == "BEP" || 
                commandType == "MUT" || commandType == "SYS" || commandType == "STA" ||
                commandType == "AUD" || commandType == "LPM" || commandType == "NAM") { // Add additional commands
              
              processUARTCommand(commandType, commandValue);
              
              // IMPROVEMENT: If we receive valid data and haven't initialized yet, mark as initialized
              if (!initialDataReceived) {
                initialDataReceived = true;
                Serial.println("Late initialization - Amplifier detected with command: " + commandType);
                if (commandType == "SRC") {
                  // Already processed in processUARTCommand
                } else if (dispSource == "Lost COM") {
                  // Communication recovery after timeout
                  Serial.println("Communication restored - Exiting Lost COM state");
                  dispSource = "UNK"; // Amplifier active, source not determined yet
                } else if (dispSource == "INIT") {
                  dispSource = "UNK"; // Amplifier active, source not determined yet
                }
              }
              // ADDITIONAL IMPROVEMENT: If we were in Lost COM and receive data, restore communication
              else if (dispSource == "Lost COM") {
                Serial.println("Communication restored from Lost COM");
                if (commandType == "SRC" || commandType == "STA") {
                  // Already processed in processUARTCommand with correct source
                } else {
                  dispSource = "UNK"; // Amplifier active, source not determined yet
                  showSource(); // Update screen immediately
                }
              }
            } else {
              Serial.println("Unknown command ignored: " + commandType);
            }
        } else if (receivedData.length() > 10) {
          // Only report as noise if line is long (avoid spam)
          Serial.println("Noise filtered (no valid format)");
        }
    }
    
    // IMPROVEMENT: No longer block loop if no initial data received
    // Display will work even without initial data
    
    // Update display if display mode has changed
    if (dispMode != prevdispMode) {
    switchDisplayMode();
    prevdispMode = dispMode;
    }

  currentMillis = millis();

  // NEW LOGIC: Non-blocking timer management (high priority)
  // Check if notification timer has expired
  if (showingNotification && (currentMillis - notificationStartTime >= NOTIFICATION_DURATION)) {
    showingNotification = false;
    // Return to appropriate display after notification
    if (muteMode) {
      showMute();
    } else {
      showSource();
    }
  }
  
  // Check if parameter timer has expired
  if (showingParameter && (currentMillis - paramDisplayStartTime >= PARAM_DISPLAY_DURATION)) {
    // NEW LOGIC: Only return to SOURCE if no active parameter flow
    bool parameterFlowActive = (lastParameterType != "" && 
                                (currentMillis - lastParameterTime < PARAM_FLOW_TIMEOUT));
    
    if (!parameterFlowActive) {
      showingParameter = false;
      lastParameterType = "";
      // Return to appropriate display after parameter
      if (muteMode) {
        showMute();
      } else {
        showSource();
      }
    } else {
      // Extend timer if active parameter flow
      paramDisplayStartTime = currentMillis;
    }
  }

  // Check if we're in lost communication state
  if (dispSource == "Lost COM") {
    showLostCommunication(); // Show "Lost COM" blinking
    // Don't process other modes until communication is restored
  } else if (standbyMode) {
      showStandby();
    if (uart.available() > 0) {
      String receivedData = uart.readStringUntil('\n');
      receivedData.trim();
      if (receivedData.startsWith("STA:")) {
        standbyMode = false;
        dispMode = SOURCE; // Return to SOURCE mode when receiving command starting with "STA:"
      }
    }
  } else if (muteMode) {
        // Show MUTE ON message if MUTE is active and delay has passed
        if (receivedMuteOnCommand && (millis() - muteOnTime) >= MUTE_DELAY) {
            showMute();
            receivedMuteOnCommand = false; // Reset indicator after showing message
        }
  } else {
      // CRITICAL IMPROVEMENT: Only change automatic display if NO parameter activity
      // and NO notifications
      bool noParameterActivity = !showingParameter && !showingNotification;
      bool timeForModeChange = (dispMode != prevdispMode);
      bool timeForUpdate = (currentMillis > (dispModeTemp_timer + DISP_UPDATE_INTERVAL));
      
      // NEW RULE: If there's recent parameter activity, DO NOT interfere
      bool recentParameterActivity = (lastParameterType != "" && 
                                      (currentMillis - lastParameterTime < PARAM_FLOW_TIMEOUT));
      
      if (noParameterActivity && !recentParameterActivity && (timeForModeChange || timeForUpdate)) {
        // Only now it's safe to change to automatic display  
        switch (dispMode) {
          case SOURCE:
            //ShowNameMode
            showSource();
            break;
          case VOLUME:
            //showVolume();
            showNumberParam("Volume", dispVolume);
            break;
            case CHN:
            //showChannel();
            showNumberParamTwo("Channel", dispChannel);
            break;
          case BAS:
            //showBass();
            showNumberParam("Bass", dispBass);
            break;
          case TRE:
            //showTreble();
            showNumberParam("Treble", dispTreble);
            break;
          default:
            display.clearDisplay();
            display.display(); 
            break;
          }
          prevdispMode = dispMode;
          dispModeTemp_timer = currentMillis;
          dispModeTempSource = false;
       }
  }
}
/*---------------------------------------------------------------------------------------------------
                             Functions for Display and Format on OLED
---------------------------------------------------------------------------------------------------*/
void showNumberParamTwo(String parmName, String parmValue) {
    // NEW SIMPLIFIED LOGIC: ALWAYS show parameters when received
    // Only check if same type for smooth update
    bool sameParameterType = (showingParameter && lastParameterType == parmName.substring(0,3));
    
    if (!sameParameterType) {
      display.clearDisplay();
      display.setTextSize(2); 
      display.setTextColor(WHITE);
      display.invertDisplay(false);
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
    } else {
      // Only clear value area for smooth update
      display.fillRect(70, 0, 58, 32, BLACK); // Clear value area
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
    }
    
    display.print(parmName + ":" + parmValue);
    display.display();
    dispModeTemp_timer = millis();
    
    // Start non-blocking timer for parameters
    paramDisplayStartTime = millis();
    showingParameter = true;
    dispModeTempSource = false; // Ensure we're not in SOURCE mode
    
    Serial.println("Showing parameter: " + parmName + ":" + parmValue);
}

void showNumberParam(String parmName, int parmValue) {
    // NEW SIMPLIFIED LOGIC: ALWAYS show parameters when received
    // Only check if same type for smooth update
    bool sameParameterType = (showingParameter && lastParameterType == parmName.substring(0,3));
    
    if (!sameParameterType) {
      display.clearDisplay();
      display.setTextSize(2); 
      display.setTextColor(WHITE);
      display.invertDisplay(false);
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
    } else {
      // Only clear value area for smooth update
      display.fillRect(70, 0, 58, 32, BLACK); // Clear value area
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
    }
    
    display.print(parmName + ":" + parmValue);
    display.display();
    dispModeTemp_timer = millis();
    
    // Start non-blocking timer for parameters
    paramDisplayStartTime = millis();
    showingParameter = true;
    dispModeTempSource = false; // Ensure we're not in SOURCE mode
    
    Serial.println("Showing parameter: " + parmName + ":" + String(parmValue));
}

void showSource() {
    // NEW PROTECTION: Avoid overwriting very recent parameters
    unsigned long timeSinceLastParam = millis() - lastParameterTime;
    if (showingParameter && timeSinceLastParam < 50) {
      // If we just showed a parameter less than 50ms ago, don't interfere
      Serial.println("Avoiding glitch - very recent parameter, postponing showSource");
      return;
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
    display.print("Mode:" + String(dispSource));
    display.display();
    dispModeTemp_timer = millis();
    dispModeTempSource = true;
    Serial.println("Showing source: " + dispSource);
}

void showNotification(const String& message) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
  display.println(message);
  display.display();
  
  // Start non-blocking timer for notifications
  notificationStartTime = millis();
  showingNotification = true;
  currentNotification = message;
  // NO delay() - handled in main loop
}

// New method to show STANDBY message
void showStandby() {
  static unsigned long prevBlinkMillis = 0;
  static bool displayStandby = true;

  unsigned long currentMillis = millis();

  if (currentMillis - prevBlinkMillis >= standbyBlinkInterval) {
    displayStandby = !displayStandby;
    prevBlinkMillis = currentMillis;
  }

  if (displayStandby) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34));
    display.print("  STANDBY ");
    display.display();
  } else {
    display.clearDisplay();
    display.display();
  }
}

// New method to show Lost COM message (no communication)
void showLostCommunication() {
  static unsigned long prevBlinkMillis = 0;
  static bool displayLostCom = true;

  unsigned long currentMillis = millis();

  if (currentMillis - prevBlinkMillis >= 1500) { // Faster blink than STANDBY
    displayLostCom = !displayLostCom;
    prevBlinkMillis = currentMillis;
  }

  if (displayLostCom) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34));
    display.print(" Lost COM ");
    display.display();
  } else {
    display.clearDisplay();
    display.display();
  }
}

void showMute() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34));
    display.print(" MUTE ON ");
    display.display();
}
