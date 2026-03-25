/*
 * nrf.c
 *
 *  Created on: Nov 3, 2025
 *      Author: phamm
 */

#include "nrf.h"

extern SPI_HandleTypeDef hspi1;

extern void delayus(uint16_t i);
extern void delayms(uint16_t i);

#define NRF_SPI &hspi1

void NRF_CSN_SET(void){
	HAL_GPIO_WritePin(NRF_CSN_PORT , NRF_CSN_PIN, 1);
}
void NRF_CSN_RESET(void){
	HAL_GPIO_WritePin(NRF_CSN_PORT , NRF_CSN_PIN, 0);
}
void NRF_CE_SET(void){
	HAL_GPIO_WritePin(NRF_CE_PORT , NRF_CE_PIN, 1);
}
void NRF_CE_RESET(void){
	HAL_GPIO_WritePin(NRF_CE_PORT , NRF_CE_PIN, 0);
}

uint8_t NRF_Write_Reg(uint8_t reg, uint8_t value){
	uint8_t status;
	uint8_t temp = reg | W_REGISTER;
	NRF_CSN_RESET();
	HAL_SPI_TransmitReceive(NRF_SPI, &temp, &status, 1, 1000);
	HAL_SPI_Transmit(NRF_SPI, &value, 1, 1000);
	NRF_CSN_SET();
	return status;
}
uint8_t NRF_Read_Reg(uint8_t reg){
	uint8_t dummy = NOP;
	uint8_t temp = reg | R_REGISTER;
	uint8_t data;
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &temp, 1, 1000);
	HAL_SPI_TransmitReceive(NRF_SPI, &dummy, &data, 1, 1000);
	NRF_CSN_SET();
	return data;
}
void NRF_Write_Buffer(uint8_t reg, uint8_t *data, uint8_t len){
	uint8_t temp = reg | W_REGISTER;
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &temp, 1, 1000);
	HAL_SPI_Transmit(NRF_SPI, data, len, 1000);
	NRF_CSN_SET();
}
void NRF_Read_Buffer(uint8_t reg, uint8_t *data, uint8_t len){
	uint8_t dummy =  NOP;
	uint8_t temp = reg | R_REGISTER;
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &temp, 1, 1000);
	for(uint8_t i = 0; i < len; i++){
		HAL_SPI_TransmitReceive(NRF_SPI, &dummy, data + i, 1, 1000);
	}
	NRF_CSN_SET();
}
uint8_t NRF_Send_Command(uint8_t cmd){
	uint8_t status;
	NRF_CSN_RESET();
	HAL_SPI_TransmitReceive(NRF_SPI, &cmd, &status, 1, 1000);
	NRF_CSN_SET();
	return status;
}
uint8_t NRF_Read_Status(void){
	uint8_t status;
	uint8_t dummy = NOP;
	NRF_CSN_RESET();
	HAL_SPI_TransmitReceive(NRF_SPI, &dummy, &status, 1, 1000);
	NRF_CSN_SET();
	return status;
}
uint8_t NRF_Read_FIFO_Status(void){
	return NRF_Read_Reg(FIFO_STATUS);
}
uint8_t NRF_Read_RX_Payload_Width(void){
	uint8_t temp = R_RX_PL_WID;
	uint8_t dummy = NOP;
	uint8_t width = 0;
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &temp, 1, 1000);
	HAL_SPI_TransmitReceive(NRF_SPI, &dummy, &width, 1, 1000);
	NRF_CSN_SET();
	if(width > 32) {
	    NRF_Flush_RX();
	    return 0;
	}
	return width;
}

void NRF_Flush_TX(void){
	NRF_Send_Command(FLUSH_TX);
}
void NRF_Flush_RX(void){
	NRF_Send_Command(FLUSH_RX);
}
void NRF_Clear_IRQ(void){
	uint8_t status = NRF_Read_Status();
	uint8_t clear_irq = status | (0b111 << 4);
	NRF_Write_Reg(STATUS, clear_irq);
}

