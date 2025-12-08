# mirega-bmp390-test – Test technique C++ capteurs

## 1. Résumé

Ce dépôt contient le résultat d’un test technique autour de capteurs environnementaux sur une plateforme type Zynq/Ubuntu :

- **Partie 1** : une petite librairie C++ de haut niveau pour le capteur **BMP390**, construite au‑dessus du driver C officiel **Bosch BMP3_SensorAPI**.
- **Partie 2** : une **architecture multi-capteurs** (BMP390 + HDC3022) décrite et illustrée en C++ (pseudo-code) pour la lecture périodique, le logging, le calcul d’une température moyenne et la gestion d’alarme.

Le code ne vise pas une compilabilité immédiate sur une cible donnée, mais une **structure claire, modulaire et expliquée**.

---

## 2. Organisation du dépôt

- bmp390-lib  
  - `include/bmp390/bmp390_driver.hpp` : interface C++ haut niveau pour le BMP390 (`bmp390::Bmp390`).  
  - `src/bmp390_driver.cpp` : implémentation C++ s’appuyant sur Bosch BMP3_SensorAPI.  
  - `src/third_party/` : fichiers du driver Bosch (`bmp3.c`, bmp3.h, bmp3_defs.h).  
  - `examples/bmp390_example.cpp` : exemple minimal d’utilisation de la librairie BMP390.  
  - `docs/README.md` : documentation dédiée à la librairie BMP390.

- examples  
  - `multisensor_example.cpp` : exemple de gestion multi-capteurs basé sur l’architecture décrite.

- docs  
  - `ARCHITECTURE_MULTISENSOR.md` : description de l’architecture multi-capteurs (interface `ISensor`, classes concrètes, boucle principale).  
  - `NOTES.md` : notes globales sur les objectifs, contraintes, TODOs.

- Fichiers racine :  
  - README.md : ce document.  
  - TIMELOG.md : temps passé sur les différentes tâches.

---

## 3. Partie 1 – Librairie BMP390

La première partie consiste à encapsuler le driver C Bosch **BMP3_SensorAPI** dans une **API C++ simple** :

- Utilisation directe des fichiers Bosch (`bmp3.c`, bmp3.h, bmp3_defs.h) intégrés dans third_party.
- Classe principale : `bmp390::Bmp390` :
  - se construit avec un `dev_id`, un `BusInterface` (callbacks de lecture/écriture bus + délai), et un booléen pour choisir I2C ou SPI ;
  - `init()` configure la structure interne `bmp3_dev` et appelle `bmp3_init` ;
  - `configure(const Config&)` mappe une configuration C++ (oversampling, ODR, filtre IIR) vers les macros Bosch (`BMP3_OVERSAMPLING_*`, `BMP3_ODR_*`, `BMP3_IIR_FILTER_*`) et appelle `bmp3_set_sensor_settings` / `bmp3_set_op_mode` ;
  - `read_measurement(Measurement&)` encapsule `bmp3_get_sensor_data` et retourne pression (Pa) + température (°C).

L’exemple bmp390_example.cpp montre un flux minimal :

1. Création d’un `BusInterface` avec des stubs I2C.  
2. Création d’un `bmp390::Bmp390`.  
3. Appels à `init()`, `configure()`, `read_measurement()`.  
4. Affichage de la pression et de la température.

---

## 4. Partie 2 – Architecture multi-capteurs

> Remarque : le capteur HDC3022 est illustré en pseudo-code (structure de classe prête,
> mais accès I2C et formules de conversion à implémenter dans un contexte réel).

La deuxième partie propose une **architecture générique** pour gérer plusieurs capteurs (BMP390 + HDC3022) :

- Interface abstraite `ISensor` (décrite dans ARCHITECTURE_MULTISENSOR.md) :
  - destructeur virtuel,
  - `update()` pour rafraîchir les données internes,
  - `double getTemperatureC() const` (retourne `NaN` si le capteur ne fournit pas de température),
  - `void log(std::ostream&) const` pour tracer l’état courant.

