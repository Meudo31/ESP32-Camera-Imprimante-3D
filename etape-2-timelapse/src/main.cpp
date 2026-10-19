// Caméra pour imprimante 3D - ESP32-CAM (AI-Thinker)
// Étape 2 : flux vidéo + timelapse enregistré sur carte micro SD
//
// Fonctionnement :
//  1. Tout ce que fait l'étape 1 : page web sur http://camera-imprimante.local, flux sur le port 81.
//  2. Un bouton de la page démarre un timelapse : une photo toutes les N secondes,
//     enregistrée sur la carte SD dans /timelapse/0001/img_00001.jpg, img_00002.jpg...
//  3. Avec un intervalle de 0, seule l'adresse /cliche prend une photo : la machine
//     (Klipper, OctoPrint) l'appelle à chaque changement de couche pour un timelapse sans tremblement.
//  4. On assemble les images en vidéo sur le PC avec ffmpeg (voir le README).
//
// Carte SD en mode 1 bit : seules GPIO2, 14 et 15 servent à la carte,
// ce qui laisse GPIO4 (LED) libre.
//
// Alimentation : 5 V sur la broche 5V, la caméra reste branchée en permanence.

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SD_MMC.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "config.h"

// ---------- Broches ----------
#define BROCHE_LED    4
#define CANAL_LED     7   // canal LEDC libre (la caméra utilise le canal 0)

// Brochage caméra AI-Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define DOSSIER_TIMELAPSE "/timelapse"

httpd_handle_t serveurPage = nullptr;
httpd_handle_t serveurFlux = nullptr;
uint8_t ledIntensite = LED_INTENSITE_DEPART;

// ---------- État du timelapse ----------
// La carte SD n'est utilisée que depuis loop() : les pages web déposent une commande,
// loop() l'exécute. Ainsi deux tâches n'écrivent jamais sur la carte en même temps.
enum Commande { AUCUNE, DEMARRER, ARRETER, CLICHE };
volatile Commande commandeEnAttente = AUCUNE;
volatile uint32_t intervalleDemande = TIMELAPSE_INTERVALLE_S;

bool carteSdOk = false;
bool timelapseActif = false;
uint32_t intervalleS = TIMELAPSE_INTERVALLE_S;  // 0 = uniquement sur appel de /cliche
uint32_t numeroSession = 0;
uint32_t nbImages = 0;
uint32_t derniereImageMs = 0;
uint32_t espaceLibreMo = 0;

// ---------- Caméra ----------
bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = TAILLE_IMAGE;
  config.jpeg_quality = QUALITE_JPEG;
  config.fb_count = 2;                  // deux tampons : le flux est plus fluide
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erreur init caméra : 0x%x\n", err);
    return false;
  }
  sensor_t* capteur = esp_camera_sensor_get();
  capteur->set_vflip(capteur, RETOURNER_VERTICAL);
  capteur->set_hmirror(capteur, MIROIR_HORIZONTAL);
  return true;
}

// ---------- LED d'éclairage ----------
void reglerLed(uint8_t intensite) {
  ledIntensite = intensite;
  ledcWrite(CANAL_LED, intensite);
}

// ---------- Carte SD et timelapse ----------
void mesurerEspaceLibre() {
  if (carteSdOk) espaceLibreMo = (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
}

bool initCarteSd() {
  // Mode 1 bit : la LED (GPIO4) reste utilisable pour l'éclairage
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Pas de carte SD : le timelapse est désactivé");
    return false;
  }
  if (!SD_MMC.exists(DOSSIER_TIMELAPSE)) SD_MMC.mkdir(DOSSIER_TIMELAPSE);
  carteSdOk = true;
  mesurerEspaceLibre();
  Serial.printf("Carte SD : %u Mo libres\n", espaceLibreMo);
  return true;
}

