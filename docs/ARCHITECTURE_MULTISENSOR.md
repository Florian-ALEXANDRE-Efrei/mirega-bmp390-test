> Document rédigé dans le cadre du test technique (Partie 2 – gestion multi-capteurs BMP390 + HDC3022).
> Il décrit une architecture C++ proposée, sans garantie de compilabilité immédiate.

# Architecture multi-capteurs (BMP390 + HDC3022)

## 1. Objectif général

L’objectif de ce module est de proposer une **architecture C++ simple et lisible** pour gérer plusieurs capteurs environnementaux sur une plateforme type **Zynq/Ubuntu** :

- Capteurs pris en charge :
  - `BMP390` : pression + température (via la librairie C++ existante `bmp390::Bmp390` dans bmp390-lib),
  - `HDC3022` : température + humidité (restera au niveau pseudo-code).
- Comportement attendu :
  - Lecture périodique de l’ensemble des capteurs dans une **boucle principale**,
  - Logging des données de chaque capteur,
  - Calcul d’une **température moyenne globale** à partir de tous les capteurs qui fournissent une température,
  - Déclenchement d’une **alarme** si la température de **n’importe quel capteur** dépasse `30.0 °C`.

L’accent est mis sur la **structure** et la **séparation des responsabilités**, plus que sur la compilabilité immédiate.

---

## 2. Interface abstraite `ISensor`

On définit une interface abstraite commune à tous les capteurs, `ISensor`, qui permet :

- d’unifier les opérations de base (mise à jour des données, logging),
- de récupérer une température quand le capteur en fournit une (BMP390, HDC3022),
- de manipuler les capteurs via des pointeurs polymorphes (`std::unique_ptr<ISensor>`).

### 2.1 Responsabilités de `ISensor`

- **Destructeur virtuel** pour un usage polymorphique sécurisé.
- **`update()`** : rafraîchir les données internes à partir du hardware.
- **`getTemperatureC() const`** : retourne la température en degrés Celsius, ou `NaN` si le capteur ne fournit pas de température.
- **`log(std::ostream& os) const`** : écrit un résumé des dernières mesures dans un flux de sortie.

### 2.2 Pseudo-code C++ pour `ISensor`

```cpp
#include <ostream>
#include <cmath>    // pour std::nan

class ISensor
{
public:
    virtual ~ISensor() = default;

    /// @brief Met à jour les données internes du capteur (lecture hardware).
    virtual void update() = 0;

    /// @brief Retourne la température en °C, ou NaN si non disponible.
    virtual double getTemperatureC() const
    {
        return std::nan(""); // Par défaut : pas de température
    }

    /// @brief Logge les données courantes sur le flux donné.
    virtual void log(std::ostream& os) const = 0;
};
```

---

## 3. Implémentations concrètes de `ISensor`

### 3.1 `Bmp390Sensor` (pression + température)

`Bmp390Sensor` encapsule la librairie existante `bmp390::Bmp390`.  
Responsabilités :

- Gérer l’objet `bmp390::Bmp390` (construction, init, configure),
- Stocker la dernière mesure (pression + température),
- Fournir `getTemperatureC()` et un `log()` adapté.

#### 3.1.1 Pseudo-code C++ pour `Bmp390Sensor`

```cpp
#include <memory>
#include <iostream>
#include "bmp390/bmp390_driver.hpp"

class Bmp390Sensor : public ISensor
{
public:
    Bmp390Sensor(const bmp390::BusInterface& bus, uint8_t i2c_addr)
        : bmp_(i2c_addr, bus, /*use_i2c=*/true)
    {
        // Initialisation de base (erreurs ignorées ici pour simplifier)
        bmp_.init();

        bmp390::Config cfg{};
        cfg.pressure_oversampling    = bmp390::Config::Oversampling::X4;
        cfg.temperature_oversampling = bmp390::Config::Oversampling::X1;
        cfg.odr                      = bmp390::Config::OutputDataRate::Hz25;
        cfg.iir_filter               = bmp390::Config::IirFilterCoeff::Coeff3;

        bmp_.configure(cfg);
    }

    void update() override
    {
        bmp390::Measurement m{};
        int ret = bmp_.read_measurement(m);
        if (ret == 0)
        {
            last_pressure_pa_   = m.pressure_pa;
            last_temperature_c_ = m.temperature_c;
            valid_ = true;
        }
        else
        {
            valid_ = false;
        }
    }

    double getTemperatureC() const override
    {
        if (!valid_)
        {
            return std::nan("");
        }
        return last_temperature_c_;
    }

    void log(std::ostream& os) const override
    {
        if (!valid_)
        {
            os << "[BMP390] Mesure invalide\n";
            return;
        }

        os << "[BMP390] P=" << last_pressure_pa_ << " Pa, "
           << "T=" << last_temperature_c_ << " °C\n";
    }

private:
    bmp390::Bmp390 bmp_;
    double last_pressure_pa_   = 0.0;
    double last_temperature_c_ = 0.0;
    bool   valid_              = false;
};
```

