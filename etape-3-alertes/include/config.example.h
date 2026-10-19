// Copier ce fichier en config.h et remplir les valeurs.
// config.h ne doit jamais être publié (il contient le mot de passe Wi-Fi).
#pragma once

#define WIFI_SSID       "NomDeTonWifi"
#define WIFI_MOTDEPASSE "MotDePasseWifi"

// Nom sur le réseau : la page sera accessible sur http://camera-imprimante.local
#define NOM_RESEAU      "camera-imprimante"

// Taille de l'image : FRAMESIZE_VGA (640x480), FRAMESIZE_SVGA (800x600),
// FRAMESIZE_XGA (1024x768), FRAMESIZE_HD (1280x720), FRAMESIZE_UXGA (1600x1200).
// Le timelapse utilise la même taille que le flux. HD donne une belle vidéo 16:9.
#define TAILLE_IMAGE    FRAMESIZE_HD

// Qualité JPEG : 10 = meilleure qualité, 63 = pire. Entre 10 et 15, c'est bien.
#define QUALITE_JPEG    12

// Limite d'images par seconde du flux, pour ne pas saturer le Wi-Fi.
#define IMAGES_PAR_SECONDE_MAX 15

// Retourner l'image si la caméra est montée à l'envers (1 = oui, 0 = non).
#define RETOURNER_VERTICAL   0
#define MIROIR_HORIZONTAL    0

// Intensité de la LED d'éclairage au démarrage (0 = éteinte, 255 = à fond).
#define LED_INTENSITE_DEPART 0

// Intervalle par défaut entre deux images du timelapse, en secondes (modifiable sur la page).
// Une impression de 2 h à 10 s = 720 images = 24 s de vidéo à 30 images/s.
#define TIMELAPSE_INTERVALLE_S 10

// ---------- Alertes sur le téléphone (appli ntfy) ----------
// Sujet ntfy : n'importe qui connaissant ce nom peut voir tes photos.
// Choisis un nom long et impossible à deviner, par exemple : imprimante-meudo-4h8k2z9q
#define NTFY_SUJET     "imprimante-change-moi-xxxxxxxx"
#define NTFY_SERVEUR   "https://ntfy.sh"

// Détection de l'impression par comparaison d'images : la caméra compare une petite
// image toutes les ANALYSE_PERIODE_S secondes avec la précédente.
#define ANALYSE_PERIODE_S        10
// Écart de luminosité (0 à 255) à partir duquel un pixel est considéré comme changé.
#define SEUIL_PIXEL              25
// Part de pixels changés, en pour mille, pour parler de mouvement (10 = 1 %).
#define SEUIL_MOUVEMENT_POURMILLE 10
// Nombre d'analyses de suite avec mouvement avant d'annoncer « impression démarrée ».
#define ANALYSES_POUR_DEMARRAGE  3
// Minutes sans aucun mouvement avant d'annoncer « impression terminée ».
#define IMMOBILE_FIN_MIN         5
// Photo de suivi envoyée sur le téléphone pendant l'impression, en minutes (0 = jamais).
#define PHOTO_SUIVI_MIN          30
// Démarrer et arrêter le timelapse tout seul avec l'impression (1 = oui, 0 = non).
#define TIMELAPSE_AUTO           1
