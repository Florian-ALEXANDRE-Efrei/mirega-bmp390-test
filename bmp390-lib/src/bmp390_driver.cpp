#include "bmp390/bmp390_driver.hpp"

#include "third_party/bmp3.h"

namespace bmp390
{

// Helpers de mapping Config -> macros BMP3
static uint8_t map_oversampling(Config::Oversampling os)
{
    switch (os)
    {
        case Config::Oversampling::X1:  return BMP3_NO_OVERSAMPLING; // TODO: vérifier le mapping exact
        case Config::Oversampling::X2:  return BMP3_OVERSAMPLING_2X;
        case Config::Oversampling::X4:  return BMP3_OVERSAMPLING_4X;
        case Config::Oversampling::X8:  return BMP3_OVERSAMPLING_8X;
        case Config::Oversampling::X16: return BMP3_OVERSAMPLING_16X;
        case Config::Oversampling::X32: return BMP3_OVERSAMPLING_32X;
        default:                        return BMP3_OVERSAMPLING_4X; // Valeur par défaut raisonnable
    }
}

static uint8_t map_odr(Config::OutputDataRate odr)
{
    switch (odr)
    {
        case Config::OutputDataRate::Hz200:  return BMP3_ODR_200_HZ;
        case Config::OutputDataRate::Hz100:  return BMP3_ODR_100_HZ;
        case Config::OutputDataRate::Hz50:   return BMP3_ODR_50_HZ;
        case Config::OutputDataRate::Hz25:   return BMP3_ODR_25_HZ;
        case Config::OutputDataRate::Hz12_5: return BMP3_ODR_12_5_HZ;
        case Config::OutputDataRate::Hz6_25: return BMP3_ODR_6_25_HZ;
        case Config::OutputDataRate::Hz3_1:  return BMP3_ODR_3_1_HZ;
        case Config::OutputDataRate::Hz1_5:  return BMP3_ODR_1_5_HZ;
        case Config::OutputDataRate::Hz0_78: return BMP3_ODR_0_78_HZ;
        case Config::OutputDataRate::Hz0_39: return BMP3_ODR_0_39_HZ;
        case Config::OutputDataRate::Hz0_2:  return BMP3_ODR_0_2_HZ;
        case Config::OutputDataRate::Hz0_1:  return BMP3_ODR_0_1_HZ;
        case Config::OutputDataRate::Hz0_05: return BMP3_ODR_0_05_HZ;
        case Config::OutputDataRate::Hz0_02: return BMP3_ODR_0_02_HZ;
        case Config::OutputDataRate::Hz0_01: return BMP3_ODR_0_01_HZ;
        default:                             return BMP3_ODR_25_HZ; // Valeur par défaut raisonnable
    }
}

static uint8_t map_iir_filter(Config::IirFilterCoeff coeff)
{
    switch (coeff)
    {
        case Config::IirFilterCoeff::Off:     return BMP3_IIR_FILTER_DISABLE;
        case Config::IirFilterCoeff::Coeff1:  return BMP3_IIR_FILTER_COEFF_1;
        case Config::IirFilterCoeff::Coeff3:  return BMP3_IIR_FILTER_COEFF_3;
        case Config::IirFilterCoeff::Coeff7:  return BMP3_IIR_FILTER_COEFF_7;
        case Config::IirFilterCoeff::Coeff15: return BMP3_IIR_FILTER_COEFF_15;
        case Config::IirFilterCoeff::Coeff31: return BMP3_IIR_FILTER_COEFF_31;
        case Config::IirFilterCoeff::Coeff63: return BMP3_IIR_FILTER_COEFF_63;
        case Config::IirFilterCoeff::Coeff127:return BMP3_IIR_FILTER_COEFF_127;
        default:                              return BMP3_IIR_FILTER_COEFF_3; // Valeur par défaut raisonnable
    }
}

// Callbacks d’adaptation entre BusInterface (reg, data, len) et bmp3 (reg_addr, reg_data, len)
static BMP3_INTF_RET_TYPE bmp3_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    BusInterface *bus = static_cast<BusInterface *>(intf_ptr);
    if (!bus || !bus->read)
    {
        return -1;
    }

    // TODO: gérer éventuellement les conversions de type / longueur si nécessaire
    if (length > 0xFFFFU)
    {
        // Longueur supérieure à uint16_t non supportée par l’interface de haut niveau
        return -1;
    }

    return bus->read(reg_addr, reg_data, static_cast<uint16_t>(length));
}

