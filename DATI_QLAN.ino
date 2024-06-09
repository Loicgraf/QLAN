#include <ArduinoJson.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <Bounce2.h>
#include <TaskScheduler.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Adafruit_MPU6050.h>//https://github.com/adafruit/Adafruit_MPU6050
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <limits.h> // Pour utiliser ULONG_MAX qui est la valeur maximale pour un unsigned long.

// Informations de connexion WiFi
String ssid = "votre_ssid";
String password = "votre_mot_de_passe";
// Adresses des serveurs
String ip ="adress_ip_rasp";

//Objets
Adafruit_MPU6050 mpu;
rgb_lcd lcd;
WiFiServer server(80);

//-----------------------------------------//
//   Variables liés au capteur_cardiaque
const char* cle = "xylofone";
const unsigned int max_coeur = 120;
const unsigned int min_coeur = 50;
//-----------------------------------------//

//-----------------------------------------//
//Variables liés aux fréquences
RF24 radio(4, 5); // CE, CSN pins
const byte addresses[][6] = {"00001", "00002"}; // Ajouter les salles si besoins
const char* roomNames[] = {"Salle A", "Salle B"}; // le nom des salles
const char text[] = "LOCATE"; // Nom de la requete envoyer aux salles
const uint8_t numAttempts = 5; // nombrte de requètes à envoyer (+ de requète = plus de temps de d'execution)
unsigned long lastRequestTime = 0; // Enregistre le moment où la dernière requête a été envoyée
const unsigned long requestInterval = 10000; // Intervalle entre les requêtes en millisecondes 
const float penaltyFactor = 0.10; // Facteur de pénalité pour les échecs
//-----------------------------------------//

//-----------------------------------------//
//Variables liés différents axes du gyroscope/accelerometre
float ax=0;
float ay=0;
float az=0;
float acceleration_total=0;

float SeuilVerticalite = 250;
float gx=0;

// Calcul du seuil
float moyenne = 23.5; // Moyenne des mesures d'accélération
float ecart_type = 2.0; // Ecart-type des mesures d'accélération
float facteur = 0.75; // Facteur pour déterminer le seuil
float seuilChute = moyenne + (facteur * ecart_type);
//----------------------------------------//

//pins
const int buzzerPin = 12;

//Variables utilisateur

String apiKey = "tPmAT5Ab3j7F9";
String utilisateur="";
String position="Inconnue";
int coeur = -1;
int coeurm = 0;
int n_alerte = 1;
String message;
String ping;
String etat;
int compteur_alarme = 15;
int pingtime=30;

bool uncoping=false;
String urle = "http://";
String get_data = urle + ip +"/get_data.php";
String list_tables = urle + ip +"/list_tables.php";
String insert_data = urle + ip +"/insert_data.php";

// Boutons
Bounce button1 = Bounce();
Bounce button2 = Bounce();
Bounce button3 = Bounce();


// Tableau des noms enregistrés
const int maxNames = 30;
String names[maxNames];
int namesCount = 0;
int nameIndex =0; // indexe de navigation dans les noms de tables

int i;
// Etats des menus
enum MenuState { MENU0, MENU1, MENU2, MENU3, MENU4, MENU5, MENU6, MENU7, MENU8, MENU9 };
MenuState currentMenu = MENU0;

// Déclaration du Scheduler
Scheduler runner;

// Prototypes des fonctions
void displayMenu0();
void displayMenu1();
void displayMenu2();
void displayMenu3();
void displayMenu4();
void displayMenu5();
void displayMenu6();
void displayMenu7();
void writeline();
void readline();
void backgroundTask3();
void updateDisplay();
void deserializeJson(String);
void majnames();
void alarme();
void pingage();
void reco();
void newWiFi();
void scanssid();
void readMPU();
void setupNRF24();
void sendLocateRequest();
void received_bpm();
void niveau_alerte(bool);

// Déclaration des tâches
Task task1(1000, TASK_FOREVER, &writeline);
Task task2(1000, TASK_FOREVER, &readline);
Task task3(10000, TASK_FOREVER, &backgroundTask3);
Task task4(5000, TASK_FOREVER, &backgroundTask3);
Task task5(1000, TASK_FOREVER, &backgroundTask3);
Task displayTask(500, TASK_FOREVER, &updateDisplay);
Task alerteTask(1000, TASK_FOREVER, &alarme);
Task pingTask(1000, TASK_FOREVER, &pingage);
Task gyroTask(200, TASK_FOREVER, &readMPU);
Task freqTask(30000, TASK_FOREVER, &sendLocateRequest);
Task bpmTask(1000, TASK_FOREVER, &received_bpm);

