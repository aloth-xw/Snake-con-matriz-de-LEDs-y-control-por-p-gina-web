#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LedControl.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define ANCHO 128
#define ALTO 64
#define TAM 8
#define ledRojo D0
#define ledAzul D3
#define buzzer D6

bool estadoLed = false;
unsigned long ultimoCambioLed = 0;

Adafruit_SSD1306 display(ANCHO, ALTO, &Wire, -1);
LedControl matriz = LedControl(D7, D5, D4, 1); // DIN, CLK, CS
ESP8266WebServer servidor(80);

int serpienteX[64], serpienteY[64];
int longitudSerpiente = 3;
int dirX = 1, dirY = 0;
int comidaX, comidaY;
int puntuacion = 0;
bool juegoTerminado = false;
bool comioFruta = false;
bool perdioJuego = false;
unsigned long ultimoMovimiento = 0;
int velocidadMs = 400;

void generarComida() {
  bool valida = false;
  while (!valida) {
    comidaX = random(0, TAM);
    comidaY = random(0, TAM);
    valida = true;
    for (int i = 0; i < longitudSerpiente; i++) {
      if (serpienteX[i] == comidaX && serpienteY[i] == comidaY) valida = false;
    }
  }
}

void moverSerpiente() {
  if (juegoTerminado) return;

  int nuevaX = serpienteX[0] + dirX;
  int nuevaY = serpienteY[0] + dirY;

  if (nuevaX < 0 || nuevaX >= TAM || nuevaY < 0 || nuevaY >= TAM) {
    juegoTerminado = true;
    perdioJuego = true;
    return;
  }
  for (int i = 0; i < longitudSerpiente; i++) {
    if (serpienteX[i] == nuevaX && serpienteY[i] == nuevaY) {
      juegoTerminado = true;
      perdioJuego = true;
      return;
    }
  }

  for (int i = longitudSerpiente; i > 0; i--) {
    serpienteX[i] = serpienteX[i - 1];
    serpienteY[i] = serpienteY[i - 1];
  }
  serpienteX[0] = nuevaX;
  serpienteY[0] = nuevaY;

  if (nuevaX == comidaX && nuevaY == comidaY) {
    longitudSerpiente++;
    puntuacion++;
    comioFruta = true;
    generarComida();
  }
}

void reiniciarJuego() {
  longitudSerpiente = 3;
  serpienteX[0] = 4; serpienteY[0] = 4;
  serpienteX[1] = 3; serpienteY[1] = 4;
  serpienteX[2] = 2; serpienteY[2] = 4;
  dirX = 1; dirY = 0;
  puntuacion = 0;
  juegoTerminado = false;
  generarComida();
}

void dibujarSerpienteMatriz() {
  matriz.clearDisplay(0);
  for (int i = 0; i < longitudSerpiente; i++) {
    matriz.setLed(0, serpienteY[i], serpienteX[i], true);
  }
  matriz.setLed(0, comidaY, comidaX, true);
}

void tocarMelodiaDerrota() {
  int notas[] = {
    440, 440, 440, 349, 523,
    440, 349, 523, 440,
    659, 659, 659, 698, 523,
    440, 349, 523, 440
  };

  int duraciones[] = {
    500, 500, 250, 250, 250,
    500, 250, 250, 1000,
    500, 500, 500, 250, 250,
    500, 250, 250, 1000
  };

  for (int i = 0; i < 18; i++) {
    tone(buzzer, notas[i], duraciones[i] * 0.9);
    delay(duraciones[i]);
    noTone(buzzer);
  }
}

void manejarDireccion() {
  String d = servidor.arg("d");
  if (d == "up" && dirY != 1) { dirX = 0; dirY = -1; }
  else if (d == "down" && dirY != -1) { dirX = 0; dirY = 1; }
  else if (d == "left" && dirX != 1) { dirX = -1; dirY = 0; }
  else if (d == "right" && dirX != -1) { dirX = 1; dirY = 0; }
  servidor.send(200, "text/plain", "ok");
}