void demarrerTimelapse(uint32_t intervalle) {
  if (!carteSdOk) return;
  // Prochain numéro de dossier libre : /timelapse/0001, /timelapse/0002...
  char dossier[32];
  do {
    numeroSession++;
    snprintf(dossier, sizeof(dossier), DOSSIER_TIMELAPSE "/%04lu", (unsigned long)numeroSession);
  } while (SD_MMC.exists(dossier));
  if (!SD_MMC.mkdir(dossier)) {
    Serial.println("Erreur : dossier impossible à créer sur la carte SD");
    return;
  }
  intervalleS = intervalle;
  nbImages = 0;
  derniereImageMs = 0;
  timelapseActif = true;
  Serial.printf("Timelapse démarré dans %s (une image toutes les %lu s)\n", dossier, (unsigned long)intervalleS);
}

void arreterTimelapse() {
  if (!timelapseActif) return;
  timelapseActif = false;
  mesurerEspaceLibre();
  Serial.printf("Timelapse arrêté : %lu images dans la session %04lu\n",
                (unsigned long)nbImages, (unsigned long)numeroSession);
}

void enregistrerImage() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Erreur : image impossible");
    return;
  }
  char chemin[48];
  snprintf(chemin, sizeof(chemin), DOSSIER_TIMELAPSE "/%04lu/img_%05lu.jpg",
           (unsigned long)numeroSession, (unsigned long)(nbImages + 1));
  File fichier = SD_MMC.open(chemin, FILE_WRITE);
  size_t ecrit = fichier ? fichier.write(fb->buf, fb->len) : 0;
  if (fichier) fichier.close();
  size_t taille = fb->len;
  esp_camera_fb_return(fb);

  if (ecrit != taille) {
    Serial.printf("Erreur d'écriture de %s : carte pleine ou retirée ? Timelapse arrêté\n", chemin);
    arreterTimelapse();
    return;
  }
  nbImages++;
  if (nbImages % 10 == 0) Serial.printf("Timelapse : %lu images\n", (unsigned long)nbImages);
}

void gererTimelapse() {
  Commande commande = commandeEnAttente;
  commandeEnAttente = AUCUNE;
  if (commande == DEMARRER) {
    arreterTimelapse();
    demarrerTimelapse(intervalleDemande);
  } else if (commande == ARRETER) {
    arreterTimelapse();
  } else if (commande == CLICHE && timelapseActif) {
    enregistrerImage();
  }

  if (timelapseActif && intervalleS > 0 && millis() - derniereImageMs >= intervalleS * 1000) {
    derniereImageMs = millis();
    enregistrerImage();
  }
}

// ---------- Page web ----------
static const char PAGE[] = R"rawliteral(<!doctype html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Caméra imprimante 3D</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:12px;text-align:center}
img{width:100%;max-width:1024px;border-radius:8px;background:#000}
.ligne{margin:12px auto;max-width:1024px;display:flex;gap:12px;align-items:center;justify-content:center;flex-wrap:wrap}
button,a.bouton{background:#e8590c;color:#fff;border:0;border-radius:6px;padding:10px 16px;font-size:16px;text-decoration:none}
input[type=range]{width:200px}
input[type=number]{width:70px;font-size:16px}
</style></head><body>
<h2>Caméra imprimante 3D</h2>
<img id="flux" alt="Flux vidéo">
<div class="ligne">
  <a class="bouton" href="/photo" target="_blank">Photo</a>
  <label>Éclairage <input id="led" type="range" min="0" max="255" value="%LED%"></label>
</div>
<div class="ligne">
  <label>Une image toutes les <input id="intervalle" type="number" min="0" max="3600" value="%INTERVALLE%"> s</label>
  <button onclick="fetch('/timelapse?action=demarrer&intervalle=' + document.getElementById('intervalle').value)">Démarrer le timelapse</button>
  <button onclick="fetch('/timelapse?action=arreter')">Arrêter</button>
</div>
<div class="ligne" id="etat">...</div>
<script>
document.getElementById('flux').src = location.protocol + '//' + location.hostname + ':81/flux';
document.getElementById('led').onchange = e => fetch('/led?val=' + e.target.value);
function majEtat() {
  fetch('/etat').then(r => r.json()).then(e => {
    let t = !e.sd ? 'Pas de carte SD' :
      e.actif ? 'Timelapse en cours : session ' + e.session + ', ' + e.images + ' images' : 'Timelapse arrêté';
    if (e.sd) t += ' · ' + e.libre_mo + ' Mo libres';
    document.getElementById('etat').textContent = t;
  }).catch(() => document.getElementById('etat').textContent = 'Caméra injoignable');
}
majEtat();
setInterval(majEtat, 2000);
</script>
</body></html>)rawliteral";

esp_err_t gererPage(httpd_req_t* req) {
  String page = PAGE;
  page.replace("%LED%", String(ledIntensite));
  page.replace("%INTERVALLE%", String(intervalleS));
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, page.c_str(), page.length());
}

