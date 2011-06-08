/*!*******************************************************************************************
*********************************************************************************************/
///			BIBLIOTECA DA PORTA PARALELA
/*!*******************************************************************************************
*********************************************************************************************/

#ifndef PARALELA_H
#define PARALELA_H

#include <stdio.h>
#include <sys/io.h>

#include "fdc_master.h"
#include "fdc_structs.h"

#define PORTA_PARALELA 0x378

#define LED_HEART_BEAT	0x03 	// Heart-beat
#define LED_START	0x05
#define LED_STOP	0x07

	
/*!*******************************************************************************************
*********************************************************************************************/
///				FUNCOES 
/*!*******************************************************************************************
*********************************************************************************************/
int open_porta_paralela(void);
int led_on(int led);
int led_off(int led);
int led_switch(int led);
int trata_led(void);

int open_porta_paralela(void)
{
	// Pede permissao para escrever na porta paralela
	if (ioperm(PORTA_PARALELA, 3, 1) == -1) {
		perror("ioperm()");
		return 1;
	}
	else 
		return 0; // Sucesso
}

int led_on(int led) 
{	
	global.led_byte |= (1 << led);

	outb(global.led_byte,PORTA_PARALELA);
	return 0;
}

int led_off(int led) 
{
	global.led_byte &= (~(1 << led));

	outb(global.led_byte,PORTA_PARALELA);	
	return 0;
}

int led_switch(int led)
{
	global.led_byte ^= (1 << led);
	
	outb(global.led_byte,PORTA_PARALELA);	
	return 0;
}

/*!*******************************************************************************************
*********************************************************************************************/
/*	Esta funcao implementa a comunicacao HARD-BIT entre o programa modo usuario e o 
modulo de tempo real, com uma taxa de um segundo. Se led = 1 ele eh acesso, e = 0 eh apagado.*/
int trata_led (void) 
{
	parser_cmd_msg_t from_parser;	
	cmd_status_t result;

	// Mensagem de verificacao para saber se o modulo RT esta vivo
	from_parser.msg.cmd = IS_ALIVE;	
	result = sendcommand(&from_parser);
	
	if ((result == NOT_OK) || (result == TIMEOUT)) 
		return 1;	// Modulo de tempo real nao respondeu
	
	if (result == OK) {	// Modulo RT esta vivo
		// Alternando o Heart-bit
		led_switch(LED_HEART_BEAT);
	}
	return 0;
}

#endif