### 3.2 `Hdc3022Sensor` (température + humidité, pseudo-code)

Le capteur **HDC3022** n’a pas encore de librairie C++ concrète.  
On propose une classe `Hdc3022Sensor` qui :

- utilisera plus tard une librairie ou des fonctions bas niveau (I2C),
- stockera température et humidité,
- exposera `getTemperatureC()` et un `log()` spécifique.

#### 3.2.1 Pseudo-code C++ pour `Hdc3022Sensor`

```cpp
class Hdc3022Sensor : public ISensor
{
public:
    Hdc3022Sensor(/* paramètres de bus / adresse I2C, etc. */)
    {
        // TODO: initialisation du capteur HDC3022 (reset, config, etc.)
    }

    void update() override
    {
        // TODO: lire les registres du HDC3022 via I2C
        // Pseudo-code:
        //  - envoyer commande de mesure
        //  - attendre la conversion
        //  - lire température brute + humidité brute
        //  - appliquer la formule de conversion (datasheet)
        //  - remplir last_temperature_c_ et last_humidity_rh_
        //  - mettre valid_ = true si succès
    }

    double getTemperatureC() const override
    {
        if (!valid_)
        {
            return std::nan("");
        }
        return last_temperature_c_;
    }

    void log(std::ostream& os) const override
    {
        if (!valid_)
        {
            os << "[HDC3022] Mesure invalide\n";
            return;
        }

        os << "[HDC3022] T=" << last_temperature_c_ << " °C, "
           << "RH=" << last_humidity_rh_ << " %\n";
    }

private:
    double last_temperature_c_ = 0.0;
    double last_humidity_rh_   = 0.0;  // humidité relative (%)
    bool   valid_              = false;
};
```

---

## 4. Gestion de la liste de capteurs

On gère un ensemble hétérogène de capteurs via une collection de pointeurs polymorphes :

```cpp
#include <vector>
#include <memory>

std::vector<std::unique_ptr<ISensor>> sensors;

// Exemple de construction (pseudo-code, dépendant de la plateforme)
// bmp390::BusInterface bus = ... (fourni par l’application)
// uint8_t bmp390_addr = 0x76; // par ex.

void setupSensors()
{
    // Capteur BMP390
    // sensors.push_back(std::make_unique<Bmp390Sensor>(bus, bmp390_addr));

    // Capteur HDC3022
    // sensors.push_back(std::make_unique<Hdc3022Sensor>(/* params de bus */));
}
```

Cette approche permet d’ajouter/retirer des capteurs sans changer la logique de la boucle principale.

---

## 5. Boucle principale (pseudo-code C++)

La boucle principale parcourt tous les capteurs, les met à jour, loggue les mesures, calcule la température moyenne et déclenche une alarme si nécessaire.

