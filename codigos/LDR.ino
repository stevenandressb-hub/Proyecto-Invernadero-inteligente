#define LDR_PIN 35

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  Serial.println("=== PRUEBA SENSOR LDR ===");
}

void loop() {

  int luz = analogRead(LDR_PIN);

  Serial.print("Valor LDR: ");
  Serial.println(luz);

  delay(1000);
}