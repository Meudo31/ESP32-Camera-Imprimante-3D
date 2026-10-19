# Étape 1 : surveiller l'impression en direct

L'ESP32-CAM se branche à côté de l'imprimante et diffuse un flux vidéo sur le Wi-Fi. On l'ouvre depuis le téléphone ou le PC sur **http://camera-imprimante.local** : flux en direct, bouton photo et réglage de la LED d'éclairage.

## Matériel

| Pièce | Rôle | Prix approx. |
| --- | --- | --- |
| ESP32-CAM AI-Thinker (caméra OV2640) | Cerveau + caméra + LED | 6 à 8 € |
| Carte de programmation ESP32-CAM-MB | Flasher et alimenter la carte, qui n'a pas d'USB | 2 à 4 € |
| Caméra OV2640 grand angle 160°, nappe 75 mm (optionnelle) | Voir tout le plateau de près | 3 à 5 € |
| Bloc USB 5 V 2 A, ou convertisseur 24 V vers 5 V | Alimentation permanente | 1 à 5 € |

## Câblage

Le plus simple : laisser la carte posée sur l'ESP32-CAM-MB et brancher un bloc USB 5 V de 2 A. Rien d'autre à câbler.

Pour s'alimenter sur l'alimentation de l'imprimante (24 V sur la plupart des machines récentes, 12 V sur les anciennes) :

| De | Vers |
| --- | --- |
| Sortie 24 V de l'alimentation de l'imprimante | Entrée IN+ / IN- du convertisseur MP1584 |
| Sortie OUT+ du convertisseur, **réglée à 5,0 V au multimètre avant de brancher** | Broche **5V** de l'ESP32-CAM |
| Sortie OUT- | GND |

**Attention** : l'alimentation de l'imprimante est reliée au 230 V. Débranche l'imprimante avant d'ouvrir quoi que ce soit, et ne touche qu'au bornier basse tension.

Pour changer la caméra contre le modèle grand angle : soulève le petit volet noir du connecteur, retire la nappe, glisse la nouvelle (contacts vers la carte) et rabats le volet.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h`, puis remplis le Wi-Fi.
2. Pose la carte sur l'ESP32-CAM-MB, branche-la au PC et lance `pio run -t upload`, puis `pio device monitor`.
3. Le moniteur affiche l'adresse de la page. Ouvre **http://camera-imprimante.local** (ou l'adresse IP si ton téléphone Android ne connaît pas les adresses `.local`).
4. Règle la mise au point si l'image est floue : l'objectif se dévisse doucement à la main (retire d'abord la goutte de colle avec un cutter). Un quart de tour suffit souvent pour une prise de vue à 20-30 cm.

## Réglages (`config.h`)

- `TAILLE_IMAGE` : SVGA (800x600) par défaut, un bon compromis. En HD ou UXGA l'image est plus fine mais le flux descend à quelques images par seconde.
- `RETOURNER_VERTICAL`, `MIROIR_HORIZONTAL` : si la caméra est montée à l'envers.
- `IMAGES_PAR_SECONDE_MAX` : limite le flux pour ne pas saturer le Wi-Fi.

## Bon à savoir

- **Un seul spectateur à la fois pour le flux.** Un deuxième onglet reste noir tant que le premier est ouvert. Les photos, elles, marchent toujours.
- Le flux brut est sur `http://camera-imprimante.local:81/flux` et la photo sur `http://camera-imprimante.local/photo`. Ce sont les deux adresses à donner à **Mainsail / Fluidd** (type de webcam « MJPEG-Streamer »), **OctoPrint** (Stream URL et Snapshot URL) ou **Home Assistant** (caméra MJPEG).
- Si la carte redémarre en boucle avec « Brownout detector was triggered », l'alimentation est trop faible : change de câble ou de bloc USB.
- La carte chauffe un peu en continu, c'est normal. Évite de la coller contre le plateau chauffant.