void manejarEstado() {
  String json = "{\"puntuacion\":" + String(puntuacion) + ",\"fin\":" + (juegoTerminado ? "true" : "false") + ",\"serpiente\":[";
  for (int i = 0; i < longitudSerpiente; i++) {
    json += "[" + String(serpienteX[i]) + "," + String(serpienteY[i]) + "]";
    if (i < longitudSerpiente - 1) json += ",";
  }
  json += "],\"comida\":[" + String(comidaX) + "," + String(comidaY) + "]}";
  servidor.send(200, "application/json", json);
}


void manejarRaiz() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>SnakeESP Arcade</title>

<style>
:root {
  --fondo: #090d18;
  --panel: #131c2b;
  --verde: #42f5a1;
  --texto: #f1f5f9;
  --muted: #94a3b8;
  --rojo: #ff5470;
}

* {
  box-sizing: border-box;
  -webkit-tap-highlight-color: transparent;
}

body {
  margin: 0;
  min-height: 100vh;
  padding: 22px 14px 32px;
  background: radial-gradient(ellipse at top, #172a37 0%, var(--fondo) 65%);
  color: var(--texto);
  font-family: system-ui, -apple-system, sans-serif;
  text-align: center;
}

.contenedor {
  width: 100%;
  max-width: 390px;
  margin: auto;
}

.cabecera {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 22px;
}

.marca {
  display: flex;
  align-items: center;
  gap: 10px;
  text-align: left;
}

.logo {
  width: 45px;
  height: 45px;
  display: grid;
  place-items: center;
  background: #17382f;
  border: 1px solid #286b51;
  border-radius: 13px;
  color: var(--verde);
  font-size: 26px;
}

h1 {
  margin: 0;
  font-size: 19px;
  letter-spacing: 2px;
}

.subtitulo {
  color: var(--muted);
  font-size: 10px;
  letter-spacing: 2px;
  margin-top: 3px;
}

.estado {
  font-size: 10px;
  color: var(--verde);
  background: #143127;
  border-radius: 20px;
  padding: 7px 10px;
}

.marcadores {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-bottom: 18px;
}

.tarjeta {
  background: var(--panel);
  border: 1px solid #253246;
  border-radius: 15px;
  padding: 13px;
  text-align: left;
}

.etiqueta {
  color: var(--muted);
  font-size: 10px;
  letter-spacing: 1.5px;
}

.numero {
  font-size: 30px;
  font-weight: 800;
  line-height: 1.35;
}

#puntos {
  color: var(--verde);
}

.marco {
  padding: 14px;
  background: #0c1420;
  border: 1px solid #263d3b;
  border-radius: 18px;
  box-shadow: 0 10px 35px #0005;
}

.titulo-tablero {
  font-size: 11px;
  color: var(--verde);
  letter-spacing: 3px;
  margin-bottom: 12px;
}

#tablero {
  width: 100%;
  max-width: 300px;
  aspect-ratio: 1;
  display: grid;
  grid-template-columns: repeat(8, minmax(0, 1fr));
  grid-template-rows: repeat(8, minmax(0, 1fr));
  gap: 3px;
  margin: auto;
  background: #080e17;
  border-radius: 8px;
  overflow: hidden;
}

.celda {
  min-width: 0;
  min-height: 0;
  background: #17232d;
  border-radius: 4px;
}

.cuerpo {
  background: #32d583;
  box-shadow: inset 0 0 0 1px #a1ffcf;
}

.cabeza {
  background: #b8ffda;
  box-shadow: 0 0 9px #42f5a1;
}

.comida {
  background: var(--rojo);
  border-radius: 50%;
  box-shadow: 0 0 9px #ff547077;
  transform: scale(.75);
}