void NRF_Write_TX_Payload(uint8_t *data, uint8_t len){
	if(len > 32) len = 32;
	uint8_t cmd = W_TX_PAYLOAD;
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &cmd, 1, 1000);
	for(uint8_t i = 0; i< len; i++){
		HAL_SPI_Transmit(NRF_SPI, data + i, 1, 1000);
	}
	NRF_CSN_SET();
}
void NRF_Read_RX_Payload(uint8_t *data, uint8_t len){
	uint8_t cmd = R_RX_PAYLOAD;
	uint8_t dummy = NOP;
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &cmd, 1, 1000);
	for(uint8_t i = 0; i < len; i++){
		HAL_SPI_TransmitReceive(NRF_SPI, &dummy, data + i, 1, 1000);
	}
	NRF_CSN_SET();
}
void NRF_Write_ACK_Payload(uint8_t pipe, uint8_t *data, uint8_t len){
	if(len > 32) len = 32;
	uint8_t cmd = W_ACK_PAYLOAD | (pipe & 0x07);
	NRF_CSN_RESET();
	HAL_SPI_Transmit(NRF_SPI, &cmd, 1, 1000);
	for(uint8_t i = 0; i< len; i++){
		HAL_SPI_Transmit(NRF_SPI, data + i, 1, 1000);
	}
	NRF_CSN_SET();
}

void NRF_Mode_TX(void){
	uint8_t config = NRF_Read_Reg(CONFIG);
	uint8_t tx_config = config & 0xFE;
	NRF_Write_Reg(CONFIG, tx_config);
}
void NRF_Mode_RX(void){
	uint8_t config = NRF_Read_Reg(CONFIG);
	uint8_t rx_config = config | 1;
	NRF_Write_Reg(CONFIG, rx_config);
}

void NRF_Set_TX_Address(uint8_t *addr, uint8_t len){
	NRF_Write_Buffer(TX_ADDR, addr, len);
}
void NRF_Set_RX_Address(uint8_t pipe, uint8_t *addr, uint8_t len){
	switch (pipe){
	case 0:
		NRF_Write_Buffer(RX_ADDR_P0, addr, len);
		break;
	case 1:
		NRF_Write_Buffer(RX_ADDR_P1, addr, len);
		break;
	case 2:
		NRF_Write_Buffer(RX_ADDR_P2, addr, len);
		break;
	case 3:
		NRF_Write_Buffer(RX_ADDR_P3, addr, len);
		break;
	case 4:
		NRF_Write_Buffer(RX_ADDR_P4, addr, len);
		break;
	case 5:
		NRF_Write_Buffer(RX_ADDR_P5, addr, len);
		break;
	}
}
void NRF_Set_Payload_Width(uint8_t pipe, uint8_t width){
	switch (pipe){
	case 0:
		NRF_Write_Reg(RX_PW_P0, width);
		break;
	case 1:
		NRF_Write_Reg(RX_PW_P1, width);
		break;
	case 2:
		NRF_Write_Reg(RX_PW_P2, width);
		break;
	case 3:
		NRF_Write_Reg(RX_PW_P3, width);
		break;
	case 4:
		NRF_Write_Reg(RX_PW_P4, width);
		break;
	case 5:
		NRF_Write_Reg(RX_PW_P5, width);
		break;
	}
}