static BMP3_INTF_RET_TYPE bmp3_bus_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    BusInterface *bus = static_cast<BusInterface *>(intf_ptr);
    if (!bus || !bus->write)
    {
        return -1;
    }

    // TODO: gérer éventuellement les conversions de type / longueur si nécessaire
    if (length > 0xFFFFU)
    {
        // Longueur supérieure à uint16_t non supportée par l’interface de haut niveau
        return -1;
    }

    return bus->write(reg_addr, reg_data, static_cast<uint16_t>(length));
}

static void bmp3_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    BusInterface *bus = static_cast<BusInterface *>(intf_ptr);
    if (bus && bus->delay_us)
    {
        bus->delay_us(period);
    }
}

// ============================================================================
// Bmp390 implementation
// ============================================================================

Bmp390::Bmp390(uint8_t dev_id, const BusInterface& bus, bool use_i2c)
    : dev_id_(dev_id),
      use_i2c_(use_i2c),
      bus_(bus),
      dev_(nullptr)
{
    dev_ = new bmp3_dev{};
}

Bmp390::~Bmp390()
{
    delete dev_;
    dev_ = nullptr;
}

int Bmp390::init()
{
    if (!dev_)
    {
        return -1;
    }

    // Configuration de la structure bmp3_dev
    dev_->dev_id = dev_id_;
    dev_->intf   = use_i2c_ ? BMP3_I2C_INTF : BMP3_SPI_INTF;

    // On passe le BusInterface via intf_ptr pour l’utiliser dans les callbacks
    dev_->intf_ptr = &bus_;

    dev_->read     = bmp3_bus_read;
    dev_->write    = bmp3_bus_write;
    dev_->delay_us = bmp3_delay_us;

    // Initialisation du capteur
    int8_t rslt = bmp3_init(dev_);

    // TODO: Optionnellement, effectuer un soft reset après init
    // if (rslt == BMP3_OK)
    // {
    //     rslt = bmp3_soft_reset(dev_);
    // }

    return static_cast<int>(rslt);
}

int Bmp390::configure(const Config& config)
{
    if (!dev_)
    {
        return -1;
    }

    bmp3_settings settings{};
    int8_t rslt = BMP3_OK;

    // Activation pression + température
    settings.press_en = BMP3_ENABLE;
    settings.temp_en  = BMP3_ENABLE;

    // Oversampling
    settings.press_os = map_oversampling(config.pressure_oversampling);
    settings.temp_os  = map_oversampling(config.temperature_oversampling);

    // ODR
    settings.odr = map_odr(config.odr);

    // Filtre IIR
    settings.iir_filter = map_iir_filter(config.iir_filter);

    // Indique quels champs on souhaite configurer
    uint32_t desired_settings = 0;
    desired_settings |= BMP3_SEL_PRESS_EN;
    desired_settings |= BMP3_SEL_TEMP_EN;
    desired_settings |= BMP3_SEL_PRESS_OS;
    desired_settings |= BMP3_SEL_TEMP_OS;
    desired_settings |= BMP3_SEL_ODR;
    desired_settings |= BMP3_SEL_IIR_FILTER;

    rslt = bmp3_set_sensor_settings(desired_settings, &settings, dev_);
    if (rslt != BMP3_OK)
    {
        return static_cast<int>(rslt);
    }

    // Choix d’un mode simple : normal mode
    settings.op_mode = BMP3_MODE_NORMAL;
    rslt = bmp3_set_op_mode(&settings, dev_);
    if (rslt != BMP3_OK)
    {
        return static_cast<int>(rslt);
    }

    return static_cast<int>(rslt);
}

int Bmp390::read_measurement(Measurement& out)
{
    if (!dev_)
    {
        return -1;
    }

    bmp3_data data{};
    int8_t rslt = bmp3_get_sensor_data(BMP3_PRESS_TEMP, &data, dev_);
    if (rslt != BMP3_OK)
    {
        return static_cast<int>(rslt);
    }

#ifdef BMP3_FLOAT_COMPENSATION
    // Version flottante : data.pressure et data.temperature sont en unités physiques
    out.pressure_pa   = data.pressure;
    out.temperature_c = data.temperature;
#else
    // TODO: adapter le scaling si on utilise la compensation entière
    // Pour l’instant, on suppose que le portage adaptera les unités au besoin.
    out.pressure_pa   = static_cast<double>(data.pressure);
    out.temperature_c = static_cast<double>(data.temperature);
#endif

    return static_cast<int>(rslt);
}

}  // namespace bmp390