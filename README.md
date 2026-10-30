# SnakeESP

Snake jugable sobre una placa ESP8266, con tres salidas sincronizadas en tiempo real: una matriz de LEDs física, una pantalla OLED y una página web que el propio ESP8266 sirve desde su propio punto de acceso WiFi (sin necesitar internet).

## Componentes

- ESP8266 (NodeMCU)
- Matriz de LEDs 8x8 con driver MAX7219
- Pantalla OLED 0.96" I2C (SSD1306)
- 2 LEDs (indicador de actividad)
- Zumbador pasivo
- Resistencias, protoboard, cables

## Cómo funciona

El ESP8266 crea su propia red WiFi (`SnakeESP`, contraseña `12345678`) y sirve una página de control en `192.168.4.1`. Desde ahí se controla la dirección de la serpiente con botones o las flechas del teclado. Cada movimiento se refleja a la vez en:

- la matriz de LEDs física,
- la pantalla OLED (puntuación, longitud y estado de la partida),
- la propia página web (que se actualiza sola cada 200ms).

Un zumbador pasivo marca cada vez que la serpiente come, y reproduce una pequeña melodía original al perder.

## Librerías necesarias

Instalar desde el gestor de bibliotecas de Arduino IDE:

- `Adafruit_SSD1306`
- `Adafruit_GFX`
- `LedControl`
- Core `ESP8266` (board manager)

## Pines

| Componente | Pin |
|---|---|
| OLED SDA | D2 |
| OLED SCL | D1 |
| Matriz DIN | D7 |
| Matriz CLK | D5 |
| Matriz CS | D4 |
| LED rojo | D0 |
| LED azul | D3 |
| Zumbador | D6 |

## Nota

La contraseña del punto de acceso está escrita directamente en el código (`WiFi.softAP(...)`). Si reutilizas este proyecto, cámbiala por la tuya antes de subirlo a un repositorio público.
