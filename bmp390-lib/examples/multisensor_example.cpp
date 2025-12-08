#include <vector>
#include <memory>
#include <iostream>
#include <cmath>
#include <cstdint>

#include "bmp390/bmp390_driver.hpp"

using namespace bmp390;

// -----------------------------------------------------------------------------
// Interface abstraite ISensor
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Implémentation BMP390 : Bmp390Sensor
// -----------------------------------------------------------------------------

class Bmp390Sensor : public ISensor
{
public:
    Bmp390Sensor(const BusInterface& bus, uint8_t i2c_addr)
        : bmp_(i2c_addr, bus, /*use_i2c=*/true)
    {
        // Initialisation de base (erreurs ignorées ici pour simplifier)
        (void)bmp_.init();

        Config cfg{};
        cfg.pressure_oversampling    = Config::Oversampling::X4;
        cfg.temperature_oversampling = Config::Oversampling::X1;
        cfg.odr                      = Config::OutputDataRate::Hz25;
        cfg.iir_filter               = Config::IirFilterCoeff::Coeff3;

        (void)bmp_.configure(cfg);
    }

    void update() override
    {
        Measurement m{};
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
    Bmp390 bmp_;
    double last_pressure_pa_   = 0.0;
    double last_temperature_c_ = 0.0;
    bool   valid_              = false;
};

// -----------------------------------------------------------------------------
// Implémentation HDC3022 (pseudo-code) : Hdc3022Sensor
// -----------------------------------------------------------------------------

class Hdc3022Sensor : public ISensor
{
public:
    Hdc3022Sensor(/* paramètres de bus / adresse I2C, etc. */)
    {
        // TODO: initialisation du capteur HDC3022 :
        //  - ouverture du bus I2C
        //  - configuration de l'adresse esclave
        //  - reset / configuration des registres si nécessaire
    }

    void update() override
    {
        // TODO: lire les registres du HDC3022 via I2C
        // Pseudo-code :
        //  1) Envoyer une commande de mesure (temp + humidité)
        //  2) Attendre la fin de conversion (delay)
        //  3) Lire les valeurs brutes (temp_raw, hum_raw)
        //  4) Appliquer les formules de conversion du datasheet :
        //       temp_c = ...
        //       hum_rh = ...
        //  5) Mettre à jour last_temperature_c_ et last_humidity_rh_
        //  6) Mettre valid_ = true si succès, false sinon

        // Exemple placeholder :
        last_temperature_c_ = 25.0;  // Valeur fictive
        last_humidity_rh_   = 50.0;  // Valeur fictive
        valid_              = true;
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

// -----------------------------------------------------------------------------
// Callbacks bas niveau BMP390 (stubs d'exemple)
// -----------------------------------------------------------------------------

int8_t my_i2c_read(uint8_t reg, uint8_t* data, uint16_t len)
{
    // TODO: implémenter la lecture I2C réelle pour la plateforme (ex: /dev/i2c-*)
    (void)reg;
    (void)data;
    (void)len;
    return 0;
}

int8_t my_i2c_write(uint8_t reg, const uint8_t* data, uint16_t len)
{
    // TODO: implémenter l'écriture I2C réelle pour la plateforme (ex: /dev/i2c-*)
    (void)reg;
    (void)data;
    (void)len;
    return 0;
}

void my_delay_us(uint32_t period)
{
    // TODO: implémenter un délai en microsecondes (usleep, HAL_Delay_us, etc.)
    (void)period;
}

// -----------------------------------------------------------------------------
// Gestion de la liste de capteurs
// -----------------------------------------------------------------------------

std::vector<std::unique_ptr<ISensor>> sensors;

void setupSensors()
{
    // Création de l'interface bus pour le BMP390
    BusInterface bus{};
    bus.read     = my_i2c_read;
    bus.write    = my_i2c_write;
    bus.delay_us = my_delay_us;

    // Adresse I2C du BMP390 (à adapter selon le câblage)
    constexpr uint8_t bmp390_i2c_addr = 0x76;

    // Capteur BMP390
    sensors.push_back(std::make_unique<Bmp390Sensor>(bus, bmp390_i2c_addr));

    // Capteur HDC3022 (pseudo-code, pas de paramètres concrets ici)
    sensors.push_back(std::make_unique<Hdc3022Sensor>());
}

// -----------------------------------------------------------------------------
// Gestion de l'alarme
// -----------------------------------------------------------------------------

void raiseAlarm(double max_temp)
{
    std::cout << "!!! ALARME TEMPERATURE !!! T_max = "
              << max_temp << " °C" << std::endl;
}

// -----------------------------------------------------------------------------
// Boucle principale
// -----------------------------------------------------------------------------

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

        // Pour éviter une boucle infinie trop agressive dans un exemple :
        break; // TODO: retirer ce break dans un vrai programme
    }
}

// -----------------------------------------------------------------------------
// Point d'entrée
// -----------------------------------------------------------------------------

int main()
{
    setupSensors();
    mainLoop();
    return 0;
}