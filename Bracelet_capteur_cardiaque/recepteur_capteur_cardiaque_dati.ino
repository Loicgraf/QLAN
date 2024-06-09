#include <WiFi.h>

const char* ssid = "votre_ssid";
const char* password = "mot_de_passe";
const char* cle = "xylofone";

WiFiServer server(80);

int coeur = -1;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connexion au WiFi...");
  }
  
  Serial.print("Connecté au WiFi. Adresse IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  received_bpm();
  Serial.print("Donnée reçue avec clé correcte: ");
  Serial.println(coeur);
}

void received_bpm() {
  WiFiClient client = server.available();
  
  if (client) {
    String message_recu = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') { // Fin de ligne, le message est complet
          if (message_recu.startsWith(cle)) { // Vérifier si le message commence par la clé
            int separatorIndex = message_recu.indexOf(':');
            if (separatorIndex != -1) {
              String bpmString = message_recu.substring(separatorIndex + 1);
              coeur = bpmString.toInt(); // Convertir la chaîne en entier
            }
          }
          message_recu = ""; // Réinitialiser la chaîne pour le prochain message
        } 
        else {
          message_recu += c;
        }
      }
    }
    client.stop();
  }
}
