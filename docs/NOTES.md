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
