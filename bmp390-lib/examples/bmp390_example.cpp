#include <cstdint>
#include <cstdio>

#include "bmp390/bmp390_driver.hpp"

using namespace bmp390;

// -----------------------------------------------------------------------------
// Implémentations dépendantes de la plateforme (stubs à compléter)
// -----------------------------------------------------------------------------

int8_t my_i2c_read(uint8_t reg, uint8_t* data, uint16_t len)
{
    // TODO: implémenter la lecture I2C réelle (ex: via /dev/i2c-*)
    (void)reg;
    (void)data;
    (void)len;
    return 0; // Retourner <0 en cas d'erreur
}

int8_t my_i2c_write(uint8_t reg, const uint8_t* data, uint16_t len)
{
    // TODO: implémenter l'écriture I2C réelle (ex: via /dev/i2c-*)
    (void)reg;
    (void)data;
    (void)len;
    return 0; // Retourner <0 en cas d'erreur
}

void my_delay_us(uint32_t period)
{
    // TODO: implémenter un délai en microsecondes (ex: usleep(period) sous Linux)
    (void)period;
}

int main()
{
    // 1) Définir l'interface bus
    BusInterface bus{};
    bus.read     = my_i2c_read;
    bus.write    = my_i2c_write;
    bus.delay_us = my_delay_us;

    // 2) Créer l'objet Bmp390
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