esp_err_t gererPhoto(httpd_req_t* req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=photo.jpg");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

esp_err_t gererLed(httpd_req_t* req) {
  char requete[32];
  char valeur[8];
  if (httpd_req_get_url_query_str(req, requete, sizeof(requete)) == ESP_OK &&
      httpd_query_key_value(requete, "val", valeur, sizeof(valeur)) == ESP_OK) {
    reglerLed(constrain(atoi(valeur), 0, 255));
    Serial.printf("LED : %u\n", ledIntensite);
  }
  return httpd_resp_send(req, "OK", 2);
}

esp_err_t gererTimelapseWeb(httpd_req_t* req) {
  char requete[64];
  char valeur[16];
  if (httpd_req_get_url_query_str(req, requete, sizeof(requete)) == ESP_OK &&
      httpd_query_key_value(requete, "action", valeur, sizeof(valeur)) == ESP_OK) {
    if (strcmp(valeur, "demarrer") == 0) {
      char intervalle[8];
      if (httpd_query_key_value(requete, "intervalle", intervalle, sizeof(intervalle)) == ESP_OK) {
        intervalleDemande = constrain(atoi(intervalle), 0, 3600);
      }
      commandeEnAttente = DEMARRER;
    } else if (strcmp(valeur, "arreter") == 0) {
      commandeEnAttente = ARRETER;
    }
  }
  return httpd_resp_send(req, "OK", 2);
}

// Une image du timelapse en cours, à appeler depuis la machine à chaque changement de couche
esp_err_t gererCliche(httpd_req_t* req) {
  commandeEnAttente = CLICHE;
  return httpd_resp_send(req, "OK", 2);
}

esp_err_t gererEtat(httpd_req_t* req) {
  char json[160];
  snprintf(json, sizeof(json),
           "{\"sd\":%s,\"actif\":%s,\"session\":%lu,\"images\":%lu,\"intervalle\":%lu,\"libre_mo\":%lu,\"led\":%u}",
           carteSdOk ? "true" : "false", timelapseActif ? "true" : "false",
           (unsigned long)numeroSession, (unsigned long)nbImages, (unsigned long)intervalleS,
           (unsigned long)espaceLibreMo, ledIntensite);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, json, strlen(json));
}

// ---------- Flux vidéo MJPEG ----------
#define SEPARATION "frontiere-image-imprimante"
static const char* TYPE_FLUX = "multipart/x-mixed-replace;boundary=" SEPARATION;
static const char* DEBUT_IMAGE = "\r\n--" SEPARATION "\r\n";
static const char* ENTETE_IMAGE = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Un seul spectateur à la fois : la tâche du serveur reste dans cette boucle tant que le flux est ouvert.
esp_err_t gererFlux(httpd_req_t* req) {
  Serial.println("Flux : spectateur connecté");
  httpd_resp_set_type(req, TYPE_FLUX);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  const uint32_t intervalleMs = 1000 / IMAGES_PAR_SECONDE_MAX;
  char entete[64];
  esp_err_t res = ESP_OK;
  while (res == ESP_OK) {
    uint32_t debut = millis();
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Erreur : image impossible");
      res = ESP_FAIL;
      break;
    }
    res = httpd_resp_send_chunk(req, DEBUT_IMAGE, strlen(DEBUT_IMAGE));
    if (res == ESP_OK) {
      size_t n = snprintf(entete, sizeof(entete), ENTETE_IMAGE, fb->len);
      res = httpd_resp_send_chunk(req, entete, n);
    }
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);

    uint32_t duree = millis() - debut;
    if (duree < intervalleMs) delay(intervalleMs - duree);
  }
  Serial.println("Flux : spectateur parti");
  return res;
}

