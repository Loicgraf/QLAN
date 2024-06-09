#include <ESP8266WiFi.h>

const char* ssid = "votre_ssid";
const char* password = "mot_de_passe";
const char* host = "adresse_ip_esp32";
const char* cle = "xylofone";

const int sensorPin = A0;
const int threshold = 512;  // Ajustez en fonction de votre signal
int smoothValue = 0;
float alpha = 0.75;
unsigned long lastPeakTime = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connexion au WiFi...");
  }
  
  Serial.println("Connecté au WiFi");
}

void loop() {
  WiFiClient client;
  
  if (!client.connect(host, 80)) {
    Serial.println("Connexion échouée");
    delay(1000);
    return;
  }
  
  int bpm = detectBPM();
  String message = String(cle) + ":" + String(bpm);
  client.print(message); // Envoyer la clé et l'entier
  client.print('\n'); // Ajouter un caractère de fin de ligne
  
  Serial.print("Donnée envoyée: ");
  Serial.println(message);
  
  client.stop();
  delay(5000); // Attendre 5 secondes avant d'envoyer à nouveau
}

int detectBPM() {
  int rawValue = analogRead(sensorPin);
  smoothValue = alpha * rawValue + (1 - alpha) * smoothValue;
  unsigned long currentTime = millis();

  // Détection des pics avec validation
  if (smoothValue > threshold) {
    if (currentTime - lastPeakTime > 300) {  // Ignorer les pics trop proches
      unsigned long interval = currentTime - lastPeakTime;
      lastPeakTime = currentTime;
      int bpm = 60000 / interval;
      if (bpm > 30 && bpm < 180) {  // Ignorer les valeurs de BPM non réalistes
        return bpm;
      }
    }
  }
  return -1;  // Retourne -1 si aucun BPM valide n'a été détecté
}