void setup() {
  Wire.begin();
  Serial.begin(115200);

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpu.begin();

  setupNRF24();

  //Initialisation des boutons
  button1.attach(25, INPUT_PULLUP);//Left
  button2.attach(27, INPUT_PULLUP);//Right
  button3.attach(26, INPUT_PULLUP);//Select
  pinMode(buzzerPin, OUTPUT);

  //Intervalle de débounce en ms
  button1.interval(25);
  button2.interval(25);
  button3.interval(25);

  lcd.begin(16,2);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connexion WiFi...");
  }
  lcd.clear();
  lcd.setCursor(0,0);
  
  HTTPClient http;
  http.begin(list_tables);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
    deserializeJson(payload);
  } else {
  }
  http.end();
  
  // Ajouter les tâches au Scheduler
  runner.init();
  runner.addTask(task1);
  runner.addTask(task2);
  runner.addTask(task3);
  runner.addTask(task4);
  runner.addTask(task5);
  runner.addTask(displayTask);
  runner.addTask(alerteTask);
  runner.addTask(pingTask);
  runner.addTask(gyroTask);
  runner.addTask(freqTask);
  runner.addTask(bpmTask);
  //Démarrer les tâches de fond
  displayTask.enable();
  // Afficher le menu initial
  displayMenu0();
}

void loop() {
  runner.execute();

  button1.update();//bas
  button2.update();//haut
  button3.update();//Select/Alarme
  if(message=="ping" && uncoping == false){
    uncoping =true;
    currentMenu = MENU6;
    displayMenu6();
    pingTask.enable();
  }

  if (button1.fell()) {
    if (currentMenu == MENU1 && nameIndex-1>=0) {
      nameIndex = (nameIndex - 1) % maxNames;
      displayMenu1();
    } else if (currentMenu == MENU3){
      currentMenu = MENU4;
      displayMenu4();
    }else if (currentMenu == MENU4)
    {
      currentMenu = MENU5;
      displayMenu5();
    }else if (currentMenu == MENU5)
    {
      currentMenu = MENU3;
      displayMenu3();
    }
  }
  
  if (button2.fell()) {
    // UP
    if(currentMenu == MENU0 ){
      newWiFi();
      displayMenu0();
    }
    else if(currentMenu == MENU3){
      displayTask.disable();
      niveau_alerte(false);
      gyroTask.disable();
      freqTask.disable();
      bpmTask.disable();

      newWiFi();

      displayTask.enable();
      niveau_alerte(true);
      gyroTask.enable();
      freqTask.enable();
      bpmTask.enable();
    }

    if (currentMenu == MENU1 && nameIndex+1<=namesCount-1) {
      nameIndex = (nameIndex + 1) % maxNames;
      displayMenu1();
    }
    else if (currentMenu == MENU4) {
      n_alerte++;
      if(n_alerte>=4)
      {
        n_alerte = 1;
      }
      niveau_alerte(true);
      displayMenu4();
    }
    else if (currentMenu == MENU5){
      etat="DECO";
      writeline();
      currentMenu = MENU1;
      displayMenu1();

      niveau_alerte(false);
      gyroTask.disable();
      freqTask.disable();
      bpmTask.disable();
    }
  }
  
  if (button3.fell()) {
    // Select alarme
    if(currentMenu == MENU0){
      currentMenu = MENU1;
      displayMenu1();
    }else if (currentMenu == MENU1) {
      utilisateur=names[nameIndex];
      readline();
      if(etat == "new"){
        currentMenu = MENU2;
        displayMenu2();
      }
      else {
        currentMenu = MENU3;
        displayMenu3();
        etat="comm";
        writeline();

        niveau_alerte(true);
        gyroTask.enable();
        freqTask.enable();
        bpmTask.enable();
      }
    }
    else if (currentMenu == MENU2) {
      currentMenu = MENU3;
      displayMenu3();
      etat="comm";
      writeline();

      niveau_alerte(true);
      gyroTask.enable();
      freqTask.enable();
      bpmTask.enable();
    }
    else if (currentMenu == MENU3 ||currentMenu == MENU4 ||currentMenu == MENU5) {
      currentMenu = MENU7;
      alerteTask.enable();
      displayMenu7();
    }
    else if (currentMenu == MENU6){
      currentMenu = MENU3;
      displayMenu3();
      pingTask.disable();
      pingtime=30;
      message="";
      writeline();
      uncoping =false;
      noTone(buzzerPin);
    }
    else if (currentMenu == MENU7){
      etat="comm";
      uncoping =false;
      currentMenu = MENU3;
      displayMenu3();
      alerteTask.disable();
      compteur_alarme = 15;
      noTone(buzzerPin);
    }
  }

  if(WiFi.status() != WL_CONNECTED){
    displayTask.disable();
    reco();
    displayTask.enable();
  }
}

