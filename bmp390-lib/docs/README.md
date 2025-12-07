# mirega-bmp390-test

> Ce wrapper C++ a été développé dans le cadre d’un test technique.
> L’objectif est de montrer une intégration propre du driver Bosch BMP3_SensorAPI
> et une API C++ simple pour le capteur BMP390.

# Librairie C++ BMP390 (wrapper Bosch BMP3_SensorAPI)

## 1. Résumé

Cette librairie fournit une interface C++ de haut niveau pour le capteur de pression BMP390, en s’appuyant sur le driver C officiel Bosch **BMP3_SensorAPI**.  
Elle expose une classe unique, `bmp390::Bmp390`, permettant :

- d’initialiser le capteur,
- de configurer les paramètres de mesure (oversampling, ODR, filtre IIR),
- de lire facilement pression et température compensées.

L’objectif est de masquer les détails de la structure `bmp3_dev` et des appels C, tout en laissant à l’application le contrôle du bus (I2C ou SPI) via des callbacks.

---

## 2. Arbre de fichiers minimal

```text
bmp390-lib/
  include/
    bmp390/
      bmp390_driver.hpp        # Interface C++ haut niveau
  src/
    bmp390_driver.cpp          # Implémentation C++ de bmp390::Bmp390
    third_party/
      bmp3.c                   # Driver Bosch BMP3_SensorAPI (implémentation C)
      bmp3.h                   # API C du driver Bosch
      bmp3_defs.h              # Définitions, registres, types Bosch
  docs/
    README.md                  # Ce document
```

---

## 3. Dépendances

### 3.1 Driver Bosch BMP3_SensorAPI

La librairie repose sur les sources officielles Bosch :

- `bmp3.c`
- bmp3.h
- bmp3_defs.h

Ces fichiers sont intégrés dans third_party et ne doivent pas être modifiés, sauf nécessité spécifique.

### 3.2 Callbacks bus fournis par l’application

L’accès matériel (I2C ou SPI) n’est **pas** géré directement par la librairie : il est délégué à l’application utilisateur via la structure `bmp390::BusInterface` :

```cpp
struct BusInterface
{
    int8_t (*read)(uint8_t reg, uint8_t* data, uint16_t len);
    int8_t (*write)(uint8_t reg, const uint8_t* data, uint16_t len);
    void   (*delay_us)(uint32_t period);
};
```

- `read` : lit `len` octets à partir du registre `reg`.
- `write` : écrit `len` octets à partir de `data` dans le registre `reg`.
- `delay_us` : temporisation en microsecondes (utilisée par le driver Bosch).

L’application doit fournir des fonctions compatibles avec ces signatures, adaptées à la plateforme cible (Zynq, Ubuntu, MCU, etc.).

---

## 4. Exemple d’utilisation minimal en C++ (pseudo-code)

```cpp
#include <cstdint>
#include <cstdio>

#include "bmp390/bmp390_driver.hpp"

using namespace bmp390;

// Implémentations dépendantes de la plateforme (exemples simplifiés)

// Exemple I2C via /dev/i2c-X, à adapter selon votre code
int8_t my_i2c_read(uint8_t reg, uint8_t* data, uint16_t len)
{
    // TODO: ouvrir /dev/i2c-X, faire un write(reg) puis read(len), etc.
    // Retourner 0 en cas de succès, <0 en cas d’erreur.
    return 0;
}

int8_t my_i2c_write(uint8_t reg, const uint8_t* data, uint16_t len)
{
    // TODO: ouvrir /dev/i2c-X, écrire reg puis les données, etc.
    // Retourner 0 en cas de succès, <0 en cas d’erreur.
    return 0;
}

void my_delay_us(uint32_t period)
{
    // Sur Linux : usleep(period);
    // Sur MCU : HAL_Delay us, timer, busy-wait, etc.
    // TODO: implémenter en fonction de la plateforme.
}

// Exemple d’utilisation
int main()
{
    // 1) Définir l’interface bus
    BusInterface bus{};
    bus.read     = my_i2c_read;
    bus.write    = my_i2c_write;
    bus.delay_us = my_delay_us;

    // 2) Créer l’objet Bmp390
    constexpr uint8_t bmp390_i2c_addr = 0x76; // ou 0x77 selon le câblage
    bool use_i2c = true;

    Bmp390 sensor(bmp390_i2c_addr, bus, use_i2c);

    // 3) Initialiser le capteur
    int ret = sensor.init();
    if (ret != 0)
    {
        std::printf("Erreur init BMP390: %d\n", ret);
        return ret;
    }

    // 4) Configurer les paramètres de mesure
    Config cfg{};
    cfg.pressure_oversampling    = Config::Oversampling::X4;
    cfg.temperature_oversampling = Config::Oversampling::X1;
    cfg.odr                      = Config::OutputDataRate::Hz25;
    cfg.iir_filter               = Config::IirFilterCoeff::Coeff3;

    ret = sensor.configure(cfg);
    if (ret != 0)
    {
        std::printf("Erreur configure BMP390: %d\n", ret);
        return ret;
    }

    // 5) Lire une mesure pression + température
    Measurement m{};
    ret = sensor.read_measurement(m);
    if (ret != 0)
    {
        std::printf("Erreur lecture BMP390: %d\n", ret);
        return ret;
    }

    // 6) Afficher les résultats
    std::printf("Pression : %f Pa\n", m.pressure_pa);
    std::printf("Température : %f °C\n", m.temperature_c);

    return 0;
}
```

