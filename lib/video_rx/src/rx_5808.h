//
// RX5808 Module
//

#ifndef __rx5808_INCLUDED__
#define __rx5808_INCLUDED__

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/spi_master.h"

#define PACKED __attribute__((packed))

#define RX5808_READ 0
#define RX5808_WRITE 1

typedef PACKED struct
{
  uint8_t address : 4;
  uint8_t readWrite : 1;
  uint32_t data : 20;
} rx5808_request_t;

// Address 0x00: Synthesizer Register A
typedef struct
{
  uint8_t unused : 5;
  uint16_t SYN_RF_R_REG : 15; // Default
                              // 5.8GHz: 0x0010
                              // R-counter divider ratio control for RF Synthesizer.
                              // For 5.8GHz Default: 00008H
                              // Crystal clock (Fosc )=: 8MHz
                              // Reference clock=crystal clock/R-counter=8MHz/8=1MHz
} PACKED rx5808_register_00_data_t;

// Address 0x01: Synthesizer Register B. Default: 0x02A05
typedef struct
{
  uint16_t SYN_RF_N_REG : 13; // N counter divider ratio control for RF Synthesizer.
  uint8_t SYN_RF_A_REG : 7;   // A counter divider ratio control for RF Synthesizer.
} PACKED rx5808_register_01_data_t;

// Address 0x02: Synthesizer Register C. Default: 0xFFE44
typedef struct
{
  uint8_t AGC_6M : 3;  // 6M Audio Demodulator AGC control
  uint8_t AGC_6M5 : 3; // 6M5 Audio Demodulator AGC control
  uint8_t CC_VCO : 2;  // VCO current control
  uint8_t CP_5GLO : 3; // 5G VCO buffer current control

  uint8_t CP_FT : 3;  // Charge pump current control (from 50uA to 6mA, default=100uA)
  uint8_t SC_CTL : 1; // CP current test control
  uint8_t MOUT : 2;   // Multi-function output select (RF R divider output, RF prescaler output, lock in detect)
  uint8_t PRES_F : 3; // Prescaler tail current control (20 ~ 140uA)

} PACKED rx5808_register_02_data_t;

// Address 0x03: Synthesizer Register D. Default: 0x03980
typedef struct
{
  uint8_t unused : 6;
  uint8_t SYN_C3 : 3; // Loop filter C3 control
  uint8_t SYN_CZ : 3; // Loop filter CZ control
  uint8_t SYN_RZ : 8; // Loop filter RZ control
} PACKED rx5808_register_03_data_t;

// Address 0x04: VCO Switch-Cap Control Register. Default: 0x7ABEF
typedef struct
{
  uint8_t RFVCO_EX_CAP : 5;   // For RF VCO adjustment
  uint8_t _480VCO_EX_CAP : 5; // For IF VCOadjustment
  uint8_t _6MVCO_EX_CAP : 5;  // For 6M VCOadjustment
  uint8_t _6M5VCO_EX_CAP : 5; // For 6M5 VCOadjustment
} PACKED rx5808_register_04_data_t;

// Address 0x05: DFC Control Register
typedef struct
{
  uint8_t EN_RECAL : 1;
  uint8_t R : 6;          // DFC reference clock control, set default value to 63.
  uint8_t OK_RF : 1;      // RF VCO fine tune
  uint8_t OK_IF : 1;      // IF VCO fine tune
  uint8_t OK_6M : 1;      // 6M VCO fine tune
  uint8_t OK_6M5 : 1;     // 6M5 VCO fine tune
  uint8_t VCODFC_OVP : 3; // RF VCO setting
  uint8_t DFC480_OVP : 3; // IF VCO setting
  uint8_t AUDFC_OVP : 3;  // 6M/6M5 VCO setting
} PACKED rx5808_register_05_data_t;

// Address 0x06: 6M Audio Demodulator Control Register. Default: 82408H
typedef struct
{
  uint8_t _6M_ICP : 6; // 6M Charge-Pump current control
  uint8_t _6M_C3 : 3;  // 6M Loop Filter Adjusting
  uint8_t _6M_CZ : 3;  // 6M Loop Filter Adjusting
  uint8_t _6M_RZ : 8;  // 6M Loop Filter Adjusting
} PACKED rx5808_register_06_data_t;

