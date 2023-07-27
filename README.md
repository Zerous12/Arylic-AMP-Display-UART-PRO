# Arylic-AMP-Display-UART-PRO
0.91" Oled Display for Arylic AMP Devices

**Control de Visualización con ESP32 y Display OLED 0.91”**

Este proyecto utiliza un ESP32S junto con un Display OLED 0.91” para mostrar información sobre el estado de un amplificador de audio. El código permite recibir comandos a través de una comunicación UART (Serial) para cambiar entre diferentes modos de visualización, como el origen de la señal, el volumen, el bajo, el agudo y más. Además, se ha implementado una función de modo STANDBY que muestra un mensaje de "STANDBY" en pantalla y parpadea cada 2 segundos hasta que se recibe un comando para volver al modo SOURCE.

**Componentes utilizados**

- ESP32 (se ha utilizado un NodeMCU ESP32S en este ejemplo)
- Display OLED (Se ha utilizado un display I2C OLED de 128x32 píxeles)
- Amplificador de Audio con capacidad de enviar comandos UART a través de su puerto serial (up2Stream PRO\_V4)

**Conexiones**

Conectar los componentes siguiendo estas conexiones:

- Display OLED:
  - Pin VCC a 5V
  - Pin GND a GND
  - Pin SDA a pin SDA del ESP32 (por ejemplo, pin 21)
  - Pin SCL a pin SCL del ESP32 (por ejemplo, pin 22)
- Módulo ESP32:
  - VCC a 5V
  - GND a GND
  - RX al TX del Amplificador (por ejemplo, pin 16)
  - TX al RX del Amplificador (por ejemplo, pin 17)
- Amplificador de Audio:
  - Conectar el puerto serial del amplificador a la UART del ESP32 (por ejemplo, GND, RX\_PIN y TX\_PIN)

**Librerías utilizadas**

El código utiliza las siguientes librerías para el correcto funcionamiento del display OLED y la comunicación UART:

- Wire.h: Para la comunicación I2C con el display OLED.
- Adafruit\_GFX.h: Para el manejo de gráficos y texto en el display OLED.
- Adafruit\_SSD1306.h: Para el control del display OLED.
- HardwareSerial.h: Para la comunicación UART (Serial) con el amplificador de audio.

**Funcionalidades**

- El código permite recibir comandos a través de la comunicación UART y cambiar entre diferentes modos de visualización.
- Los modos de visualización incluyen el origen de la señal, el volumen, el bajo, el agudo y un modo BLANK (aun no implementado) para apagar la pantalla.
- Se ha implementado un modo STANDBY que muestra un mensaje de "STANDBY" en pantalla y parpadea cada 2 segundos hasta que se recibe un comando para volver al modo SOURCE.
- El código es capaz de procesar comandos específicos (SRC, VOL, BAS, TRE, CHN, LED, BTC, VBS, BEP) y actualizar la información en pantalla en consecuencia.

**Uso**

1. Conecta los componentes como se ha indicado en la sección "Conexiones".
1. Sube el código al ESP32 utilizando el IDE de Arduino.
1. Conecta el amplificador de audio y asegúrate de que esté enviando comandos UART a través del puerto serial.
1. Observa la información en pantalla y controla los diferentes modos de visualización enviando los comandos correspondientes a través de la comunicación UART.

**Notas**

- Si deseas agregar más comandos o funcionalidades, puedes modificar la función **processUARTCommand** para manejar los nuevos comandos y actualizar la información en pantalla según sea necesario.

**Créditos**

Este proyecto ha sido creado por Richard Mequert [Zerous] y está inspirado en el trabajo de ResinChem Tech y la documentación de las librerías utilizadas.