Cet exemple est volontairement simplifié : il faut adapter `my_i2c_read`, `my_i2c_write` et `my_delay_us` à votre environnement réel.

---

## 5. Intégration sur une plateforme type Zynq/Ubuntu

### 5.1 Accès I2C via `/dev/i2c-*`

Sur une plateforme Linux (Zynq, PC Ubuntu, etc.), l’I2C est généralement exposé via des fichiers de périphériques i2c-0, i2c-1, etc.

Les callbacks `read` et `write` de `BusInterface` peuvent être implémentés avec l’API Linux I2C :

- ouvrir le descripteur de fichier (`open("/dev/i2c-1", O_RDWR);`),
- sélectionner l’esclave (ioctl `I2C_SLAVE` avec l’adresse du BMP390),
- faire un `write` pour envoyer le registre, suivi d’un `read` pour récupérer les données (pour la lecture),
- ou un `write` combinant registre + données (pour l’écriture).

Pseudo-code simplifié :

```cpp
int8_t my_i2c_read(uint8_t reg, uint8_t* data, uint16_t len)
{
    // 1) Ecrire l’adresse de registre (write)
    // 2) Lire len octets (read)
    // 3) Gérer les erreurs et retourner 0 ou <0
}

int8_t my_i2c_write(uint8_t reg, const uint8_t* data, uint16_t len)
{
    // 1) Construire un buffer [reg][data...]
    // 2) Write sur le descripteur I2C
    // 3) Retourner 0 ou <0
}
```

Le descripteur de fichier peut être stocké dans des variables globales, dans un singleton, ou passé via un contexte si vous adaptez le design.

### 5.2 Délai en microsecondes

Sur Linux (y compris Zynq/Ubuntu), `delay_us` peut utiliser :

- `usleep(period);` (POSIX, microsecondes),
- `nanosleep` pour un contrôle plus fin.

Exemple :

```cpp
#include <unistd.h>

void my_delay_us(uint32_t period)
{
    usleep(period);
}
```

Sur un microcontrôleur ou une plateforme bare-metal, `delay_us` peut être mappé sur :

- une fonction HAL fournie par le vendor,
- un timer matériel,
- une boucle de busy-wait calibrée (moins recommandé).

---

## 6. Limites et améliorations possibles

### 6.1 Mapping des enums `Config` vers les macros Bosch

- Les enums `Config::Oversampling`, `Config::OutputDataRate` et `Config::IirFilterCoeff` sont mappés vers les macros Bosch (`BMP3_OVERSAMPLING_*`, `BMP3_ODR_*`, `BMP3_IIR_FILTER_*`) via des fonctions internes au `.cpp` (switch).
- Ces mappings semblent raisonnables, mais ils doivent être **validés**:
  - vérifier les correspondances exactes avec la documentation Bosch,
  - ajuster les valeurs par défaut (oversampling, ODR, filtre) selon les besoins réels (précision vs consommation).

### 6.2 Gestion d’erreurs

- Actuellement, la classe `Bmp390` se contente de retourner les codes d’erreur Bosch (`int8_t`), castés en `int`.
- Améliorations possibles :
  - exposer une enum C++ de codes d’erreur plus parlants,
  - fournir une fonction d’aide pour traduire un code d’erreur en message texte,
  - ajouter des checks supplémentaires (nullptr, état interne, reconfigurations, etc.),
  - intégrer un mécanisme de logging (std::cerr, syslog, logger custom).

### 6.3 Tests unitaires et exemples supplémentaires

- Ajouter des tests unitaires (par ex. avec GoogleTest ou Catch2) :
  - tests de mapping enums -> macros,
  - tests de comportement en cas d’erreur bus (callbacks qui renvoient une erreur),
  - tests de scénarios réels simulés (mock I2C).
- Fournir plus d’exemples :
  - intégration complète sur Linux `/dev/i2c-*`,
  - intégration sur un MCU avec HAL (STM32, Zynq PS, etc.),
  - lecture périodique dans une boucle principale, avec filtrage ou logging.

---

En résumé, cette librairie propose une abstraction C++ simple au-dessus du driver Bosch BMP3_SensorAPI, tout en laissant la flexibilité nécessaire pour l’intégrer sur différentes plateformes (Linux, Zynq, MCU). L’étape suivante consiste à adapter les callbacks bus à votre environnement et à renforcer la robustesse (mappings, erreurs, tests).