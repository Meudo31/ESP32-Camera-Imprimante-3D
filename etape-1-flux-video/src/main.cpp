// Caméra pour imprimante 3D - ESP32-CAM (AI-Thinker)
// Étape 1 : surveiller l'impression en direct depuis le navigateur
//
// Fonctionnement :
//  1. L'ESP32-CAM se connecte au Wi-Fi et s'annonce sous le nom http://camera-imprimante.local
//  2. La page web (port 80) affiche le flux vidéo, un bouton photo et le réglage de la LED.
//  3. Le flux MJPEG brut est sur http://camera-imprimante.local:81/flux : il s'ouvre aussi
//     dans VLC, OctoPrint, Mainsail/Fluidd ou Home Assistant.
//
// Alimentation : 5 V sur la broche 5V, la caméra reste branchée en permanence.

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
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

httpd_handle_t serveurPage = nullptr;
httpd_handle_t serveurFlux = nullptr;
uint8_t ledIntensite = LED_INTENSITE_DEPART;

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
</style></head><body>
<h2>Caméra imprimante 3D</h2>
<img id="flux" alt="Flux vidéo">
<div class="ligne">
  <a class="bouton" href="/photo" target="_blank">Photo</a>
  <label>Éclairage <input id="led" type="range" min="0" max="255" value="%LED%"></label>
</div>
<script>
document.getElementById('flux').src = location.protocol + '//' + location.hostname + ':81/flux';
document.getElementById('led').onchange = e => fetch('/led?val=' + e.target.value);
</script>
</body></html>)rawliteral";

esp_err_t gererPage(httpd_req_t* req) {
  String page = PAGE;
  page.replace("%LED%", String(ledIntensite));
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
  if (httpd_start(&serveurPage, &config) == ESP_OK) {
    httpd_register_uri_handler(serveurPage, &page);
    httpd_register_uri_handler(serveurPage, &photo);
    httpd_register_uri_handler(serveurPage, &led);
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
  connecterWifi();
  demarrerServeurs();
}

void loop() {
  // Le Wi-Fi se reconnecte tout seul ; s'il reste coupé plus d'une minute, on redémarre.
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
  delay(1000);
}
