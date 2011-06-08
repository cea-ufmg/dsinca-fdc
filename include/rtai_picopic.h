/*!*******************************************************************************************
*********************************************************************************************/
///			BIBLIOTECA DE CONTROLE DOS SERVOS
/*!*******************************************************************************************
*********************************************************************************************/

#ifndef _PICOPIC_SERIAL_H
#define _PICOPIC_SERIAL_H

#include "rtai_rt_serial.h"

// Valor default (entre 1 e 255) 
#define PICOPIC_ADRESS 120 

// Valores maximos e minimos de posicionamento dos servos
#define MIN_POSITION  500
#define MAX_POSITION 2500

#define BAUD_COMAND 	0x15
#define SAVEALL_COMAND	0x17

// Enumeracao de possiveis comandos de velocidade da porta serial
typedef enum {
	BPS_1200   = 0x01,
	BPS_2400   = 0x02,
	BPS_9600   = 0x03,	// Valor default
	BPS_19200  = 0x04,
	BPS_38400  = 0x05,
	BPS_57600  = 0x06,
	BPS_76800  = 0x07,
	BPS_115200 = 0x08
} baud_rate_t;

static int rt_picopic_send_serial_comand(int adress, int command, int opcao1, int opcao2, int opcao3);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/* Envia comando de posicao e velocidade para o servo "servo_number" 
	servo_number de 1 a 20
	position de 500 a 2500
	speed de 1 (mais lento) a 255 (mais rapido) (0 eh o mais rapido possivel)*/
static int rt_picopic_servo_move_comand(int servo_number, int position, int speed)
{
	int position_h, position_l;
	
	// Confina o valor de position dentro dos limites 
	if (position < MIN_POSITION)
		position = MIN_POSITION;
	if (position > MAX_POSITION)
		position = MAX_POSITION;
		
	// Separa a posicao desejada em dois bytes que serao enviados pela serial
	position_h = (position & 0xFF00) >> 8; 	// MSB
	position_l =  position & 0xFF;		// LSB
	
	// Envia o comando pela porta serial e retorna o resultado
	return rt_picopic_send_serial_comand(PICOPIC_ADRESS, servo_number, position_h, position_l, speed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Altera a velocidade da comunicacao serial no pico pic.*/
static int rt_picopic_change_baud_rate (baud_rate_t new_baud)
{	
	return rt_picopic_send_serial_comand(PICOPIC_ADRESS, BAUD_COMAND, (int)new_baud, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Salva todas as configuracao dos servos, e exige que o REBOOT esteja ativo.*/
static int rt_picopic_save_all_comands (void)
{	
	return rt_picopic_send_serial_comand(PICOPIC_ADRESS, SAVEALL_COMAND, 0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/* 	Envia um comando padrao pela por serial para o pico pic em tempo real
	- adress eh sempre o endereco do pico pic
	- command eh o comando desejado 
	- opcao1, 2 e 3 sao as opcoes, dependendo do tipo de comando requisitado*/
static int rt_picopic_send_serial_comand(int adress, int command, int opcao1, int opcao2, int opcao3)
{
	// Envia primeiro o endereco do controlador de servos
	if (rt_putch_serial(PICO_PIC, adress) == 0)
		return -1;
	
	// Envia o numero do servo que se quer controlar
	if (rt_putch_serial(PICO_PIC, command) == 0)
		return -1;
	
	// Envia os bytes de opcao escolhidos
	if (rt_putch_serial(PICO_PIC, opcao1) == 0)
		return -1;
	if (rt_putch_serial(PICO_PIC, opcao1) == 0)
		return -1;
	if (rt_putch_serial(PICO_PIC, opcao1) == 0)
		return -1;
	
	return 0; // Sucesso
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Abre e configura o dispositivo de controle dos servo-motores. */
static int rt_open_picopic(void)
{
	// Abre a serial do pico pic para comunicacao no formato padrao (9600 bps)
	if (rt_open_serial(PICO_PIC,9600) < 0) {
		rt_printk("Nao abriu a serial do Controlador de Servos\n");
            	return -1; // Erro
        }
	
	// Configura o pico pic para a comunicacao a 115200 bps (mais veloz)
	if (rt_picopic_change_baud_rate (BPS_115200) < 0) {
		rt_printk("Nao configurou a serial do Controlador de Servos\n");
		rt_close_serial(PICO_PIC); // Fecha a porta serial
            	return -1; // Erro
        }
	// Fecha a porta serial para a configuracao padrao
	rt_close_serial(PICO_PIC); 
	
	// Agora abre a serial do pico pic para comunicacao a 115200
	if (rt_open_serial(PICO_PIC,115200) < 0) {
		rt_printk("Nao abriu a serial do Controlador de Servos novamente\n");
		rt_picopic_save_all_comands (); // Salva a nova configuracao
            	return -1; // Erro
        }
	
	return 0; // Sucesso
}

#endif

