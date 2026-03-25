#include "nrf.h"

void 				CS_Low						(void){
	HAL_GPIO_WritePin(NRF_CS_GPIO, NRF_CS_PIN, 0);
}
void 				CS_High						(void){
	HAL_GPIO_WritePin(NRF_CS_GPIO, NRF_CS_PIN, 1);
}

void 				CE_Low						(void){
	HAL_GPIO_WritePin(NRF_CE_GPIO, NRF_CE_PIN, 0);
}
void 				CE_High						(void){
	HAL_GPIO_WritePin(NRF_CE_GPIO, NRF_CE_PIN, 1);
}

void 				NRF_W_Reg					(uint8_t Reg, uint8_t Data){
	uint8_t Buffer[2];
	Buffer[0] = Reg | (1 << 5);
	Buffer[1] = Data;
	
	CS_Low();
	HAL_SPI_Transmit(NRF_SPI, Buffer, 2, 1000);
	CS_High();
}
void				NRF_Wx_Reg				(uint8_t Reg, uint8_t *Data, uint8_t Size){
	uint8_t Buffer = Reg | (1 << 5);
	
	CS_Low();
	HAL_SPI_Transmit(NRF_SPI, &Buffer, 1, 100);
	HAL_SPI_Transmit(NRF_SPI, Data, Size, 1000);
	CS_High();
}

uint8_t 		NRF_R_Reg					(uint8_t Reg){
	uint8_t Data = 0;
	
	CS_Low();
	HAL_SPI_Transmit(NRF_SPI, &Reg, 1, 100);
	HAL_SPI_Receive(NRF_SPI, &Data, 1, 100);
	CS_High();
	
	return Data;
}
void				NRF_Rx_Reg				(uint8_t Reg, uint8_t *Data, uint8_t Size){
	CS_Low();
	HAL_SPI_Transmit(NRF_SPI, &Reg, 1, 100);
	HAL_SPI_Receive(NRF_SPI, Data, Size, 1000);
	CS_High();
}

void				NRF_Send_CMD			(uint8_t Cmd){
	CS_Low();
	HAL_SPI_Transmit(NRF_SPI, &Cmd, 1, 100);
	CS_High();
}

void				NRF_Init					(void){
	CE_Low();
	
	NRF_Reset(0);
	
	NRF_W_Reg(CONFIG, 0);
	
	NRF_W_Reg(EN_AA, 0);
	
	NRF_W_Reg(EN_RXADDR, 0);
	
	NRF_W_Reg(SETUP_AW, 0x03);
	
	NRF_W_Reg(SETUP_RETR, 0);
	
	NRF_W_Reg(RF_CH, 0);
	
	NRF_W_Reg(RF_SETUP, 0x0E);
	
	CE_High();
}
void				NRF_Reset					(uint8_t Register){
	if(Register == STATUS){
		
		NRF_W_Reg(STATUS, 0x00);
		
	}else if(Register == FIFO_STATUS){
		
		NRF_W_Reg(FIFO_STATUS, 0x11);
		
	} else {
		
		uint8_t rx_addr_p0_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
		uint8_t rx_addr_p1_def[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
		uint8_t tx_addr_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
		
		NRF_W_Reg(CONFIG, 0x08);
		NRF_W_Reg(EN_AA, 0x3F);
		NRF_W_Reg(EN_RXADDR, 0x03);
		NRF_W_Reg(SETUP_AW, 0x03);
		NRF_W_Reg(SETUP_RETR, 0x03);
		NRF_W_Reg(RF_CH, 0x02);
		NRF_W_Reg(RF_SETUP, 0x0E);
		NRF_W_Reg(STATUS, 0x00);
		NRF_W_Reg(OBSERVE_TX, 0x00);
		NRF_W_Reg(RPD, 0x00);
		
		NRF_Wx_Reg(RX_ADDR_P0, rx_addr_p0_def, 5);
		NRF_Wx_Reg(RX_ADDR_P1, rx_addr_p1_def, 5);
		NRF_W_Reg(RX_ADDR_P2, 0xC3);
		NRF_W_Reg(RX_ADDR_P3, 0xC4);
		NRF_W_Reg(RX_ADDR_P4, 0xC5);
		NRF_W_Reg(RX_ADDR_P5, 0xC6);
		
		NRF_Wx_Reg(TX_ADDR, tx_addr_def, 5);
		
		NRF_W_Reg(RX_PW_P0, 0);
		NRF_W_Reg(RX_PW_P1, 0);
		NRF_W_Reg(RX_PW_P2, 0);
		NRF_W_Reg(RX_PW_P3, 0);
		NRF_W_Reg(RX_PW_P4, 0);
		NRF_W_Reg(RX_PW_P5, 0);
		
		NRF_W_Reg(FIFO_STATUS, 0x11);
		NRF_W_Reg(DYNPD, 0);
		NRF_W_Reg(FEATURE, 0);
	}
}

void				NRF_PTX_Begin			(uint8_t *Address, uint8_t Channel){
	CE_Low();
	
	NRF_W_Reg(RF_CH, Channel);
	
	NRF_Wx_Reg(TX_ADDR, Address, 5);
	
	uint8_t config = NRF_R_Reg(CONFIG);
	config = config | (1<<1);
	config &= ~(1 << 0);
	NRF_W_Reg(CONFIG, config);
	
	CE_High();
}
uint8_t				NRF_Transmit			(uint8_t *Data){
	uint8_t cmd = 0;
	
	CS_Low();
	cmd = W_TX_PAYLOAD;
	HAL_SPI_Transmit(NRF_SPI, &cmd, 1, 100);
	HAL_SPI_Transmit(NRF_SPI, Data, 32, 1000);
	CS_High();
	
	HAL_Delay(1);
	
	uint8_t fifostatus = NRF_R_Reg(FIFO_STATUS);
	
	if((fifostatus&(1<<4)) && (!(fifostatus&(1<<3)))){
		cmd = FLUSH_TX;
		NRF_Send_CMD(cmd);
		NRF_Reset(FIFO_STATUS);
		return 1;
	}
	return 0;
}

void				NRF_PRX_Begin			(uint8_t *Address, uint8_t Channel){
	CE_Low();
	NRF_Reset(STATUS);
	NRF_W_Reg(RF_CH, Channel);
	
	NRF_W_Reg(EN_RXADDR, 0x01);
	NRF_Wx_Reg(RX_ADDR_P0, Address, 5);
	
	uint8_t config = NRF_R_Reg(CONFIG);
	config = config | (1 << 1) | (1 << 0);
	NRF_W_Reg(CONFIG, config);
	CE_High();
}
void				NRF_Receive				(uint8_t *Data){
	uint8_t cmd =0;
	CS_Low();
	cmd = R_RX_PAYLOAD;
	HAL_SPI_Transmit(NRF_SPI, &cmd, 1, 100);
	HAL_SPI_Receive(NRF_SPI, Data, 32, 1000);
	CS_High();
	HAL_Delay(1);
	cmd = FLUSH_RX;
	NRF_Send_CMD(cmd);
}
uint8_t			NRF_isData				(uint8_t Pipe){
	uint8_t status = NRF_R_Reg(STATUS);
	if ((status&(1<<6))&&(status&(Pipe<<1))){
		NRF_W_Reg(STATUS, 1<<6);
		return 1;
	}
	return 0;
}