void displayMenu0(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Appuyez M pour");
  lcd.setCursor(0, 1);
  lcd.print("commencer");
}

void displayMenu1() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Choix:");
  lcd.setCursor(0, 1);
  lcd.print(names[nameIndex]);
}

void displayMenu2() {
  lcd.clear();
  lcd.print("Suite=MID");
  lcd.setCursor(0, 0);
}

void displayMenu3() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(utilisateur);
  lcd.setCursor(0, 1);
  lcd.print(coeur);
  lcd.setCursor(15, 1);
  lcd.print(n_alerte);
}

void displayMenu4() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Niveau d'Alerte");
  lcd.setCursor(8, 1);
  lcd.print(n_alerte);
}

void displayMenu5() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Deconnexion |OUI");
  lcd.setCursor(0, 1);
  lcd.print("            |NON");
}

void displayMenu6() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Appuyez M si");
  lcd.setCursor(0,1);
  lcd.print("tout va bien !");
}

void displayMenu7() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Decompte alarme");
  lcd.setCursor(0, 1);
  lcd.print("Appuyez M - ");
  lcd.setCursor(13, 1);
  lcd.print(compteur_alarme);
}

//  Fonction à exécuter grâce a scheduler en arrière plan
void readline() {
    HTTPClient http;
    String url = get_data + String("?table_name=") + utilisateur;

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();

      // Désérialiser le JSON
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);
      
      if (!error) {
        String nom = doc["nom"].as<String>();
        coeurm = doc["coeurm"];
        message = doc["message"].as<String>();
      } else {
        Serial.print("Erreur de désérialisation: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
}

void  writeline() { //insertion de data
    if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(insert_data);
    // Préparer les données POST
    String postData = "table_name=" + utilisateur +
                      "&api_key=" + apiKey +
                      "&nom=" + utilisateur +
                      "&etat=" + etat +
                      "&coeur=" + String(coeur) +
                      "&coeurm=" + String(coeurm) +
                      "&position=" + position +
                      "&message=" + message +
                      "&n_alerte=" + String(n_alerte);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
  }
}

void backgroundTask3() {
  readline();
  writeline();
}

void updateDisplay(){
  switch(currentMenu){
    case MENU1:
      displayMenu1();
      break;
    case MENU2:
      displayMenu2();
      break;
    case MENU3:
      displayMenu3();
      break;
    case MENU4:
      displayMenu4();
      break;
    case MENU5:
      displayMenu5();
      break;
    case MENU6:
      displayMenu6();
      break;
    case MENU7:
      displayMenu7();
      break;
  }
}

//Récupération des tables déjà existantes
void deserializeJson(String payload) {
  // Désérialiser le JSON reçu
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("Erreur de désérialisation: ");
    Serial.println(error.c_str());
    return;
  }

  namesCount = doc.size();
  for (int i = 0; i < namesCount; i++) {
    names[i] = doc[i].as<String>();
  }
}

void majnames(){
  HTTPClient http;
  http.begin(list_tables);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
    deserializeJson(payload);
  }
  http.end();
}

void alarme(){
  compteur_alarme--;
  if(compteur_alarme<0){
    compteur_alarme=0;
  }
  if(compteur_alarme<=0){
    etat="alerte";
    writeline();
  }
  if(compteur_alarme%2==0){
    tone(buzzerPin, 500);
  } else {
    noTone(buzzerPin);
  }
}

void pingage(){
  pingtime--;
  if(pingtime<0){
    pingtime=0;
  }
  if(pingtime<=0){
    currentMenu = MENU7;
    alerteTask.enable();
    displayMenu7();
    pingtime=30;
    message="";
    pingTask.disable();
  }
  if(pingtime%3==0){
    tone(buzzerPin, 500);
  } else {
    noTone(buzzerPin);
  }
}

