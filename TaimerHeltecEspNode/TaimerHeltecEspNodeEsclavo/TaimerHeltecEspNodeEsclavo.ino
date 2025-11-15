#include <LoRa.h>
#include <Preferences.h>

#define LORA_SS 5
#define LORA_RST 14
#define LORA_DI0 26
#define LORA_BAND 915E6

#define LED_PIN 2

const int nodoID = 2;  // Cambia según el nodo

struct Bloque {
  int startMin;  // minutos desde 00:00
  int endMin;
  int onTime;
  int offTime;
};

Bloque bloques[6];
int numBloques = 0;

unsigned long previousMillis = 0;
bool ledState = false;

// "Reloj virtual"
int horaInicialMin = 0;
unsigned long millisHoraInicial = 0;

Preferences prefs;  // para almacenamiento en flash

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("Fallo al iniciar LoRa");
    while (true);
  }

  prefs.begin("config", false);
  cargarConfiguracion();

  Serial.println("Nodo esclavo iniciado.");
}

void loop() {
  recibirMensajeLoRa();
  manejarTemporizador();
}

void recibirMensajeLoRa() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0) return;

  String msg = "";
  while (LoRa.available()) {
    msg += (char)LoRa.read();
  }

  Serial.print("Mensaje recibido: ");
  Serial.println(msg);

  // ------------------------
  if (msg.startsWith("time:")) {
    String hora = msg.substring(5);
    int h = hora.substring(0, 2).toInt();
    int m = hora.substring(3).toInt();
    horaInicialMin = h * 60 + m;
    millisHoraInicial = millis();

    guardarHora(); // 💾 Guardar hora en memoria
    Serial.print("Hora sincronizada: ");
    Serial.println(hora);
  }

  // ------------------------
  else if (msg.startsWith("schedule:")) {
    int idStart = 9;
    int idEnd = msg.indexOf(';', idStart);
    int id = msg.substring(idStart, idEnd).toInt();

    if (id != nodoID) {
      Serial.println("Configuración no es para este nodo.");
      return;
    }

    Serial.println("Procesando bloques...");
    numBloques = 0;

    String bloquesStr = msg.substring(idEnd + 1);
    int pos = 0;

    while (pos < bloquesStr.length() && numBloques < 6) {
      int bStart = bloquesStr.indexOf("b", pos);
      if (bStart == -1) break;

      int colon = bloquesStr.indexOf(':', bStart);
      int dash = bloquesStr.indexOf('-', colon);
      int comma1 = bloquesStr.indexOf(',', dash);
      int comma2 = bloquesStr.indexOf(',', comma1 + 1);
      int semi = bloquesStr.indexOf(';', comma2);

      if (semi == -1) semi = bloquesStr.length();

      String inicio = bloquesStr.substring(colon + 1, dash);
      String fin = bloquesStr.substring(dash + 1, comma1);
      String on = bloquesStr.substring(comma1 + 1, comma2);
      String off = bloquesStr.substring(comma2 + 1, semi);

      Bloque b;
      b.startMin = parseHora(inicio);
      b.endMin = parseHora(fin);
      b.onTime = on.toInt() * 1000;
      b.offTime = off.toInt() * 1000;

      bloques[numBloques++] = b;

      Serial.print("Bloque ");
      Serial.print(numBloques);
      Serial.print(": ");
      Serial.print(inicio);
      Serial.print(" - ");
      Serial.print(fin);
      Serial.print(" ON: ");
      Serial.print(b.onTime);
      Serial.print(" OFF: ");
      Serial.println(b.offTime);

      pos = semi + 1;
    }

    guardarConfiguracion(); // 💾 Guardar bloques en memoria
    Serial.println("Configuración guardada en memoria flash.");
  }
}

int parseHora(String hora) {
  int h = hora.substring(0, 2).toInt();
  int m = hora.substring(3).toInt();
  return h * 60 + m;
}

int getMinutosActuales() {
  return horaInicialMin + (millis() - millisHoraInicial) / 60000;
}

void manejarTemporizador() {
  int ahoraMin = getMinutosActuales();

  // Buscar bloque activo
  for (int i = 0; i < numBloques; i++) {
    Bloque b = bloques[i];
    if (ahoraMin >= b.startMin && ahoraMin < b.endMin) {
      ejecutarTemporizador(b.onTime, b.offTime);
      return;
    }
  }

  // Si no hay bloque activo
  digitalWrite(LED_PIN, LOW);
  ledState = false;
}

void ejecutarTemporizador(unsigned long onTime, unsigned long offTime) {
  unsigned long currentMillis = millis();

  if (ledState && currentMillis - previousMillis >= onTime) {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
    previousMillis = currentMillis;
  } else if (!ledState && currentMillis - previousMillis >= offTime) {
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
    previousMillis = currentMillis;
  }
}

void guardarConfiguracion() {
  prefs.putInt("numBloques", numBloques);
  for (int i = 0; i < numBloques; i++) {
    prefs.putInt(("start" + String(i)).c_str(), bloques[i].startMin);
    prefs.putInt(("end" + String(i)).c_str(), bloques[i].endMin);
    prefs.putInt(("on" + String(i)).c_str(), bloques[i].onTime);
    prefs.putInt(("off" + String(i)).c_str(), bloques[i].offTime);
  }
}

void cargarConfiguracion() {
  numBloques = prefs.getInt("numBloques", 0);
  if (numBloques > 0) {
    Serial.println("📦 Cargando configuración almacenada...");
    for (int i = 0; i < numBloques; i++) {
      bloques[i].startMin = prefs.getInt(("start" + String(i)).c_str(), 0);
      bloques[i].endMin = prefs.getInt(("end" + String(i)).c_str(), 0);
      bloques[i].onTime = prefs.getInt(("on" + String(i)).c_str(), 1000);
      bloques[i].offTime = prefs.getInt(("off" + String(i)).c_str(), 1000);

      Serial.printf("Bloque %d: %02d:%02d - %02d:%02d | ON %d ms | OFF %d ms\n",
                    i + 1,
                    bloques[i].startMin / 60, bloques[i].startMin % 60,
                    bloques[i].endMin / 60, bloques[i].endMin % 60,
                    bloques[i].onTime, bloques[i].offTime);
    }
  } else {
    Serial.println("⚠️ No hay configuración previa guardada.");
  }

  // También carga la hora sincronizada si existe
  horaInicialMin = prefs.getInt("horaInicial", 0);
  millisHoraInicial = millis();
}

void guardarHora() {
  prefs.putInt("horaInicial", horaInicialMin);
}
