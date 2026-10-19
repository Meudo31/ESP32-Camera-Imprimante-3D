# Bonus

Ces améliorations seront codées après les étapes 1 à 3. Voici comment on les fera.

## Détecteur de fumée et coupure d'urgence

Le but : couper l'imprimante et prévenir le téléphone si ça sent le brûlé.

- Un module **MQ-2** (fumée et gaz) se branche sur **GPIO13**, la seule broche libre et sans piège avec la carte SD en mode 1 bit. On utilise sa sortie numérique DO, réglée avec le potentiomètre du module. Le MQ-2 est alimenté en 5 V et sa sortie monte à 5 V : on la ramène à 3,3 V avec un pont diviseur (10 kΩ + 20 kΩ).
- L'imprimante est branchée sur une **prise connectée Shelly Plug S**, que l'ESP32 éteint par le Wi-Fi avec une simple adresse web (`http://IP-de-la-prise/relay/0?turn=off`), sans passer par Internet. Aucun fil 230 V à toucher.
- Le MQ-2 chauffe environ une minute au démarrage avant de donner des mesures fiables : le code ignorera ses alertes pendant ce temps.
- Ça reste un gadget : un **vrai détecteur de fumée certifié** au-dessus de l'imprimante est indispensable, surtout si elle tourne la nuit.

## Anneau LED d'éclairage

La LED de l'ESP32-CAM éclaire fort au centre et pas du tout sur les bords. Un **anneau NeoPixel de 12 ou 16 LED** autour de l'objectif donne une lumière douce et régulière, et il peut changer de couleur selon l'état : bleu au repos, blanc pendant l'impression, vert quand c'est fini, rouge en cas d'alerte. Il se branche sur GPIO13 (donc pas en même temps que le MQ-2) et sur le 5 V.

## Carte plus puissante

L'ESP32-CAM plafonne vers 10 à 15 images par seconde en SVGA. Les cartes **ESP32-S3** (Freenove ESP32-S3 WROOM CAM, Seeed XIAO ESP32S3 Sense) ont un processeur plus rapide, 8 Mo de PSRAM et un vrai port USB : flux plus fluide en HD, plus de broches libres, pas de carte de programmation. Il faudra changer le brochage de la caméra dans le code et la carte dans `platformio.ini`.
