/*!*******************************************************************************************
*********************************************************************************************/
///				BIBLIOTECA DA PLACA DAQ
/*!*******************************************************************************************
*********************************************************************************************/
#ifndef RTAI_DAQ_H
#define RTAI_DAQ_H

// Inclue o cabecalho de mensagens para utilizar msg_daq
#include "messages.h"

//#include <sys/io.h>
#include <stdio.h>

#define PORTS_PER_CARD     16

#define BASE_ADRESS		0x300	/* Endereco da placa*/
#define CARD			0

#define STATUS          0x00    /*  Read port */
#define CONTROL         0x00    /*  Write port */
#define SELECT          0x01    /*  Write port */
#define CONVERT         0x02    /*  Write port */
#define TDELAY          0x03    /*  Write port */
#define ADCLO           0x04    /*  Read only port */
#define ADCHI           0x05    /*  Read only port */
#define DIGLO           0x06    /*  Digital low - read/write */
#define DIGHI           0x07    /*  Digital high - read/write */
#define SERCS           0x08    /*  Serial Chip select - write */
#define SERSTAT         0x09    /*  Serial status - read port */
#define SERDATA         0x09    /*  Serial data send and clock - write port */

#define DONE_BIT        0x40    /* bit mask for A/D conversion complete */
#define BUSY_BIT        0x80    /* bit mask for A/D converion busy */
#define DA              0x01    /* bit mask for D/A chip select */
#define DPOT            0x02    /* bit mask for DPOT chip select */
#define EEPROM          0x04    /* bit mask for EEPROM chip select */

#define AINPUT_CG 	 	 0x7ffbL /* value read for gain calibration */
#define AOUTPUT_RB	 	 0x7fffL /* value read back for full scale output */

// Funcoes para acesso de hardware
#define SSL_OUT(port,data)	outb(data,port)
#define SSL_OUTW(port,data) outw(data,port)
#define SSL_IN(port)		inb(port) 
#define SSL_INW(port)		inw(port)

/*----------------------------------------------------------------------
 *  Misc. Definitions
 *----------------------------------------------------------------------*/
#define SSL_STRICT
#define SSL_TRUE	      1
#define SSL_FALSE	      0
#define SSL_NULL	      0
#define SSL_ON		      1
#define SSL_OFF 	      0

/*----------------------------------------------------------------------
 *  Error Code Definitions
 *----------------------------------------------------------------------*/
#define SSL_ERR_NOERROR       0
#define SSL_ERR_GEN_FAIL      1
#define SSL_ERR_BUSY	      2
#define SSL_ERR_TIMEOUT       3
#define SSL_ERR_BAD_MASK      4
#define SSL_ERR_OVERFLOW      5
#define SSL_ERR_UNDERFLOW     6
#define SSL_ERR_TEST_FAIL     7

#define SSL_ERR_BADARG1       11
#define SSL_ERR_BADARG2       12
#define SSL_ERR_BADARG3       13
#define SSL_ERR_BADARG4       14
#define SSL_ERR_BADARG5       15
#define SSL_ERR_BADARG6       16
#define SSL_ERR_BADARG7       17
#define SSL_ERR_BADARG8       18
#define SSL_ERR_BADARG9       19

/*----------------------------------------------------------------------
 *  VERSALOGIC VCM-DAS-1 DEVICE DRIVER CONSTANTS
 *----------------------------------------------------------------------*/
#define VCMDAS1_PM5                     0
#define VCMDAS1_PM10                    1

#define VCMDAS1_DATA_BUS_WIDTH_8        0
#define VCMDAS1_DATA_BUS_WIDTH_16       1

#define VCMDAS1_CSDA 0x01
#define VCMDAS1_CSDP 0x02
#define VCMDAS1_CSEE 0x04
#define VCMDAS1_CSDL 0x08

/* Digital Pot addresses */

#define VCMDAS1_DP_AD_OFFSET 	0x00
#define VCMDAS1_DP_AD_GAIN		0x02
#define VCMDAS1_DP_DA0_GAIN 	0x01
#define VCMDAS1_DP_DA1_GAIN 	0x03

/*--------------------------------------------------------------------------
 *  MACRO DEFINITIONS
 *-------------------------------------------------------------------------*/

void OUT_SAVEW(unsigned int p, unsigned int d);
#define OUT_SAVEW(p,d)(\
							SSL_OUTW(BASE_ADRESS + (p), (d)),\
							VCMDAS1.out_val = (d))

/*--------------------------------------------------------------------------
 *  STRUCTURE DEFINITIONS
 *-------------------------------------------------------------------------*/
struct VCMDAS1_info 
{
	 unsigned char initialized;
	 unsigned int ain_range;
	 unsigned int ain_offset;
	 unsigned int ain_gain;
	 unsigned int aout_range[2];
	 unsigned int aout_offset[2];
	 unsigned int aout_gain[2];
	 unsigned int out_val;
};

struct VCMDAS1_info VCMDAS1;

/*--------------------------------------------------------------------------
 *  DEFINIÇÃO DAS FUNÇÕES
 *-------------------------------------------------------------------------*/
int rt_process_daq_16(msg_daq_t* msg);

int InitHw(int base_addr, int ain_range, int aout_0_range, int aout_1_range);

int ResetCard();

int AnaIn(int channel, short *code);

int getChannelVolts(int channel, float *volts);

#endif

