# Étape 3 : les alertes sur le téléphone

Tout ce que font les étapes 1 et 2, plus des notifications avec photo sur le téléphone :

- **« Impression démarrée »** quand la caméra voit l'imprimante se mettre à bouger ;
- **une photo de suivi** toutes les 30 minutes pendant l'impression, pour vérifier de loin que rien n'est parti en spaghetti ;
- **« Impression terminée »** quand plus rien ne bouge depuis 5 minutes, avec la photo de la pièce finie et la durée ;
- un bouton **Photo sur le téléphone** sur la page.

Le timelapse démarre et s'arrête tout seul avec l'impression (`TIMELAPSE_AUTO`). Toucher une notification ouvre la page de la caméra quand on est sur le même Wi-Fi.

## Comment la caméra sait que l'imprimante tourne

Toutes les 10 secondes, elle réduit la photo à 100x75 pixels en niveaux de gris et la compare avec la précédente. Si plus de 1 % des pixels ont changé, il y a du mouvement. Trois analyses de suite avec du mouvement : l'impression a démarré. Cinq minutes sans aucun mouvement : elle est terminée.

Pas besoin de câbler l'imprimante ni d'installer quoi que ce soit dessus : ça marche avec n'importe quelle machine. La ligne d'état de la page affiche le pourcentage de mouvement mesuré, pratique pour régler les seuils.

## Matériel

Le même qu'à l'étape 2. Rien à acheter en plus.

## Mise en route

1. Installe l'appli **ntfy** sur ton téléphone et abonne-toi à un sujet au nom long et impossible à deviner (exemple : `imprimante-meudo-4h8k2z9q`). Sur le serveur public ntfy.sh, n'importe qui connaissant ce nom peut voir les photos.
2. Copie `include/config.example.h` en `include/config.h`, puis remplis le Wi-Fi et le sujet ntfy.
3. `pio run -t upload` puis `pio device monitor`. Au démarrage, tu dois recevoir « Camera en service ».
4. Lance une impression : « Impression démarrée » arrive au bout de 30 secondes environ.

## Réglages (`config.h`)

- `SEUIL_MOUVEMENT_POURMILLE` : 10 = 1 % des pixels. Si l'impression est annoncée alors que rien ne bouge (lumière qui change, reflets), monte à 20 ou 30. Si elle n'est jamais détectée (petite pièce loin de la caméra), descends à 5.
- `IMMOBILE_FIN_MIN` : 5 minutes par défaut. Augmente-le si tes impressions font de longues pauses (changement de filament, chauffe du plateau en cours de route).
- `PHOTO_SUIVI_MIN` : 0 pour ne recevoir que le début et la fin.
- `TIMELAPSE_AUTO` : 0 pour garder la main sur le timelapse.

## Bon à savoir

- **Cadre serré sur le plateau.** Une personne qui passe devant la caméra ou une lampe qu'on allume peut faire croire à un démarrage. C'est sans conséquence (au pire une notification de trop), mais un cadrage serré limite les fausses alertes.
- Changer l'intensité de la LED fait aussi « bouger » l'image : évite de jouer avec pendant la détection.
- « Impression terminée » veut dire « plus rien ne bouge ». Une impression arrêtée net (plantage, buse bouchée qui fige la machine) envoie la même alerte : c'est justement utile.
- Cette caméra ne remplace pas un détecteur de fumée. Voir le bonus pour aller plus loin.