```cpp
#include <iostream>
#include <cmath>

void raiseAlarm(double max_temp)
{
    std::cout << "!!! ALARME TEMPERATURE !!! T_max = "
              << max_temp << " °C" << std::endl;
}

void mainLoop()
{
    const double alarm_threshold = 30.0;

    while (true)
    {
        double sum_temp      = 0.0;
        int    count_temp    = 0;
        double max_temp_seen = -1e9;
        bool   alarm         = false;

        for (const auto& sensor : sensors)
        {
            // 1) Mise à jour des données
            sensor->update();

            // 2) Logging
            sensor->log(std::cout);

            // 3) Récupération de la température (si dispo)
            double t = sensor->getTemperatureC();
            if (!std::isnan(t))
            {
                sum_temp   += t;
                count_temp += 1;

                if (t > max_temp_seen)
                {
                    max_temp_seen = t;
                }

                if (t > alarm_threshold)
                {
                    alarm = true;
                }
            }
        }

        // 4) Calcul de la température moyenne globale
        if (count_temp > 0)
        {
            double avg_temp = sum_temp / static_cast<double>(count_temp);
            std::cout << "[GLOBAL] Température moyenne = "
                      << avg_temp << " °C" << std::endl;
        }
        else
        {
            std::cout << "[GLOBAL] Aucune température disponible" << std::endl;
        }

        // 5) Gestion de l’alarme
        if (alarm)
        {
            raiseAlarm(max_temp_seen);
        }

        // 6) Temporisation entre deux cycles de mesure
        // TODO: adapter la période (ex: std::this_thread::sleep_for)
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

---

## 6. Justification des choix

### 6.1 Pourquoi une interface `ISensor` ?

- **Abstraction commune** : permet de traiter tous les capteurs de la même façon (update/log/getTemperatureC),
  même si leurs implémentations internes (BMP390, HDC3022, futurs capteurs) sont différentes.
- **Extensibilité** : ajouter un nouveau capteur revient à créer une nouvelle classe qui hérite de `ISensor`,
  sans modifier la boucle principale ni la logique de calcul global.
- **Découplage** : la boucle principale ne dépend pas des détails de la librairie `bmp390::Bmp390` ni de la future
  librairie HDC3022 ; elle consomme uniquement l’interface abstraite.

### 6.2 Pourquoi une boucle simple monothread ?

- **Simplicité** : pour un prototype ou un système où la fréquence de lecture est modérée (par ex. 1–10 Hz),
  une boucle monothread est suffisante et facile à raisonner.
- **Débogage plus simple** : pas de conditions de concurrence, pas de mutex, pas de problèmes de synchronisation.
- **Plateforme Zynq/Ubuntu** : même si l’OS supporte le multi-threading, rien n’impose ici une architecture
  concurrente ; la latence et le débit de lecture des capteurs sont généralement faibles.

Si plus tard les contraintes temps réel deviennent fortes, on pourra évoluer vers une architecture plus sophistiquée (voir section suivante).

---

## 7. Alternatives possibles

### 7.1 Threads par capteur

- **Idée** : un thread par capteur qui lit périodiquement les données et les stocke dans une structure partagée.
- **Avantages** :
  - Chaque capteur peut avoir sa propre fréquence de rafraîchissement,
  - Les opérations de lecture potentiellement bloquantes n’impactent pas les autres capteurs.
- **Inconvénients** :
  - Complexité accrue (synchronisation, mutex, conditions de course),
  - Plus difficile à tester et à déboguer.

### 7.2 Scheduler / RTOS

- **Idée** : utiliser un RTOS (ou un scheduler sur Zynq) avec des tâches périodiques par capteur.
- **Avantages** :
  - Contrôle précis des périodes de mesure,
  - Intégration naturelle dans un système embarqué temps réel.
- **Inconvénients** :
  - Dépendance à un environnement spécifique (RTOS, scheduler),
  - Complexité de configuration et de déploiement plus élevée.

### 7.3 Pattern Observateur (publisher/subscriber) pour les alarmes

- **Idée** : lorsqu’un capteur dépasse la température seuil, il **publie** un événement, et un composant séparé
  (abonné) reçoit ces événements et gère les alarmes (logging, actions, etc.).
- **Avantages** :
  - Découplage entre capteurs et logique d’alarme,
  - Possibilité de plusieurs abonnés (GUI, logger, système d’alertes réseau).
- **Inconvénients** :
  - Architecture plus complexe (bus d’événements, gestion des abonnements),
  - Peut être surdimensionné pour un petit système de test.

---

En résumé, l’architecture proposée repose sur :

- une interface commune `ISensor`,
- des implémentations concrètes par capteur (`Bmp390Sensor`, `Hdc3022Sensor`),
- une boucle principale monothread qui orchestre mise à jour, logging, calcul de moyenne et gestion des alarmes.

Cette base est simple à faire évoluer vers des architectures plus avancées (multi-threads, RTOS, events) si les besoins du projet augmentent.