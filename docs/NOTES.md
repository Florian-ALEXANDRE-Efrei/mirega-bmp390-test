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

## Design de la librairie C++ BMP390

- Lib C++ de plus haut niveau au-dessus du driver C Bosch (BMP3_SensorAPI).
- Fichiers principaux :
  - `bmp390-lib/include/bmp390/bmp390_driver.hpp` : interface C++.
  - `bmp390-lib/src/bmp390_driver.cpp` : implémentation de l’interface.
  - `bmp390-lib/src/third_party/bmp3.c/h/defs.h` : driver Bosch inchangé.

### Principes retenus

- Abstraction de bus `BusInterface` :
  - l’application fournit des callbacks `read`, `write`, `delay_us`.
  - permet d’isoler la dépendance à la plateforme (I2C/SPI Linux, HAL STM32, etc.).
- Classe `bmp390::Bmp390` :
  - encapsule la structure `bmp3_dev` du driver Bosch.
  - fournit trois méthodes principales :
    - `init()` : initialise le capteur (bmp3_init).
    - `configure(const Config&)` : configure oversampling/ODR/filtre + mode (bmp3_set_sensor_settings / bmp3_set_op_mode).
    - `read_measurement(Measurement&)` : lit pression + température (bmp3_get_sensor_data).
- `Config` :
  - expose des enums C++ simples (Oversampling, OutputDataRate, IirFilterCoeff),
  - mappées en interne vers les macros du driver Bosch.
- `Measurement` :
  - expose directement des unités physiques : `pressure_pa` (Pa), `temperature_c` (°C).

### Points à vérifier / limites

- Mapping exact `Config::Oversampling::X1` → macro Bosch (`BMP3_NO_OVERSAMPLING` vs `BMP3_OVERSAMPLING_1X`).
- Hypothèse actuelle sur les unités si la compensation entière est utilisée (TODO dans le code).
- Pas encore de gestion détaillée des codes d’erreur, on relaie directement les retours Bosch.

## État de la Partie 1 (librairie BMP390)

- Wrapper C++ fonctionnel autour du driver Bosch :
  - Architecture claire (BusInterface, Bmp390, Config, Measurement).
  - Driver Bosch inchangé en third_party.
- README dédié à la librairie + exemple d’utilisation.
- Limitations connues :
  - Mapping exact des enums à valider.
  - Callbacks I2C/SPI encore en pseudo-code (stubs).

## État de la Partie 2 (multi-capteurs)

- Document d’architecture : `docs/ARCHITECTURE_MULTISENSOR.md`
  - Interface générique `ISensor`.
  - Implémentations Bmp390Sensor et Hdc3022Sensor (pseudo-code).
  - Boucle principale monothread avec calcul de température moyenne et alarme > 30 °C.
- Exemple de pseudo-implémentation : `examples/multisensor_example.cpp`.

Limitations:
- Hdc3022Sensor uniquement en pseudo-code (I2C et formules à implémenter).
- Pas de vraie gestion de threads ou de scheduler, architecture centrée sur une boucle simple.