void reco(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connexion WiFi..");
    delay(1000);
  }
}

void scanssid(){
  int n = WiFi.scanNetworks();
  i=0;
  if (n == 0) {
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(WiFi.SSID(i).c_str());
    while (true) {

      button1.update();//Left
      button2.update();//Right
      button3.update();//Select/Alarme
      if(button1.fell()){
        if(i>0){
          i--;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(WiFi.SSID(i).c_str());
        }
      }
      if(button2.fell()){
        if(i<n-1){
          i++;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(WiFi.SSID(i).c_str());
        }
      }
      if(button3.fell()){
        ssid=WiFi.SSID(i).c_str();
        break;
      }
    }
  }
  delay(10);
  WiFi.scanDelete();
  button1.update();//Left
  button2.update();//Right
  button3.update();//Select/Alarme
}

void newWiFi(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Recherche wifi..");
  scanssid();
  reco();
}

void niveau_alerte(bool verif){
  if(verif){
    if (n_alerte == 1){
      task4.disable();
      task5.disable();
      task3.enable();
    }
    else if(n_alerte == 2){
      task3.disable();
      task5.disable();
      task4.enable();
    }
    else if(n_alerte == 3){
      task3.disable();
      task4.disable();
      task5.enable();
    }
  }
  else{
    task3.disable();
    task4.disable();
    task5.disable();
  }
}

//Fonction du gyroscope les valeurs de l'acceleromètre/gyroscope
void readMPU() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    gx = g.gyro.x*(180.0 / PI);

    acceleration_total = sqrt(ax * ax + ay * ay + az * az);

    if (abs(gx) >= SeuilVerticalite){
      // Vérifier si l'accélération dépasse le seuil de chute
      if (acceleration_total >= seuilChute && ((coeur > max_coeur) || (coeur < min_coeur)) && coeur != -1) { //si il y a chute et rythme cardiaque anormal
        currentMenu = MENU7;
        alerteTask.enable(); //alarme plus rapide
        displayMenu7();
        Serial.println("CHUTE_et_COEUR");
      } 
      else if (acceleration_total >= seuilChute) { //si il y a chute
        currentMenu = MENU7;
        alerteTask.enable();
        displayMenu7();
        Serial.println("CHUTE");
      }
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
  uint8_t responseCounts[sizeof(addresses) / 6] = {0}; // Tableau pour stocker le nombre de réponses reçues pour chaque adresse
  unsigned long responseTimes[sizeof(addresses) / 6] = {0}; // Tableau pour stocker le temps total de réponse pour chaque adresse

  // Pour chaque adresse 
  for (uint8_t i = 0; i < sizeof(addresses) / 6; i++) {
    // boucle du nombre de requête envoyée
    for (uint8_t attempt = 0; attempt < numAttempts; attempt++) {
      radio.openWritingPipe(addresses[i]); // Définition de l'adresse de destination pour l'envoi
      radio.openReadingPipe(1, addresses[i]); // Définition de l'adresse de réception pour recevoir les réponses
      radio.stopListening(); // Arrêt de l'écoute pour passer en mode écriture
      delay(2);  // Petite temporisation pour stabiliser la communication
      radio.write(&text, sizeof(text)); // Envoi du message de demande de localisation

      radio.startListening(); // Passage en mode écoute pour attendre les réponses
      unsigned long started_waiting_at = millis();
      bool timeout = false; // Variable pour suivre si un délai d'attente s'est écoulé

      // Attente de la disponibilité d'une réponse ou d'un délai d'attente
      while (!radio.available()) {
        runner.execute();
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
      } else {
        // Ajouter une pénalité pour l'échec
        responseTimes[i] += 300 * (1 + penaltyFactor); // Pénalité de 10% du timeout
      }

      radio.stopListening(); // Arrêt de l'écoute pour passer en mode écriture pour la prochaine adresse
      delay(100);
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
    position = (char*)roomNames[bestIndex];
  } else {
    Serial.println("Pas de réponse");
    position = "Inconnu";
  }
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

              if(((coeur > max_coeur) || (coeur < min_coeur)) && coeur != -1){
                displayMenu7();
                alerteTask.enable();
              }
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

void delayms(int interval) { //remplace le delay pour éviter d'areter totalement l'arduino
  unsigned long currentTime = millis();
  unsigned long previousTime = millis();
  while (currentTime - previousTime < interval) {
    runner.execute();
    currentTime = millis();
  }
}
