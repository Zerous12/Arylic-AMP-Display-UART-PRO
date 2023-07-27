#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

//Configuracion OLED (Cambialo si usas otro tamaño de display OLED)
#define SCREEN_WIDTH 128      // Ancho de pantalla OLED, en píxeles
#define SCREEN_HEIGHT 32     // Altura de la pantalla OLED, en píxeles
#define SCREEN_ADDR 0x3C    //Dirección I2C de 7 bits

//Asignacion de comunicacion Serie entre AMP > ESP > OLED (Cambialo si usas diferentes pines)
#define SDA_PIN 21         // Pin Data SDA para pantalla SSD1306
#define SCL_PIN 22        // Pin Clock SCL para pantalla SSD1306
#define RX_PIN 16        // Serial2 RX a Amp TX
#define TX_PIN 17       // Serial2 TX a Amp RX
#define LED_PIN 12     // Control para vúmetro retroiluminado

//Modos de display
#define SOURCE 1
#define VOLUME 2
#define CHN 3
#define BAS 4   
#define TRE 5
#define BLANK 6

// Variables globales para los modos y tiempos de visualización
byte dispMode = SOURCE;
byte prevdispMode = 1;
unsigned long dispModeTemp_timer = 0;
unsigned long currentMillis = 0;
unsigned long standbyBlinkInterval = 2000;        // Intervalo de parpadeo de STANDBY (segundos)
const unsigned long DISP_UPDATE_INTERVAL = 1000; // Aqui ajustas el tiempo de actualizacion entre modos de visualizacion.
bool dispModeTempSource = true;

bool initialDataReceived = false;    // variable para almacenar inicio de STA
bool bluetoothConnected = false;    // Variable para almacenar el estado de conexión Bluetooth
bool virtualBassEnabled = false;   // Variable para almacenar el estado del Virtual Bass
bool beepEnabled = false;         // Variable para almacenar el estado del Beep
bool standbyMode = false;        // Variable para almacenar el estado de STANDBY
//bool dispModeTempMute = true;

//Variables para almacenar datos procesados UART
//int dispBalance = 0;
int dispBass = 0;
int dispTreble = 0;
int dispVolume = 0;
String dispChannel = "";
String dispSource = "x_x!"; //Pre-carga de commandValue antes de inicio


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Configuración del display
HardwareSerial uart(2);   // Uso de la interfaz de hardware Serial2

// Función para procesar los comandos UART recibidos
void processUARTCommand(const String& commandType, const String& commandValue) {
   // Aquí puedes agregar la lógica para procesar los comandos UART
  // y actualizar las variables de visualización en consecuencia
     
  if (commandType == "SRC") {
      // Mapear los valores recibidos en commandValue para re-designar valores de salidas
      if (commandValue == "BT;") {
          dispSource = "BT";
      } else if (commandValue == "LINE-IN;") {
          dispSource = "LINE";
      } else if (commandValue == "USB;") {
          dispSource = "USB";
      } else if (commandValue == "NET;") {
          dispSource = "WIFI";
      }
    showSource();
   
  } else if (commandType == "VOL") {
    // Mapear el valor del volumen a "MAX" si es = 100
    dispVolume = commandValue.toInt();
    if (dispVolume == 100) {
      showVolume("VOLUME MAX");
    // Mapear el valor del volumen entre {0-99}
    } else {
      showNumberParam("Volume", dispVolume);
    }
  
  } else if (commandType == "BAS") {
    // Obtener el valor del BASS del comando recibido
    dispBass = commandValue.toInt();
    // Actualizar la variable dispBass con el valor del bass
    showNumberParam("Bass", dispBass);
  
  } else if (commandType == "TRE") {
    // Obtener el valor del TREBLE del comando recibido
    dispTreble = commandValue.toInt();
    // Actualizar la variable dispTreble con el valor del treble
    showNumberParam("Treble", dispTreble);
  
  } else if (commandType == "CHN") {
    // Obtener el valor del CHN del comando recibido
    dispChannel = commandValue;
    // Actualizar la variable dispTreble con el valor del Channel
    showNumberParamTwo("Channel", dispChannel);
    
    //Notificar accion de LED    
  } else if (commandType == "LED") {
    if (commandValue == "1;") {
    showNotification("LED ON");
    // Encender el LED del Vumeter (Externo)
    digitalWrite(LED_PIN, HIGH);
    Serial.println("encendido LED");
    } else if (commandValue == "0;") {
    // Apagar el LED del Vumeter (Externo)
    showNotification("LED OFF");
    digitalWrite(LED_PIN, LOW);
    Serial.println("apagado LED");
    }
    //Notificar accion de BTC
  } else if (commandType == "BTC") {
    if (commandValue == "1;") {
      bluetoothConnected = true;
      showNotification("Connected");
    } else if (commandValue == "0;") {
      bluetoothConnected = false;
      showNotification("Disconnect");
    } 
    //Notificar accion de VBS
  } else if (commandType == "VBS") {
    if (commandValue == "1;") {
      virtualBassEnabled = true;
      showNotification("VBS ON");
    } else if (commandValue == "0;") {
      virtualBassEnabled = false;
      showNotification("VBS OFF");
    }
    //Notificar accion de BEP
    //Comando BEP no puede ser cambiado por IR
  } else if (commandType == "BEP") {
    if (commandValue == "1;") {
      beepEnabled = true;
      showNotification("BEEP ON");
    } else if (commandValue == "0;") {
      beepEnabled = false;
      showNotification("BEEP OFF");
    }
  } //Cambiar Estado pioritario de STANDBY
    if (commandType == "SYS" && commandValue == "STANDBY;") {
    standbyMode = true;
    showStandby();
  }
}
// Función para cambiar al siguiente modo de visualización
void switchDisplayMode() {
  dispMode++;
  if (dispMode > BLANK) {
    dispMode = SOURCE;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Arrancando..");

  // Inicializar la comunicación UART
  uart.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  uart.setTimeout(2000);
//Procesar datos recibidos en los primeros segundos, obteniendos datos de estado anterior en AMP
  while (!initialDataReceived) {
    if (uart.available() > 0) {
      String receivedData = uart.readStringUntil('\n');
      receivedData.trim();

        // Verificar si los datos recibidos comienzan con "STA:"
      if (receivedData.startsWith("STA:")) {
        // Extraer el commandValue después de "STA:"
        String commandValue = receivedData.substring(4);

        // Verificar el tipo de comando recibido y procesar los datos adecuadamente
        if (commandValue.startsWith("BT")) {
          // Si el comando inicia con "BT", el dispositivo está conectado a Bluetooth
          dispSource = "BT";
        } else if (commandValue.startsWith("LINE-IN")) {
          // Si el comando inicia con "LINE", el dispositivo está conectado a LINE-IN
          dispSource = "LINE";
        } else if (commandValue.startsWith("USB")) {
          // Si el comando inicia con "USB", el dispositivo está conectado a USB
          dispSource = "USB";
        } else if (commandValue.startsWith("NET")) {
          // Si el comando inicia con "NET", el dispositivo está conectado a WIFI/Internet
          dispSource = "WIFI";
        } 
        initialDataReceived = true; // Marcar que los datos iniciales se han recibido
      }
    }
  }
  
// Inicializar pines
  pinMode(LED_PIN, OUTPUT);
// Inicializar el display OLED
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR);
  display.clearDisplay();

  // Configurar el tamaño y estilo del texto
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
  display.println("Starting..");
  display.display();
  delay(2000);
  display.clearDisplay();
  // Notificacion de inicio de bucle principal
  Serial.println("Inicializando el buble principal...");
  uart.flush();
}

