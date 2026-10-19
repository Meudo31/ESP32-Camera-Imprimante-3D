# Caméra pour imprimante 3D

Une caméra Wi-Fi à moins de 15 € pour surveiller l'imprimante 3D depuis le téléphone, filmer des timelapses et recevoir une alerte avec photo quand l'impression est terminée. Le projet avance par étapes, du plus simple au plus complet ; chaque étape est un projet PlatformIO autonome et peut faire l'objet d'une vidéo.

| Étape | Dossier | Ce que ça fait | Carte |
| --- | --- | --- | --- |
| 1 | [etape-1-flux-video](etape-1-flux-video) | Flux vidéo en direct dans le navigateur, photo, réglage de l'éclairage | ESP32-CAM |
| 2 | [etape-2-timelapse](etape-2-timelapse) | Timelapse enregistré sur carte micro SD, à intervalle fixe ou à chaque couche | ESP32-CAM |
| 3 | [etape-3-alertes](etape-3-alertes) | Détection du début et de la fin d'impression, notifications avec photo sur le téléphone | ESP32-CAM |
| Bonus | [bonus](bonus) | Détecteur de fumée et coupure d'urgence, anneau LED, carte plus puissante | |

Les trois étapes compilent avec PlatformIO (ESP32-CAM AI-Thinker, framework Arduino) mais n'ont pas encore été testées sur une vraie carte.

## Liste d'achat par étape

Prix approximatifs ; chaque étape réutilise ce qui a été acheté avant. L'ESP32-CAM et sa carte de programmation sont les mêmes que pour la boîte aux lettres : un lot de 2 ou 3 revient moins cher.

### Étape 1 (environ 12 à 20 €)

| Pièce | Prix approx. | Où chercher |
| --- | --- | --- |
| ESP32-CAM AI-Thinker (caméra OV2640, 4 Mo de PSRAM) | 6 à 8 € | [Amazon](https://www.amazon.fr/s?k=ESP32-CAM+AI-Thinker+OV2640) · [AliExpress](https://fr.aliexpress.com/w/wholesale-esp32-cam-ai-thinker.html) |
| Carte de programmation ESP32-CAM-MB (USB) | 2 à 4 € | [Amazon](https://www.amazon.fr/s?k=ESP32-CAM-MB) · [AliExpress](https://fr.aliexpress.com/w/wholesale-esp32-cam-mb.html) |
| Caméra OV2640 grand angle 160°, nappe 75 mm (conseillée : on voit tout le plateau de près) | 3 à 5 € | [Amazon](https://www.amazon.fr/s?k=OV2640+160+degr%C3%A9s+75mm) · [AliExpress](https://fr.aliexpress.com/w/wholesale-ov2640-160-degree-75mm.html) |
| Bloc secteur USB 5 V 2 A + câble micro USB | 5 € | souvent déjà dans un tiroir |
| Ou : convertisseur abaisseur 24 V vers 5 V (MP1584 ou LM2596) pour s'alimenter sur l'imprimante | 1 à 2 € | [Amazon](https://www.amazon.fr/s?k=MP1584+convertisseur) · [AliExpress](https://fr.aliexpress.com/w/wholesale-mp1584-buck.html) |
| Boîtier et support imprimés en 3D | 0 € | modèles gratuits sur Printables, recherche « ESP32-CAM mount » |

### Étape 2 (environ 5 € en plus)

| Pièce | Prix approx. | Où chercher |
| --- | --- | --- |
| Carte micro SD de 32 Go maximum, formatée en FAT32 | 5 € | [Amazon](https://www.amazon.fr/s?k=carte+micro+SD+32+Go) |

### Étape 3 (0 € en plus)

L'appli **ntfy** est gratuite (Android et iPhone).

### Bonus

| Pièce | Prix approx. | Où chercher |
| --- | --- | --- |
| Capteur de fumée et gaz MQ-2 (module avec sortie numérique) | 2 € | [Amazon](https://www.amazon.fr/s?k=MQ-2+module) · [AliExpress](https://fr.aliexpress.com/w/wholesale-mq-2-module.html) |
| Prise connectée Wi-Fi pilotable en local (Shelly Plug S) | 20 à 25 € | [Amazon](https://www.amazon.fr/s?k=Shelly+Plug+S) |
| Anneau NeoPixel WS2812B 12 ou 16 LED | 3 à 5 € | [Amazon](https://www.amazon.fr/s?k=anneau+WS2812B+16+LED) · [AliExpress](https://fr.aliexpress.com/w/wholesale-ws2812b-ring-16.html) |
| Carte plus puissante : Freenove ESP32-S3 WROOM CAM ou Seeed XIAO ESP32S3 Sense | 12 à 18 € | [Amazon](https://www.amazon.fr/s?k=ESP32-S3+CAM) |

## Mise en route

Chaque dossier a son propre README avec le câblage et la mise en route. Dans tous les cas : copier `include/config.example.h` en `include/config.h`, remplir le Wi-Fi, puis `pio run -t upload` depuis le dossier de l'étape avec la carte posée sur l'ESP32-CAM-MB. Le fichier `config.h` est exclu du dépôt.

Une fois la carte flashée, la page est sur **http://camera-imprimante.local** (ou sur l'adresse IP affichée dans `pio device monitor`).
