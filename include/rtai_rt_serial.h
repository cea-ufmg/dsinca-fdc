/*!*******************************************************************************************
*********************************************************************************************/
///			BIBLIOTECA DA SERIAL EM TEMPO REAL
/*!*******************************************************************************************
*********************************************************************************************/
#ifndef _RT_SERIAL_H
#define _RT_SERIAL_H

#include <rtai_serial.h>

/// Definicao atribuidas pela configuracao da placa PC104
#define AHRS_PORT		0
#define GPS_PORT		1
#define NAV_PORT		2
#define PITOT_PORT		3
//#define MODEM_PORT 	 	COM2
//#define PICO_PIC_PORT 		COM2

//Define serial sync characters para a transmissao via modem
//#define SYNC_SER_0 ((char) 0x55)
//#define SYNC_SER_1 ((char) 0xAA)
	
/*!*******************************************************************************************
*********************************************************************************************/
///				FUNCOES DA SERIAL
/*!*******************************************************************************************
*********************************************************************************************/
int rt_clear_serial(int fd);

unsigned int rt_getch_serial(int fd);

int rt_bytes_avail_serial(int fd);

int rt_putch_serial(int fd, int Byte);

void rt_flush_serial(int fd);

/*!*******************************************************************************************
*********************************************************************************************/
///	FUNCOES DE INTERACAO COM A PORTA SERIAL EM TEMPO REAL
/*!*******************************************************************************************
*********************************************************************************************/
/*	Abre a serial em tempo real.
	- fd eh o dispositivo serial (COM1 ou COM2)
	- rate eh a taxa de comunicacao */
int rt_open_serial(int fd, int rate)
//! Abre a porta serial
{
	return rt_spopen(	fd,
				rate, //9600,//115200,
				8,
				1 /*um stop bit */,
				RT_SP_PARITY_NONE,
				RT_SP_NO_HAND_SHAKE,
				RT_SP_FIFO_SIZE_8	);
}
//////////////////////////////////////////////////////////////////////////////////////////////
int rt_close_serial(int fd)
//! Fecha a porta serial
{
	return rt_spclose(fd);
}
//////////////////////////////////////////////////////////////////////////////////////////////
int rt_bytes_avail_serial(int fd)
//! Returns whether there are bytes remaining in the incoming serial queue.
{
	if (rt_spget_rxavbs(fd) > 0) // Get how many chars are in receive buffer
		return 1;
	else
		return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int rt_clear_serial(int fd)
//! Limpa a fila de recepcao da IMU
{
	return  rt_spclear_rx(fd);	//Clear all received chars in buffer and inside UART FIFO
}
//////////////////////////////////////////////////////////////////////////////////////////////
unsigned int rt_getch_serial(int fd)
//! Retorna um byte do buffer da serial
{
	char tmpbyte;

	if (rt_spread( fd, &tmpbyte, 1) == 0)	//Le um byte e retorna o numero de chars NAO lidos
		return (unsigned int)tmpbyte;
	else
		return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int rt_putch_serial(int fd, int Byte)
//! Transmite um byte pela serial
{
	char chrByte;

	chrByte = (char)Byte;
	if (rt_spwrite(fd, &chrByte, sizeof(chrByte)) != 0) { // Retorna o numero de chars NAO enviados
		//printk("- Problemas de transmissao !\n");
		return 0;
	}
	else
		return 1; // Sucesso
}
//////////////////////////////////////////////////////////////////////////////////////////////
void rt_flush_serial(int fd)
//! Flushes the serial output (transmits any pending characters).
{
	/*// Get how many chars are in transmit buffer waiting to be sent by UART
	while (rt_spget_txfrbs(IMU) > 0) { 
		rt_sleep(nano2count(1000000)); //Dorme 1 ms
	}*/
}
//////////////////////////////////////////////////////////////////////////////////////////////
#endif

