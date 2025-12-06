# NOTES – Test BMP390

## Objectifs du test

### Partie 1 – Librairie BMP390
- Adapter le driver Bosch BMP3_SensorAPI pour créer une librairie standalone BMP390.
- Fournir une API simple pour initialiser le capteur et lire pression et température.
- Documenter l’intégration (README) et éventuellement fournir un exemple d’utilisation.

### Partie 2 – Gestion multi-capteurs (BMP390 + HDC3022)
- Proposer une architecture permettant de gérer plusieurs capteurs.
- Boucle principale qui :
  - Lit et logge les données de chaque capteur.
  - Calcule la température moyenne.
  - Déclenche une alarme si température > 30°C.

## Contraintes
- Temps total : environ 2 jours.
- Le code n’a pas nécessairement besoin de compiler sur la cible.
- L’important : structure propre, lisibilité, et explication des choix.

## TODO à affiner plus tard
- Liste précise des fichiers Bosch utilisés.
- Décisions d’architecture prises pour la lib.
- Points à faire si plus de temps.

## Driver Bosch BMP3_SensorAPI – éléments importants

Fichiers critiques pour une utilisation minimale (pression + température) :
- `bmp3_defs.h` : définitions de registres, constantes, structures (`bmp3_dev`, `bmp3_settings`, `bmp3_data`, etc.).
- `bmp3.h` : déclarations de l’API (init, configuration, lecture de données).
- `bmp3.c` : implémentation de l’API.

Callbacks à fournir côté plateforme :
- `bmp3_read_fptr_t` : lecture I2C/SPI.
- `bmp3_write_fptr_t` : écriture I2C/SPI.
- `bmp3_delay_fptr_t` : fonction de délai.

Workflow minimal pour lire pression + température :
1. Remplir `struct bmp3_dev` (dev_id, type d’interface, callbacks).
2. Appeler `bmp3_init(&dev)` (+ éventuellement `bmp3_soft_reset(&dev)`).
3. Configurer `struct bmp3_settings` et appeler  
   `bmp3_set_sensor_settings(desired_settings, &settings, &dev)`.
4. Choisir le mode (`settings.op_mode`, typiquement `BMP3_MODE_FORCED` ou `BMP3_MODE_NORMAL`) et appeler  
   `bmp3_set_op_mode(&settings, &dev)`.
5. Lire les mesures avec  
   `bmp3_get_sensor_data(BMP3_PRESS_TEMP, &data, &dev)`  
   puis utiliser `data.pressure` et `data.temperature`.