void demarrerServeurs() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;
  httpd_uri_t page  = { .uri = "/",      .method = HTTP_GET, .handler = gererPage,  .user_ctx = nullptr };
  httpd_uri_t photo = { .uri = "/photo", .method = HTTP_GET, .handler = gererPhoto, .user_ctx = nullptr };
  httpd_uri_t led   = { .uri = "/led",   .method = HTTP_GET, .handler = gererLed,   .user_ctx = nullptr };
  httpd_uri_t tl    = { .uri = "/timelapse", .method = HTTP_GET, .handler = gererTimelapseWeb, .user_ctx = nullptr };
  httpd_uri_t cliche = { .uri = "/cliche", .method = HTTP_GET, .handler = gererCliche, .user_ctx = nullptr };
  httpd_uri_t etat  = { .uri = "/etat",  .method = HTTP_GET, .handler = gererEtat,  .user_ctx = nullptr };
  if (httpd_start(&serveurPage, &config) == ESP_OK) {
    httpd_register_uri_handler(serveurPage, &page);
    httpd_register_uri_handler(serveurPage, &photo);
    httpd_register_uri_handler(serveurPage, &led);
    httpd_register_uri_handler(serveurPage, &tl);
    httpd_register_uri_handler(serveurPage, &cliche);
    httpd_register_uri_handler(serveurPage, &etat);
  }

  // Le flux a son propre serveur sur le port 81, pour que la page reste utilisable pendant le flux
  config.server_port = 81;
  config.ctrl_port = 32769;
  httpd_uri_t flux = { .uri = "/flux", .method = HTTP_GET, .handler = gererFlux, .user_ctx = nullptr };
  if (httpd_start(&serveurFlux, &config) == ESP_OK) {
    httpd_register_uri_handler(serveurFlux, &flux);
  }
}

// ---------- Réseau ----------
void connecterWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(NOM_RESEAU);
  WiFi.setSleep(false);  // économie d'énergie coupée : flux plus fluide
  WiFi.begin(WIFI_SSID, WIFI_MOTDEPASSE);
  Serial.print("Connexion au Wi-Fi");
  uint32_t debut = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - debut > 30000) {
      Serial.println("\nErreur : Wi-Fi injoignable, redémarrage");
      ESP.restart();
    }
    delay(250);
    Serial.print(".");
  }
  Serial.printf("\nWi-Fi connecté, signal %d dBm\n", WiFi.RSSI());

  if (MDNS.begin(NOM_RESEAU)) MDNS.addService("http", "tcp", 80);
  Serial.printf("Page : http://%s.local ou http://%s\n", NOM_RESEAU, WiFi.localIP().toString().c_str());
  Serial.printf("Flux : http://%s:81/flux\n", WiFi.localIP().toString().c_str());
}

// ---------- Programme principal ----------
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Démarrage de la caméra imprimante 3D");

  ledcSetup(CANAL_LED, 5000, 8);
  ledcAttachPin(BROCHE_LED, CANAL_LED);
  reglerLed(LED_INTENSITE_DEPART);

  if (!initCamera()) {
    delay(5000);
    ESP.restart();
  }
  initCarteSd();
  connecterWifi();
  demarrerServeurs();
}

// Le Wi-Fi se reconnecte tout seul ; s'il reste coupé plus d'une minute, on redémarre.
// Un timelapse en cours continue pendant la coupure.
void surveillerWifi() {
  static uint32_t debutCoupure = 0;
  if (WiFi.status() != WL_CONNECTED) {
    if (debutCoupure == 0) {
      debutCoupure = millis();
      Serial.println("Wi-Fi perdu");
    } else if (millis() - debutCoupure > 60000) {
      Serial.println("Wi-Fi toujours perdu, redémarrage");
      ESP.restart();
    }
  } else {
    debutCoupure = 0;
  }
}

void loop() {
  surveillerWifi();
  gererTimelapse();
  delay(20);
}