- Implémentations concrètes :
  - `Bmp390Sensor` : encapsule un `bmp390::Bmp390`, stocke la dernière mesure pression + température, implémente `update()`, `getTemperatureC()` et `log()`.  
  - `Hdc3022Sensor` : pseudo-code représentant un capteur température + humidité ; les accès I2C et conversions sont laissés en TODO (structure prête pour une implémentation réelle).

- Gestion des capteurs :
  - Un `std::vector<std::unique_ptr<ISensor>> sensors` contient tous les capteurs (BMP390, HDC3022, etc.).  
  - Une fonction `setupSensors()` crée les instances et les ajoute au vecteur.

- Boucle principale (`mainLoop()`), implémentée en pseudo-code dans `examples/multisensor_example.cpp` :
  - pour chaque capteur : `update()` puis `log()` ;  
  - récupération des températures valides pour calculer une moyenne globale ;  
  - si au moins une température dépasse 30 °C, appel de `raiseAlarm(max_temp)` (par ex. affichage d’un message).

Cette partie met l’accent sur la **séparation des responsabilités** (capteur vs orchestration), l’extensibilité (ajout facile de nouveaux capteurs) et la lisibilité.

---

## 5. Limitations et pistes d’amélioration

- **Intégration matérielle incomplète** :
  - Les callbacks I2C/SPI sont fournis sous forme de stubs (`TODO`) ; il reste à les connecter aux drivers réels de la plateforme (ex. `/dev/i2c-*` sous Linux, HAL sur MCU).
  - Le capteur HDC3022 est uniquement décrit en pseudo-code : l’implémentation réelle (adresse, registres, formules du datasheet) reste à faire.

- **Mappings enums → macros Bosch** :
  - Les enums C++ de configuration (`Config::Oversampling`, `OutputDataRate`, `IirFilterCoeff`) sont mappés via des `switch` vers les macros Bosch.  
  - Ces correspondances semblent cohérentes mais méritent une **validation** plus poussée et des tests.

- **Gestion d’erreurs minimale** :
  - Les fonctions retournent essentiellement les codes d’erreur du driver Bosch.  
  - On pourrait introduire une couche d’erreurs C++ plus lisible (enum, exceptions ou wrapper de statut) et un logging plus riche.

- **Tests et CI** :
  - Il n’y a pas encore de **tests unitaires** ni d’intégration continue.  
  - Des tests pourraient couvrir : mapping de configuration, gestion des erreurs bus, comportement de la boucle multi-capteurs sur des capteurs simulés/mockés.

- **Évolution possible de l’architecture** :
  - Passage à un modèle multi-threads ou basé sur un scheduler/RTOS si les contraintes temps réel l’exigent.  
  - Utilisation d’un pattern observateur (publisher/subscriber) pour les alarmes et le logging.

---

## 6. Temps passé

Le détail du temps passé par tâche est consigné dans TIMELOG.md.  
Environ **8 heures** ont été consacrées au total, réparties entre :

- conception et implémentation de la librairie BMP390,  
- réflexion et rédaction de l’architecture multi-capteurs,  
- écriture des exemples et de la documentation associée.

## 7. Utilisation de l’IA générative

Ce projet a été réalisé **avec l’aide d’outils d’IA générative**, principalement :

- **GitHub Copilot / Copilot Chat** (VS Code, interface GitHub),
- un assistant de type ChatGPT pour la réflexion et la structuration du travail.

Rôle de ces outils :

- génération de **squelettes de code** (par ex. wrapper C++ autour du driver Bosch, exemples `main`),
- assistance pour la **rédaction de documentation** (README, notes d’architecture),
- aide à la **mise en forme** (noms de fichiers, organisation des dossiers, structuration des sections).

Rôle du développeur :

- choix de l’**architecture globale** (organisation de la librairie, interface `ISensor`, design multi-capteurs),
- **validation / adaptation** du code généré (simplifications, commentaires, TODO, limitations),
- rédaction et ajustement du **TIMELOG** et des **notes** pour refléter le temps passé, les compromis et les pistes d’amélioration.

L’objectif n’était pas de cacher l’usage de l’IA, mais au contraire de l’utiliser comme un outil de productivité, en gardant la responsabilité humaine sur :
- la compréhension du sujet,
- les décisions techniques,
- la cohérence du livrable.
