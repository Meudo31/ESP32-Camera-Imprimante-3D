// Copier ce fichier en config.h et remplir les valeurs.
// config.h ne doit jamais être publié (il contient le mot de passe Wi-Fi).
#pragma once

#define WIFI_SSID       "NomDeTonWifi"
#define WIFI_MOTDEPASSE "MotDePasseWifi"

// Nom sur le réseau : la page sera accessible sur http://camera-imprimante.local
#define NOM_RESEAU      "camera-imprimante"

// Taille de l'image : FRAMESIZE_VGA (640x480), FRAMESIZE_SVGA (800x600),
// FRAMESIZE_XGA (1024x768), FRAMESIZE_HD (1280x720), FRAMESIZE_UXGA (1600x1200).
// Plus c'est grand, moins il y a d'images par seconde.
#define TAILLE_IMAGE    FRAMESIZE_SVGA

// Qualité JPEG : 10 = meilleure qualité, 63 = pire. Entre 10 et 15, c'est bien.
#define QUALITE_JPEG    12

// Limite d'images par seconde du flux, pour ne pas saturer le Wi-Fi.
#define IMAGES_PAR_SECONDE_MAX 15

// Retourner l'image si la caméra est montée à l'envers (1 = oui, 0 = non).
#define RETOURNER_VERTICAL   0
#define MIROIR_HORIZONTAL    0

// Intensité de la LED d'éclairage au démarrage (0 = éteinte, 255 = à fond).
#define LED_INTENSITE_DEPART 0
