/*!*******************************************************************************************
*********************************************************************************************/
///			BIBLIOTECA DE ACESSO AO MODEM
/*!*******************************************************************************************
*********************************************************************************************/

#ifndef _MODEM_SERIAL_H
#define _MODEM_SERIAL_H

#include "rtai_rt_serial.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*	Abre e configura o dispositivo de transmissao. */
static int rt_open_modem(void)
{
	// Abre a serial do pico pic para comunicacao no formato padrao (9600 bps)
	if (rt_open_serial(MODEM_PORT,9600) < 0) {
		//rt_printk("Nao abriu a serial do Modem\n");
            	return -1; // Erro
        }
	
	return 0; // Sucesso
}

#endif

