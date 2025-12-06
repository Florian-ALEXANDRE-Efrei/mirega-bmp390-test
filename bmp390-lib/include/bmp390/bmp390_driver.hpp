#pragma once

#include <cstdint>

struct bmp3_dev;  // Forward declaration of Bosch BMP3 device struct

namespace bmp390
{

/**
 * @brief Abstraction de l’interface bus (I2C ou SPI) pour le BMP390.
 *
 * Les fonctions doivent être fournies par l’application et réaliser
 * les accès registres (adressés par un registre 8 bits) et le délai.
 */
struct BusInterface
{
    /**
     * @brief Fonction de lecture sur le bus.
     *
     * @param reg  Adresse du registre de départ.
     * @param data Buffer de destination.
     * @param len  Nombre d’octets à lire.
     * @return 0 si succès, valeur négative en cas d’erreur.
     */
    int8_t (*read)(uint8_t reg, uint8_t* data, uint16_t len) = nullptr;

    /**
     * @brief Fonction d’écriture sur le bus.
     *
     * @param reg  Adresse du registre de départ.
     * @param data Buffer source.
     * @param len  Nombre d’octets à écrire.
     * @return 0 si succès, valeur négative en cas d’erreur.
     */
    int8_t (*write)(uint8_t reg, const uint8_t* data, uint16_t len) = nullptr;

    /**
     * @brief Fonction de délai en microsecondes.
     *
     * @param period Durée du délai en microsecondes.
     */
    void (*delay_us)(uint32_t period) = nullptr;
};

/**
 * @brief Configuration minimale de la mesure BMP390.
 *
 * Les valeurs sont exprimées via des enums proches de celles du driver Bosch,
 * mais sans en exposer directement les détails dans cette interface C++.
 */
struct Config
{
    /// Oversampling générique (sera mappé sur les macros BMP3_OVERSAMPLING_xX).
    enum class Oversampling : uint8_t
    {
        X1  = 0,
        X2  = 1,
        X4  = 2,
        X8  = 3,
        X16 = 4,
        X32 = 5
    };

    /// Fréquence de sortie (ODR) générique (sera mappée sur les macros BMP3_ODR_x).
    enum class OutputDataRate : uint8_t
    {
        Hz200   = 0,
        Hz100   = 1,
        Hz50    = 2,
        Hz25    = 3,
        Hz12_5  = 4,
        Hz6_25  = 5,
        Hz3_1   = 6,
        Hz1_5   = 7,
        Hz0_78  = 8,
        Hz0_39  = 9,
        Hz0_2   = 10,
        Hz0_1   = 11,
        Hz0_05  = 12,
        Hz0_02  = 13,
        Hz0_01  = 14
    };

    /// Coefficients de filtre IIR génériques.
    enum class IirFilterCoeff : uint8_t
    {
        Off   = 0,
        Coeff1,
        Coeff3,
        Coeff7,
        Coeff15,
        Coeff31,
        Coeff63,
        Coeff127
    };

    /// Oversampling pression (par défaut X4).
    Oversampling pressure_oversampling = Oversampling::X4;

    /// Oversampling température (par défaut X1).
    Oversampling temperature_oversampling = Oversampling::X1;

    /// Taux de sortie (par défaut 25 Hz).
    OutputDataRate odr = OutputDataRate::Hz25;

    /// Coefficient de filtre IIR (par défaut faible).
    IirFilterCoeff iir_filter = IirFilterCoeff::Coeff3;
};

/**
 * @brief Mesure compensée pression + température.
 */
struct Measurement
{
    /// Pression compensée en Pascals.
    double pressure_pa = 0.0;

    /// Température compensée en degrés Celsius.
    double temperature_c = 0.0;
};

/**
 * @brief Classe de haut niveau pour le capteur BMP390, basée sur BMP3_SensorAPI.
 *
 * Cette classe encapsule la configuration du driver Bosch (bmp3_dev)
 * et expose une interface C++ simple pour l’initialisation et la lecture
 * des mesures pression + température.
 */
class Bmp390
{
public:
    /**
     * @brief Construit un objet BMP390.
     *
     * @param dev_id Identifiant du device :
     *               - En I2C : adresse 7 bits (0x76, 0x77, …).
     *               - En SPI : valeur de chip select (interprétation laissée à l’application).
     * @param bus    Interface bus (callbacks lecture/écriture/délai) fournie par l’application.
     * @param use_i2c Si vrai, utilisation de l’interface I2C, sinon SPI.
     */
    Bmp390(uint8_t dev_id, const BusInterface& bus, bool use_i2c);
    
    ~Bmp390();

    /**
     * @brief Initialise le capteur BMP390.
     *
     * Cette méthode encapsule la configuration de la structure interne bmp3_dev
     * et l’appel à bmp3_init (et éventuellement un soft reset).
     *
     * @return 0 si succès, valeur négative en cas d’erreur.
     */
    int init();

    /**
     * @brief Configure les paramètres de mesure du capteur.
     *
     * Cette méthode encapsule l’appel à bmp3_set_sensor_settings et
     * bmp3_set_op_mode (typiquement en mode normal ou forced).
     *
     * @param config Configuration souhaitée (oversampling, ODR, filtre).
     * @return 0 si succès, valeur négative en cas d’erreur.
     */
    int configure(const Config& config);

    /**
     * @brief Lit une mesure pression + température.
     *
     * Cette méthode encapsule l’appel à bmp3_get_sensor_data et renvoie
     * les valeurs compensées dans l’unité physique (Pa et °C).
     *
     * @param out Structure de sortie pour la mesure.
     * @return 0 si succès, valeur négative en cas d’erreur.
     */
    int read_measurement(Measurement& out);

private:
    uint8_t dev_id_;
    bool use_i2c_;
    BusInterface bus_;

    /// Pointeur vers la structure BMP3 interne (gérée en implémentation).
    bmp3_dev* dev_;
};

}  // namespace bmp390