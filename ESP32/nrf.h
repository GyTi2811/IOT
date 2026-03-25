/*
 * nrf.h
 *
 *  Created on: Nov 3, 2025
 *      Author: phamm
 */

#ifndef INC_NRF_H_
#define INC_NRF_H_

#include <stm32h5xx_hal.h>
#include <stdint.h>


#define NRF_CSN_PORT GPIOA
#define NRF_CE_PORT GPIOA

#define NRF_CSN_PIN GPIO_PIN_3
#define NRF_CE_PIN GPIO_PIN_4

#define R_REGISTER 0x00
#define W_REGISTER 0x02
#define R_RX_PAYLOAD 0x61
#define W_TX_PAYLOAD 0xA0
#define FLUSH_TX 0xE1
#define FLUSH_RX 0x62
#define REUSE_TX_PL 0xE3
#define R_RX_PL_WID 0x60
#define W_ACK_PAYLOAD 0xA8
#define W_TX_PAYLOAD_NOACK 0xB0
#define NOP 0xFF

#define CONFIG              0x00
#define EN_AA               0x01
#define EN_RXADDR           0x02
#define SETUP_AW            0x03
#define SETUP_RETR          0x04
#define RF_CH               0x05
#define RF_SETUP            0x06
#define STATUS              0x07
#define OBSERVE_TX          0x08
#define RPD                 0x09
#define RX_ADDR_P0          0x0A
#define RX_ADDR_P1          0x0B
#define RX_ADDR_P2          0x0C
#define RX_ADDR_P3          0x0D
#define RX_ADDR_P4          0x0E
#define RX_ADDR_P5          0x0F
#define TX_ADDR             0x10
#define RX_PW_P0            0x11
#define RX_PW_P1            0x12
#define RX_PW_P2            0x13
#define RX_PW_P3            0x14
#define RX_PW_P4            0x15
#define RX_PW_P5            0x16
#define FIFO_STATUS         0x17
#define DYNPD               0x1C
#define FEATURE             0x1D

void NRF_CSN_SET(void);
void NRF_CSN_RESET(void);
void NRF_CE_SET(void);
void NRF_CE_RESET(void);

uint8_t NRF_Write_Reg(uint8_t reg, uint8_t value);
uint8_t NRF_Read_Reg(uint8_t reg);
void NRF_Write_Buffer(uint8_t reg, uint8_t *data, uint8_t len);
void NRF_Read_Buffer(uint8_t reg, uint8_t *data, uint8_t len);

uint8_t NRF_Send_Command(uint8_t cmd);
uint8_t NRF_Read_Status(void);
uint8_t NRF_Read_FIFO_Status(void);
uint8_t NRF_Read_RX_Payload_Width(void);

void NRF_Flush_TX(void);
void NRF_Flush_RX(void);
void NRF_Clear_IRQ(void);

void NRF_Write_TX_Payload(uint8_t *data, uint8_t len);
void NRF_Read_RX_Payload(uint8_t *data, uint8_t len);
void NRF_Write_ACK_Payload(uint8_t pipe, uint8_t *data, uint8_t len);

void NRF_Mode_TX(void);
void NRF_Mode_RX(void);

void NRF_Set_TX_Address(uint8_t *addr, uint8_t len);
void NRF_Set_RX_Address(uint8_t pipe, uint8_t *addr, uint8_t len);
void NRF_Set_Payload_Width(uint8_t pipe, uint8_t width);

void NRF_Set_Channel(uint8_t channel);
void NRF_Set_DataRate(uint8_t rate);
void NRF_Set_Power(uint8_t level);
void NRF_Enable_CRC(uint8_t crcbyte);

void NRF_PowerUp(void);
void NRF_PowerDown(void);
uint8_t NRF_Check(void);

void NRF_Start_TX(void);
void NRF_Start_RX(void);

void NRF_Init_TX(uint8_t* tx_add);
void NRF_Init_RX(uint8_t* rx_add);

void NRF_Send_Payload(uint8_t* payload,uint8_t len);
uint16_t NRF_Receive_Payload(uint8_t* payload);

#endif /* INC_NRF_H_ */
