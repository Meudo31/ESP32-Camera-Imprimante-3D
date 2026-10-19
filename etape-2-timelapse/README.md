# Étape 2 : le timelapse de l'impression

Tout ce que fait l'étape 1, plus un timelapse enregistré sur la carte micro SD. Un bouton de la page le démarre, un autre l'arrête ; les images s'assemblent ensuite en vidéo sur le PC.

## Matériel

Celui de l'étape 1, plus une **carte micro SD de 32 Go maximum formatée en FAT32**, à glisser dans le lecteur de l'ESP32-CAM. Les cartes de 64 Go et plus sont en exFAT : il faut les reformater en FAT32 (avec l'outil gratuit « guiformat » sous Windows).

Pas de câblage en plus. La carte SD fonctionne en mode 1 bit pour laisser la LED (GPIO4) libre.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h`, puis remplis le Wi-Fi.
2. Glisse la carte SD, puis `pio run -t upload` et `pio device monitor`. Le moniteur doit afficher « Carte SD : xxx Mo libres ».
3. Sur la page, choisis l'intervalle et clique **Démarrer le timelapse**. La ligne d'état compte les images.
4. À la fin de l'impression, clique **Arrêter**. Chaque timelapse a son dossier : `/timelapse/0001`, `/timelapse/0002`...

## Assembler la vidéo

Retire la carte SD, mets-la dans le PC, ouvre un terminal dans le dossier du timelapse et lance :

```
ffmpeg -framerate 30 -i img_%05d.jpg -c:v libx264 -pix_fmt yuv420p -crf 18 timelapse.mp4
```

`-framerate 30` donne 30 images par seconde : 720 photos font 24 secondes de vidéo. ffmpeg est gratuit (ffmpeg.org) ; DaVinci Resolve ou Shotcut savent aussi importer une suite d'images.

## Choisir l'intervalle

| Durée de l'impression | Intervalle | Vidéo obtenue (30 i/s) |
| --- | --- | --- |
| 1 h | 5 s | 24 s |
| 3 h | 10 s | 36 s |
| 8 h | 30 s | 32 s |

Une image en HD pèse environ 100 à 150 Ko : une carte de 32 Go en contient plus de 200 000.

## Le timelapse « une image par couche »

Avec un intervalle fixe, la tête bouge d'une image à l'autre et la vidéo tremble. Les beaux timelapses prennent une photo à chaque changement de couche, tête rangée sur le côté. Pour ça :

1. Démarre le timelapse avec un intervalle de **0** : la caméra n'enregistre plus rien toute seule.
2. Fais appeler l'adresse **http://camera-imprimante.local/cliche** par l'imprimante à chaque couche, juste après avoir rangé la tête :
   - **Klipper** : macro de changement de couche avec l'extension `gcode_shell_command` qui lance `curl -s http://camera-imprimante.local/cliche`.
   - **OctoPrint** : plugin « Octolapse » ou « Gcode System Commands » avec la même commande `curl`.
   - Dans le trancheur (Cura, PrusaSlicer, OrcaSlicer), le G-code de changement de couche range la tête (`G1 X... Y...`) et fait une petite pause (`G4 P500`) le temps de la photo.

Sans Klipper ni OctoPrint, l'imprimante ne sait pas appeler une adresse web : reste en intervalle fixe.

## Bon à savoir

- Le timelapse continue même si personne ne regarde la page et même si le Wi-Fi coupe.
- Si la carte est pleine ou retirée, le timelapse s'arrête tout seul (message dans le moniteur).
- Allume la LED à faible intensité si la pièce est sombre : l'exposition change sinon entre le jour et la nuit et la vidéo clignote.
