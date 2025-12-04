#include <Arduino.h>

// Most ESP32 dev boards have the built-in LED on GPIO 2
#define LED_PIN 2

void setup() {
  // Initialize the LED pin as an output
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Blink LED Project Started");
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // Turn the LED on
  Serial.println("LED ON");
  delay(1000); // Wait for a second

  digitalWrite(LED_PIN, LOW); // Turn the LED off
  Serial.println("LED OFF");
  delay(1000); // Wait for a second
}
