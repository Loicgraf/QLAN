#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <limits.h> // Pour utiliser ULONG_MAX qui est la valeur maximale pour un unsigned long.

RF24 radio(7, 8); // CE, CSN pins
const byte addresses[][6] = {"00001", "00002"}; // Ajouter les salles si besoins
const char* roomNames[] = {"Salle A", "Salle B"}; // le nom des salles
const char text[] = "LOCATE"; // Nom de la requete envoyer aux salles
const uint8_t numAttempts = 5; // nombrte de requètes à envoyer (+ de requète = plus de temps de d'execution)
unsigned long lastRequestTime = 0; // Enregistre le moment où la dernière requête a été envoyée
const unsigned long requestInterval = 10000; // Intervalle entre les requêtes en millisecondes 
const float penaltyFactor = 0.10; // Facteur de pénalité pour les échecs

void setup() {
  Serial.begin(9600);
  setupNRF24();
}

void loop() {
  unsigned long currentMillis = millis();

  // gère la communication (ici 3s (requestInterval))
  if (currentMillis - lastRequestTime >= requestInterval) {
    lastRequestTime = currentMillis;
    sendLocateRequest();
  }
}

/*/////////////////////////////////////////////////////////////
            Setup du NRF24
/////////////////////////////////////////////////////////////*/
void setupNRF24() {
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // Configuration de la puissance d'émission
  radio.setDataRate(RF24_250KBPS); // débit de données
  radio.setChannel(108); // canal
}

/*/////////////////////////////////////////////////////////////
      Partie pour déterminer la salle la plus proche
/////////////////////////////////////////////////////////////*/
void sendLocateRequest() {
  //Serial.println("Envoi de la requête de localisation");

  uint8_t responseCounts[sizeof(addresses) / 6] = {0}; // Tableau pour stocker le nombre de réponses reçues pour chaque adresse
  unsigned long responseTimes[sizeof(addresses) / 6] = {0}; // Tableau pour stocker le temps total de réponse pour chaque adresse

  // Pour chaque adresse 
  for (uint8_t i = 0; i < sizeof(addresses) / 6; i++) {
    // boucle du nombre de requete envoyé
    for (uint8_t attempt = 0; attempt < numAttempts; attempt++) {
      radio.openWritingPipe(addresses[i]); // Définition de l'adresse de destination pour l'envoi
      radio.openReadingPipe(1, addresses[i]); // Définition de l'adresse de réception pour recevoir les réponses
      radio.stopListening(); // Arrêt de l'écoute pour passer en mode écriture
      delayms(2);  // Petite temporisation pour stabiliser la communication
      radio.write(&text, sizeof(text)); // Envoi du message de demande de localisation
      Serial.print("Requête envoyée à l'adresse ");
      Serial.println((char*)addresses[i]);

      radio.startListening(); // Passage en mode écoute pour attendre les réponses
      unsigned long started_waiting_at = millis();
      boolean timeout = false; // Variable pour suivre si un délai d'attente s'est écoulé

      // Attente de la disponibilité d'une réponse ou d'un délai d'attente
      while (!radio.available()) {
        if (millis() - started_waiting_at > 300) { // Timeout de 300 ms
          timeout = true;
          break;
        }
      }

      // Traitement de la réponse reçue, si elle est disponible avant le délai d'attente
      if (!timeout) {
        char response[32] = ""; // stockage de la réponse
        radio.read(&response, sizeof(response)); // lecture de la réponse
        unsigned long responseTime = millis() - started_waiting_at; // Calcul du temps de réponse
        responseCounts[i]++; // Incrémentation du compteur de réponses pour cette adresse
        responseTimes[i] += responseTime; // Ajout du temps de réponse total pour cette adresse
        Serial.print("Réponse reçue: ");
        Serial.println(response);
        Serial.print("Temps de réponse: ");
        Serial.println(responseTime);
      } else {
        // Ajouter une pénalité pour l'échec
        responseTimes[i] += 300 * (1 + penaltyFactor); // Pénalité de 10% du timeout
        /*Serial.print("Pas de réponse de l'adresse ");
        Serial.println((char*)addresses[i]);*/
        Serial.print("Temps de réponse pénalisé: ");
        Serial.println(300 * (1 + penaltyFactor));
      }

      radio.stopListening(); // Arrêt de l'écoute pour passer en mode écriture pour la prochaine adresse
    }
  }

  // Recherche de l'adresse avec le temps de réponse moyen le plus faible
  int8_t bestIndex = -1; // Initialisation de l'indice de la meilleure adresse à -1
  unsigned long bestTime = ULONG_MAX; // Initialisation du meilleur temps à la valeur maximale

  // Boucle pour trouver la meilleure adresse avec des réponses
  for (uint8_t i = 0; i < sizeof(addresses) / 6; i++) {
    // Si des réponses ont été reçues pour cette adresse
    if (responseCounts[i] > 0) {
      unsigned long averageResponseTime = responseTimes[i] / responseCounts[i]; // Calcul du temps de réponse moyen
      // Si le temps de réponse moyen est plus faible que le meilleur temps actuel
      if (averageResponseTime < bestTime) {
        bestTime = averageResponseTime; // Mise à jour du meilleur temps
        bestIndex = i; // Mise à jour de l'indice de la meilleure adresse
      }
    }
  }

  // Affichage de l'adresse la plus proche, si une adresse avec des réponses a été trouvée
  if (bestIndex != -1) {
    Serial.print("-----------------------------------Salle la plus proche: ");
    Serial.println((char*)roomNames[bestIndex]);
  } else {
    Serial.println("Pas de réponse");
  }
}

void delayms(int interval) { //remplace le delay pour éviter d'areter totalement l'arduino
  unsigned long currentTime = millis();
  unsigned long previousTime = millis();
  
  while (currentTime - previousTime < interval) {
    currentTime = millis();
  }
}