void loop() {
    // Leer los datos del puerto UART
    if (uart.available() > 0) {
        String receivedData = uart.readStringUntil('\n');
        receivedData.trim(); // Elimina los espacios en blanco y caracteres de nueva línea

      // Imprimir los datos recibidos en el Monitor Serie
        Serial.println("Datos recibidos: " + receivedData);

        // Verifica si el comando recibido tiene un formato válido: {commandType}:{value}
        int separatorIndex = receivedData.indexOf(':');
        if (separatorIndex > 0 && separatorIndex < receivedData.length() - 1) {
            String commandType = receivedData.substring(0, separatorIndex);
            String commandValue = receivedData.substring(separatorIndex + 1);

            processUARTCommand(commandType, commandValue);
        }
      }
    if (!initialDataReceived) {
    // Esperar hasta que los datos iniciales se reciban antes de continuar
    return;
  }
    // Actualizar el display si el modo de visualización ha cambiado
    if (dispMode != prevdispMode) {
    switchDisplayMode();
    prevdispMode = dispMode;
    }

  currentMillis = millis();

  if (standbyMode) {
      showStandby();
    if (uart.available() > 0) {
      String receivedData = uart.readStringUntil('\n');
      receivedData.trim();
      if (receivedData.startsWith("STA:")) {
        standbyMode = false;
        dispMode = SOURCE; // Volver al modo SOURCE al recibir un comando que comience con "STA:"
      }
    }
  } else {
      // Solo restablecer las variables después de un cambio de modo
    if (dispMode != prevdispMode || currentMillis > (dispModeTemp_timer + DISP_UPDATE_INTERVAL)) {
    // Agregar lógica para volver al modo SOURCE después de STANDBY
      
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


// Funciones de SHOW

void showVolume(const String& volumeValue) {
    if (!dispModeTempSource) {
      display.clearDisplay();
      display.setTextSize(2); 
      display.setTextColor(WHITE);
      display.invertDisplay(false);
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34));
      display.print(volumeValue);
      display.display();
      dispModeTemp_timer = millis();
   }
}

void showNumberParamTwo(String parmName, String parmValue) {
    if (!dispModeTempSource) {
      display.clearDisplay();
      display.setTextSize(2); 
      display.setTextColor(WHITE);
      display.invertDisplay(false);
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
      display.print(parmName + ": " + parmValue);
      display.display();
      dispModeTemp_timer = millis();
    }
}

void showNumberParam(String parmName, int parmValue) {
    if (!dispModeTempSource) {
      display.clearDisplay();
      display.setTextSize(2); 
      display.setTextColor(WHITE);
      display.invertDisplay(false);
      display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
      display.print(parmName + ": " + parmValue);
      display.display();
      dispModeTemp_timer = millis();
    }
}

void showSource() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
    display.print("Mode:" + String(dispSource));
    display.display();
    dispModeTemp_timer = millis();
    dispModeTempSource = true;
}

void showNotification(const String& message) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor((SCREEN_WIDTH) / 1, (SCREEN_HEIGHT - 34) );
  display.println(message);
  display.display();
  delay(2000);
  showSource(); // Mostrar nuevamente el modo de visualización actual después de 2 segundos
}

// Nuevo método para mostrar el mensaje de STANDBY
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