#mensaje {
  color: var(--muted);
  font-size: 12px;
  min-height: 18px;
  margin-top: 12px;
}

.controles {
  display: grid;
  grid-template-columns: repeat(3, 66px);
  grid-template-rows: repeat(2, 58px);
  gap: 8px;
  justify-content: center;
  margin: 22px auto 15px;
}

button {
  border: 1px solid #2b594b;
  border-radius: 15px;
  background: #163329;
  color: var(--verde);
  font-size: 27px;
  font-weight: 700;
  cursor: pointer;
  touch-action: manipulation;
  box-shadow: 0 4px 0 #0b211b;
}

button:active {
  transform: translateY(3px);
  box-shadow: none;
  background: #245440;
}

button[data-dir="up"] {
  grid-column: 2;
  grid-row: 1;
}

button[data-dir="left"] {
  grid-column: 1;
  grid-row: 2;
}

button[data-dir="down"] {
  grid-column: 2;
  grid-row: 2;
}

button[data-dir="right"] {
  grid-column: 3;
  grid-row: 2;
}

.ayuda {
  color: #64748b;
  font-size: 11px;
  line-height: 1.8;
}

#reiniciar {
  font-size: 13px;
  padding: 11px 18px;
  margin: 8px;
  border-color: #344155;
  background: #202c3c;
  color: var(--texto);
  box-shadow: none;
  border-radius: 10px;
}

@media (max-width: 350px) {
  body { padding: 14px 9px; }
  .controles {
    grid-template-columns: repeat(3, 58px);
    grid-template-rows: repeat(2, 54px);
  }
}
</style>
</head>

<body>
<main class="contenedor">
  <header class="cabecera">
    <div class="marca">
      <div class="logo">&#10038;</div>
      <div>
        <h1>SNAKEESP</h1>
        <div class="subtitulo">ARCADE EDITION</div>
      </div>
    </div>
    <div class="estado">● ONLINE</div>
  </header>

  <section class="marcadores">
    <div class="tarjeta">
      <div class="etiqueta">PUNTUACIÓN</div>
      <div class="numero" id="puntos">000</div>
    </div>
    <div class="tarjeta">
      <div class="etiqueta">LONGITUD</div>
      <div class="numero" id="longitud">03</div>
    </div>
  </section>

  <section class="marco">
    <div class="titulo-tablero">TABLERO DE JUEGO</div>
    <div id="tablero"></div>
    <div id="mensaje">Preparado para jugar</div>
  </section>

  <div class="controles">
    <button data-dir="up" aria-label="Arriba">&#8593;</button>
    <button data-dir="left" aria-label="Izquierda">&#8592;</button>
    <button data-dir="down" aria-label="Abajo">&#8595;</button>
    <button data-dir="right" aria-label="Derecha">&#8594;</button>
  </div>

  <button id="reiniciar" onclick="location.reload()">
    VOLVER A CONECTAR
  </button>

  <div class="ayuda">
    Usa los botones o las flechas del teclado.<br>
    Come la fruta y evita chocar con los bordes.
  </div>
</main>

<script>
const tablero = document.getElementById('tablero');
const puntosEl = document.getElementById('puntos');
const longitudEl = document.getElementById('longitud');
const mensaje = document.getElementById('mensaje');

const celdas = [];

for (let y = 0; y < 8; y++) {
  for (let x = 0; x < 8; x++) {
    const c = document.createElement('div');
    c.className = 'celda';
    tablero.appendChild(c);
    celdas.push(c);
  }
}

function mandar(d) {
  fetch('/dir?d=' + d).catch(() => {
    mensaje.textContent = 'Error de conexión con el ESP8266';
  });
}

document.querySelectorAll('[data-dir]').forEach(b => {
  b.addEventListener('click', () => mandar(b.dataset.dir));
});

