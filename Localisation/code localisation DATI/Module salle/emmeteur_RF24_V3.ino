#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN pins

const byte address[6] = "00002"; // Adresse de communication pour la salle B


void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS); // Assurez-vous que tous les modules utilisent le même débit de données
  radio.setChannel(108); // Assurez-vous que tous les modules utilisent le même canal
  radio.openReadingPipe(1, address);
  radio.startListening();
}

void loop() {
  const char roomName[] = "Salle B"; // Nom de la salle
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text)); // ajouter en interruption ?
    /*Serial.print("Requete recue: ");
    Serial.println(text);*/
   
    if (strcmp(text, "LOCATE") == 0) {
      //delay(random(100, 500)); // Attendre un délai aléatoire entre 100 et 500 ms pour éviter les collisions
      radio.stopListening();
      radio.openWritingPipe(address);
      radio.write(&roomName, sizeof(roomName));
      /*Serial.print("Réponse envoyée: ");
      Serial.println(roomName);*/
      radio.startListening();
    }
  }
}
