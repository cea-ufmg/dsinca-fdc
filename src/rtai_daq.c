#include "rtai_daq.h"

MODULE_AUTHOR("Guilherme A. S. Pereira and Armando Alves Neto");
MODULE_DESCRIPTION("Real time data acquisition of PC104 DAC");
MODULE_LICENSE("GPL");

/*!/////////////////////////////////////////////////////////////////////////////////////////////
 *  Inicio do modulo da daq
 */
static int __rtai_daq_init(void)
{
    InitHw(BASE_ADRESS, VCMDAS1_PM5, VCMDAS1_PM5, VCMDAS1_PM5);   
    return 0;
}

/*!/////////////////////////////////////////////////////////////////////////////////////////////
 * Fim do modulo da daq
 */
static void __rtai_daq_exit(void)
{
}

module_init(__rtai_daq_init);
module_exit(__rtai_daq_exit);

/*!///////////////////////////////////////////////////////////////////////////////////////////
 * Efetua a leitura dos 16 canais e prenche as estruturas da mensagem da fifo e do modem
 */ 
int rt_process_daq_16(msg_daq_t* msg)
{
    int i, ret;
    int invalido = 0; // Validade dos dados coletados
    float valor = 0.0;

    for (i=0; i<16; i++)
        {        
            ret = getChannelVolts(i, &valor);
            
            if (ret != SSL_ERR_NOERROR){
                    invalido = 1;    //Dados coletados sao invalidos
	    }
            
            msg->tensao[i] = valor;

            valor = 0.0;
            
        }
        
    if (invalido)
        return 0;
    else
        return 1;        
}

/*!////////////////////////////////////////////////////////////////////////////////////////////
 *  InitHw
 *
 *  Input:
 *    base_addr     - the base address for the first contiguous card
 *    ain_range     - the data range to use
 *    aout_0_range  - output voltage range channel 0
 *    aout_1_range  - outpur voltage range channel 1
 *
 *  Output:
 *    (returns)   - error code
 *
 *  Ex: InitHw(BASE_ADRESS, VCMDAS1_PM5, VCMDAS1_PM5, VCMDAS1_PM5);
 */
int InitHw(int base_addr, int ain_range, int aout_0_range, int aout_1_range)
{
    int ret_val = SSL_ERR_NOERROR;

    VCMDAS1.initialized = SSL_TRUE;
    VCMDAS1.ain_range = ain_range;
        VCMDAS1.ain_offset = 0;
        VCMDAS1.ain_gain = 0;
    VCMDAS1.aout_range[0] = aout_0_range;
        VCMDAS1.aout_range[1] = aout_1_range;
    VCMDAS1.aout_offset[0] = 0;
    VCMDAS1.aout_offset[1] = 0;
    VCMDAS1.aout_gain[0] = 0;
    VCMDAS1.aout_gain[1] = 0;
            
    if (ResetCard())
        ret_val = SSL_ERR_TIMEOUT;

    return(ret_val);
}


/*!////////////////////////////////////////////////////////////////////////////////////////////
 *  ResetCard
 *
 *  This procedure resets the Analog Input, Analog Output, and Digital I/O
 *  circuits on a specific VCM-DAS-1 card.  The card is restored to it's
 *  power-on reset state as summarized below:
 *
 *  ANALOG INPUT                            ANALOG OUTPUT
 *  ------------                            -------------
 *  - Channel 0 is selcted                  - All Channels set to 0 Volts
 *  - Auto Increment Disabled
 *  - Auto Trigger Disabled                 PARALLEL I/O
 *  - Scan Range Limit = Unrestricted       ------------
 *  - A/D Interrupts Disabled               - All Channels set to Input
 *  - A/D Interrupt Request Cleared         - Parallel Interrupts Disabled
 *                                                                  - Parallel Interrupt Request Cleared
 *  EEPROM
 *  ------
 *  - Enables writes to the EEPROM
 *
 *  Input:
 *    card      - the card number to reset
 *
 *  Output:
 *    (returns) - error code
*/
int ResetCard()
{
    short data;
    int ret_val;

   /*---------------------------
    * reset control register to
    * power-on value
    *---------------------------*/
    SSL_OUT(BASE_ADRESS+CONTROL, 0);

    /*---------------------------
    * reset analog input
    *---------------------------*/
    SSL_OUT(BASE_ADRESS, 0);
    ret_val = AnaIn(0, &data);

 return (ret_val);
}

/*!////////////////////////////////////////////////////////////////////////////////////////////
 *  AnaIn
 *
 *  This procedure reads an input voltage from a channel without any
 *  conversion.
 *
 *  Input:
 *    channel   - the channel number
 *
 *  Output:
 *    (returns) - error code
 *    code      - A/D data from the hardware
*/
int AnaIn(int channel, short *code)
{
     unsigned int done;
     unsigned int timedout;
     short data;

    /*---------------------------
    *  select channel and start the conversion
    *---------------------------*/
    SSL_OUT(BASE_ADRESS+SELECT, channel);
    SSL_OUT(BASE_ADRESS+CONVERT, 0x01);

    /*---------------------------
    *  wait for conversion with a timeout loop
    *---------------------------*/
    done = SSL_IN(BASE_ADRESS+STATUS) & DONE_BIT;

    for( timedout = 8000; !done && timedout; timedout-- )
        done = SSL_IN(BASE_ADRESS + STATUS) & DONE_BIT;

    /*---------------------------
    *  return 16-bit data
    *---------------------------*/
    if (!done)
        return SSL_ERR_TIMEOUT;
    else
    {
        *code = 0;
        
        data = SSL_IN(BASE_ADRESS+ADCLO);
        data = data & 0xFF;
        *code = data;        
        
        data = SSL_IN(BASE_ADRESS+ADCHI);
        data = data & 0xFF;
        *code += (data << 8);
    }
    
    *code &= 0xFFFF;

    return SSL_ERR_NOERROR;
}


/*!////////////////////////////////////////////////////////////////////////////////////////////
    Captura o valor de um canal em volts

    Input:
         channel - canal
         data - valor em bits
         
    Output:
        (returns) - error code
*/
int getChannelVolts(int channel, float *volts)
{
    short data;
    int ret = AnaIn(channel, &data);
    
    *volts = 100.0;
    
    switch (VCMDAS1.ain_range) 
    {
        case VCMDAS1_PM10:
            *volts = (20.0f/65536.0f)*data;
        break;
        
        case VCMDAS1_PM5:
            *volts = (10.0f/65536.0f)*data;
    }
    
    return ret; // O retorno é o valor raw do canal
}