void NRF_Set_Channel(uint8_t channel){
	NRF_Write_Reg(RF_CH, channel);
}
void NRF_Set_DataRate(uint8_t rate){
	uint8_t rf_setup = NRF_Read_Reg(RF_SETUP);
	uint8_t temp = 0;
	switch (rate){
	case 0: // 00 - 1Mbps
		temp = rf_setup & ~(0x28);
		break;
	case 1: // 01 - 2Mbps
		temp = rf_setup & ~(0x28);
		temp |= 0x08;
		break;
	case 2: // 10 - 250kpbs
		temp = rf_setup & ~(0x28);
		temp |= 0x20;
		break;
	}
	NRF_Write_Reg(RF_SETUP, temp);
}
void NRF_Set_Power(uint8_t level){
	uint8_t rf_setup = NRF_Read_Reg(RF_SETUP);
	uint8_t temp = 0;
	switch (level){
	case 0: // 00 - 1Mbps
		temp = rf_setup & ~(0x06);
		break;
	case 1: // 01 - 2Mbps
		temp = rf_setup & ~(0x06);
		temp |= 0x02;
		break;
	case 2: // 10 - 250kpbs
		temp = rf_setup & ~(0x06);
		temp |= 0x04;
		break;
	case 3:
		temp |= 0x06;
		break;
	}
	NRF_Write_Reg(RF_SETUP, temp);
}
void NRF_Enable_CRC(uint8_t crcbyte){
	uint8_t config = NRF_Read_Reg(CONFIG);
	uint8_t temp = config | 0x08;
	switch (crcbyte){
	case 1:
		temp &= ~(1 << 2);
		break;
	case 2:
		temp |= (1 << 2);
		break;
	}
	NRF_Write_Reg(CONFIG, temp);
}

void NRF_PowerUp(void){
	uint8_t config = NRF_Read_Reg(CONFIG);
	uint8_t temp = config | 0x02;
	NRF_Write_Reg(CONFIG, temp);
	delayms(2);
}
void NRF_PowerDown(void){
	uint8_t config = NRF_Read_Reg(CONFIG);
	uint8_t temp = config & 0xFD;
	NRF_Write_Reg(CONFIG, temp);
}
void NRF_Start_TX(void){
	NRF_CE_SET();
	delayus(15);
	NRF_CE_RESET();
}
void NRF_Start_RX(void){
	NRF_CE_SET();
}

void NRF_Init_TX(uint8_t* tx_add){
	NRF_CE_RESET();
	NRF_PowerUp();
	NRF_Enable_CRC(2);
	NRF_Write_Reg(EN_AA, 0x01);
	NRF_Write_Reg(EN_RXADDR, 0x01);
	NRF_Write_Reg(SETUP_AW, 0x03);
	NRF_Write_Reg(SETUP_RETR, 0x03);
	NRF_Set_Channel(70);
	NRF_Set_DataRate(0x00);
	NRF_Set_Power(0x03);
	NRF_Set_TX_Address(tx_add, 5); // Defualt 5 byte
	NRF_Set_RX_Address(0, tx_add, 5); // Ack payload pipe
	NRF_Write_Reg(DYNPD, 0x01);
	NRF_Write_Reg(FEATURE, 0x07);
	NRF_Mode_TX();
	NRF_Flush_TX();
	NRF_Flush_RX();
	NRF_Clear_IRQ();
}

void NRF_Init_RX(uint8_t* rx_add){
	NRF_CE_RESET();
	NRF_PowerUp();
	NRF_Enable_CRC(2);
	NRF_Write_Reg(EN_AA, 0x01);
	NRF_Write_Reg(EN_RXADDR, 0x01);
	NRF_Write_Reg(SETUP_AW, 0x03);
	NRF_Write_Reg(SETUP_RETR, 0x03);
	NRF_Set_Channel(70);
	NRF_Set_DataRate(0x00);
	NRF_Set_Power(0x03);
	NRF_Set_RX_Address(0, rx_add, 5); // = tx_add
	NRF_Write_Reg(DYNPD, 0x01);
	NRF_Write_Reg(FEATURE, 0x07);
	NRF_Mode_RX();
	NRF_Flush_TX();
	NRF_Flush_RX();
	NRF_Clear_IRQ();
}

void NRF_Send_Payload(uint8_t* payload,uint8_t len){
	NRF_Write_TX_Payload(payload, len);
	NRF_Start_TX();
}

uint16_t NRF_Receive_Payload(uint8_t* payload){
	uint8_t len = NRF_Read_RX_Payload_Width();
	uint8_t pipe = ((NRF_Read_Status() & 0x0E) >> 1);
	NRF_Read_RX_Payload(payload, len);
	NRF_Clear_IRQ();
	return (pipe | (uint16_t)(len << 8));
}