document.addEventListener('keydown', e => {
  const teclas = {
    ArrowUp: 'up',
    ArrowDown: 'down',
    ArrowLeft: 'left',
    ArrowRight: 'right',
    w: 'up', s: 'down', a: 'left', d: 'right'
  };

  if (teclas[e.key]) {
    e.preventDefault();
    mandar(teclas[e.key]);
  }
});

let consultando = false;

async function actualizar() {
  if (consultando) return;
  consultando = true;

  try {
    const r = await fetch('/estado', {cache: 'no-store'});
    if (!r.ok) throw new Error('HTTP');

    const e = await r.json();

    puntosEl.textContent = String(e.puntuacion).padStart(3, '0');
    longitudEl.textContent = String(e.serpiente.length).padStart(2, '0');

    for (const c of celdas) c.className = 'celda';

    e.serpiente.forEach((s, i) => {
      const c = celdas[s[1] * 8 + s[0]];
      if (c) c.classList.add(i === 0 ? 'cabeza' : 'cuerpo');
    });

    const comida = celdas[e.comida[1] * 8 + e.comida[0]];
    if (comida) comida.className = 'celda comida';

    if (e.fin) {
      mensaje.textContent = '¡Partida terminada!';
    } else {
      mensaje.textContent = 'Come la fruta para sumar puntos';
    }
  } catch (err) {
    mensaje.textContent = 'Reconectando con SnakeESP...';
  } finally {
    consultando = false;
  }
}

actualizar();
setInterval(actualizar, 200);
</script>
</body>
</html>
)HTML";

  servidor.send(200, "text/html; charset=utf-8", html);
}


void setup() {
  pinMode(ledRojo, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  pinMode(buzzer, OUTPUT);
  randomSeed(analogRead(A0));
  reiniciarJuego();

  Wire.begin(D2, D1);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);

  matriz.shutdown(0, false);
  matriz.setIntensity(0, 8);
  matriz.clearDisplay(0);

  WiFi.softAP("SnakeESP", "12345678");
  servidor.on("/", manejarRaiz);
  servidor.on("/dir", manejarDireccion);
  servidor.on("/estado", manejarEstado);
  servidor.begin();
}

void loop() {
  if (millis() - ultimoMovimiento > velocidadMs) {
    moverSerpiente();
    if (comioFruta) {
      digitalWrite(buzzer, HIGH);
      delay(80);
      digitalWrite(buzzer, LOW);
      comioFruta = false;
    }
    dibujarSerpienteMatriz();

    if (perdioJuego) {
    tocarMelodiaDerrota();
      perdioJuego = false;
    }

    
  display.clearDisplay();

  // Cabecera
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("SNAKE ESP");

  display.setCursor(91, 0);
  display.print("ARCADE");

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  // Etiqueta y puntuacion
  display.setCursor(0, 17);
  display.print("PUNTOS");

  display.setTextSize(3);
  display.setCursor(0, 27);

  if (puntuacion < 10) display.print("00");
  else if (puntuacion < 100) display.print("0");

  display.print(puntuacion);

  // Longitud
  display.setTextSize(1);
  display.setCursor(88, 20);
  display.print("LONG.");

  display.setTextSize(2);
  display.setCursor(88, 32);

  if (longitudSerpiente < 10) display.print("0");
  display.print(longitudSerpiente);

  // Pie de pantalla
  display.drawLine(0, 53, 127, 53, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 56);

  if (juegoTerminado) {
    display.print("GAME OVER!");
  } else {
    display.print("JUGANDO...");
    display.setCursor(91, 56);
    display.print("*");
  }

  display.display();


    if (juegoTerminado) {
      delay(2000);
      reiniciarJuego();
    }

    ultimoMovimiento = millis();
  }
  if (millis() - ultimoCambioLed > 500) {
    estadoLed = !estadoLed;
    digitalWrite(ledRojo, estadoLed);
    digitalWrite(ledAzul, !estadoLed);
    ultimoCambioLed = millis();
  }
  servidor.handleClient();
}