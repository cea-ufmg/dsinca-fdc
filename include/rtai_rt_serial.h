/*!*******************************************************************************************
*********************************************************************************************/
///            BIBLIOTECA DA SERIAL EM TEMPO REAL
/*!*******************************************************************************************
*********************************************************************************************/
#ifndef _RT_SERIAL_H
#define _RT_SERIAL_H

#include <rtai_serial.h>

/// Definicao atribuidas pela configuracao da placa PC104
#define AHRS_PORT       0
#define GPS_PORT        1
#define NAV_PORT        2
#define PITOT_PORT      3
#define MODEM_PORT      5

/*!*******************************************************************************************
*********************************************************************************************/
///    FUNCOES DE INTERACAO COM A PORTA SERIAL EM TEMPO REAL
/*!*******************************************************************************************
*********************************************************************************************/
/*    Abre a serial em tempo real.
    - fd eh o dispositivo serial (COM1 ou COM2)
    - rate eh a taxa de comunicacao */
static inline int rt_open_serial(int fd, int rate)
//! Abre a porta serial
{
    return rt_spopen(    fd,
                rate, //9600,//115200,
                8,
                1 /*um stop bit */,
                RT_SP_PARITY_NONE,
                RT_SP_NO_HAND_SHAKE,
                RT_SP_FIFO_SIZE_8    );
}
//////////////////////////////////////////////////////////////////////////////////////////////
static inline int rt_close_serial(int fd)
//! Fecha a porta serial
{
    return rt_spclose(fd);
}
//////////////////////////////////////////////////////////////////////////////////////////////
static inline int rt_bytes_avail_serial(int fd)
//! Returns whether there are bytes remaining in the incoming serial queue.
{
    if (rt_spget_rxavbs(fd) > 0) // Get how many chars are in receive buffer
        return 1;
    else
        return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////
static inline int rt_clear_serial(int fd)
//! Limpa a fila de recepcao da IMU
{
    return  rt_spclear_rx(fd);    //Clear all received chars in buffer and inside UART FIFO
}
//////////////////////////////////////////////////////////////////////////////////////////////
static inline unsigned int rt_getch_serial(int fd)
//! Retorna um byte do buffer da serial
{
    char tmpbyte;

    if (rt_spread( fd, &tmpbyte, 1) == 0)    //Le um byte e retorna o numero de chars NAO lidos
        return (unsigned int)tmpbyte;
    else
        return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////
static inline int rt_putch_serial(int fd, int Byte)
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
static inline void rt_flush_serial(int fd)
//! Flushes the serial output (transmits any pending characters).
{
    /*// Get how many chars are in transmit buffer waiting to be sent by UART
    while (rt_spget_txfrbs(IMU) > 0) { 
        rt_sleep(nano2count(1000000)); //Dorme 1 ms
    }*/
}
//////////////////////////////////////////////////////////////////////////////////////////////
#endif