// Address 0x07: 6M5 Audio Demodulator Control Register. Default: 82408H
typedef struct
{
  uint8_t _6M5_ICP : 6; // 6M5 Charge-Pump current control
  uint8_t _6M5_C3 : 3;  // 6M5 Loop Filter Adjusting
  uint8_t _6M5_CZ : 3;  // 6M5 Loop Filter Adjusting
  uint8_t _6M5_RZ : 8;  // 6M5 Loop Filter Adjusting

} PACKED rx5808_register_07_data_t;

// Address 0x08: Receiver Control Register 1. Default: 0FF80H
typedef struct
{
  uint8_t unused : 6;
  uint8_t CP_MIXER : 3; // RF Mixer current control
  uint8_t IFA_GN : 3;   // IFABF gain control
  uint8_t VAMP_GN : 8;  // Video Amp gain control 2
} PACKED rx5808_register_08_data_t;

// Address 0x09: Receiver Control Register 2. Default: B2007H
typedef struct
{
  uint8_t IFAF_GN : 3;        // IFAAF gain control
  uint8_t REGBS_VADJ : 3;     // BS regulator reference voltage adjust
  uint8_t REGIF_VADJ : 3;     // IF regulator reference voltage adjust
  uint8_t BC : 3;             // BC adjust
  uint8_t RSSI_SQUELCH_D : 8; // RSSI & noise squelch control
} PACKED rx5808_register_09_data_t;

// Address 0x0A: Power Down Control Register. Default: 5.8GHz: 10C13H
typedef struct
{
  uint8_t PD_VCLAMP : 1;       // Video clamp power down control
  uint8_t PD_VAMP : 1;         // Video amp power down control
  uint8_t PD_IF_DEMOD : 1;     // IF demodulator power down control
  uint8_t PD_IFAF : 1;         // IFAF power down control
  uint8_t PD_RSSI_SQUELCH : 1; // RSSI & noise squelch power down control
  uint8_t PD_REGBS : 1;        // BS regulator power down control
  uint8_t PD_REGIF : 1;        // IF regulator power down control
  uint8_t PD_BC : 1;           // BC power down control
  uint8_t PD_DIV4 : 1;         // Divide-by-4 power down control
  uint8_t PD_5GVCO : 1;        // 5G VCO power down control
  uint8_t PD_SYN : 1;          // SYN power down control
  uint8_t PD_AU6M : 1;         // 6M audio modulator power down control
  uint8_t PD_6M : 1;           // 6M power down control
  uint8_t PD_AU6M5 : 1;        // 6M5 audio modulator power down control
  uint8_t PD_6M5 : 1;          // 6M5 power down control
  uint8_t PD_REG1D8 : 1;       // 1.8V regulator power down control
  uint8_t PD_IFABF : 1;        // IFABF power down control
  uint8_t PD_MIXER : 1;        // RF Mixer power down control
  uint8_t PD_DIV80 : 1;        // Divide-by-80 power down control
  uint8_t PD_PLL1D8 : 1;       // PLL 1.8V regulator power down control
} PACKED rx5808_register_0A_data_t;

// Address 0x0B~0x0E: Reserved

// Address 0x0F: State Register
typedef struct
{
  uint32_t unused : 17;
  uint8_t STATE : 3; // State Name Description
                     // 000 RESET Reset state.
                     // 001 PWRON_CAL Power on state.
                     // 010 STBY Standby state.
                     // 011 VCO_CAL VCO state.

} PACKED rx5808_register_0F_data_t;

typedef struct
{
  rx5808_register_00_data_t reg00;
  rx5808_register_01_data_t reg01;
  rx5808_register_02_data_t reg02;
  rx5808_register_03_data_t reg03;
  rx5808_register_04_data_t reg04;
  rx5808_register_05_data_t reg05;
  rx5808_register_06_data_t reg06;
  rx5808_register_07_data_t reg07;
  rx5808_register_08_data_t reg08;
  rx5808_register_09_data_t reg09;
  rx5808_register_0A_data_t reg0A;
  rx5808_register_0F_data_t reg0F;

} rx5808_state_t;

uint32_t rx5808_spi_read(spi_device_handle_t dev, uint8_t address);
int rx5808_spi_read_state(spi_device_handle_t spi, rx5808_state_t *state);

#endif