#include <SPI.h>
#include <LoRa.h>

#define SS 18
#define RST 14
#define DIO0 26
#define BAND 915E6

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("🚀 Heltec maestro iniciado");
  
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND)) {
    Serial.println("❌ Error al iniciar LoRa");
    while (1);
  }
  Serial.println("✅ LoRa inicializado correctamente");
}

void loop() {
  // Si llega algo desde Node.js por el puerto serie:
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      Serial.print("🔁 Enviando por LoRa: ");
      Serial.println(msg);
      LoRa.beginPacket();
      LoRa.print(msg);
      LoRa.endPacket();
    }
  }
}
