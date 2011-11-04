#include "rtai_imu.h"

MODULE_AUTHOR("Armando Alves Neto");
MODULE_DESCRIPTION("Real time data acquisition of IMU");
MODULE_LICENSE("GPL");

/*!*******************************************************************************************
*********************************************************************************************/
///                FUNCOES DA IMU
/*!*******************************************************************************************
*********************************************************************************************/
//!Process the serial state machine to receive IMU packets.
//! This routine can be called multiple times to accumulate a packet.
int rt_process_imu_serial(unsigned char* MessageBuffer, int *type_message)
{
    unsigned char state = 0;      // State machine para processar pacotes da serial
    int ch;            // Byte corrente da serial

    static unsigned char MessageType;            // Tipo da mensagem da IMU
    static unsigned char MessageLength;            // Tamanho da mensagem da IMU
    static unsigned char MessageIndex = 0;            // Index da mensagem corrente

    static unsigned char MessageCRC16H;            //CRC16 high byte
    static unsigned char MessageCRC16L;            //CRC16 low byte

    unsigned short MessageCRC16;
    unsigned short ReceivedCRC16;

    while (rt_bytes_avail_serial(IMU_PORT)) // Avalia se existem bytes a serem lidos
    {
        //Get a byte from the serial port.
        ch = rt_getch_serial(IMU_PORT);
        ch=ch&0xFF;

        switch (state)
        {
            case 0:  //Look for the first sync character
                if (ch == SYNC_IMU_SER_0)
                {
                    state++;
                    MessageBuffer[MessageIndex++] = ch;  //Save the byte
                }
            break;
            /////////////////////////////////////////////////////////////////////////
            case 1:  //Look for the second sync character
                if (ch == SYNC_IMU_SER_1)
                {
                    state++;
                    MessageBuffer[MessageIndex++] = ch;  //Save the byte
                }
            break;
            /////////////////////////////////////////////////////////////////////////
            case 2:  //Get the message type
                MessageType = ch;
                MessageBuffer[MessageIndex++] = ch;  //Save the byte
                state++;
                *type_message = ch;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 3:  //Get the message length
                MessageLength = ch;
                MessageBuffer[MessageIndex++] = ch;  //Save the byte

                //     If the message is too big then this packet is invalid and we need to start
                // looking for another packet.
                if ((MessageLength + MESSAGE_HEADER_LEN + 2) > MAX_MESSAGE_LEN)
                    state=0;
                else
                    state++;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 4:  //Get the message body
                MessageBuffer[MessageIndex++] = ch;  //Save the message body bytes

                //If we have received all the message body bytes, go on.
                if (MessageIndex >= (MessageLength+MESSAGE_HEADER_LEN))
                    state++;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 5:  //Get the CRCH byte.
                MessageCRC16H = ch;
                state++;
            break;
            /////////////////////////////////////////////////////////////////////////
            case 6:  //Get the CRCL byte.
                MessageCRC16L = ch;

                //We now have a complete packet. Verify the checksum and process it.
                //Convert individual bytes to one 16 bit checksum.
                MessageCRC16 = ((unsigned short)MessageCRC16H<<8)|(unsigned short)MessageCRC16L;

                ReceivedCRC16 = CRC16(MessageBuffer,MessageLength+MESSAGE_HEADER_LEN);

                state = 0;
                MessageIndex = 0;
                rt_clear_serial(IMU); // Limpa a fila da serial
                
                if (MessageCRC16 == ReceivedCRC16) {    // Avalia o sucesso da funcao
                    return 1;    // Sucesso na obtencao dos dados
                }
                else {
                //Regardless whether the packet was good or bad, it's time to go look for another one.
                    return 0;    // Fracasso na obtencao dos dados
                }
            break;
            /////////////////////////////////////////////////////////////////////////
        } //Switch
    } //while bytes available
    return -1;    // Sem dados na serial
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
int rt_convert_imu_data(msg_imu_t* msg, unsigned char msgbuf[])
{
    signed short v1,v2,v3;
    unsigned long TimerTicksSincePPS;      //!< Timer ticks since GPS Pulse Per Second
        
    v1 = (int)((int)msgbuf[4]<<8|msgbuf[5]);
    v2 = (int)((int)msgbuf[6]<<8|msgbuf[7]);
    v3 = (int)((int)msgbuf[8]<<8|msgbuf[9]);

    msg->gyro[X_AXIS] = (float)v1/CONV_GYRO_RESOLUTION;
    msg->gyro[Y_AXIS] = (float)v2/CONV_GYRO_RESOLUTION;
    msg->gyro[Z_AXIS] = (float)v3/CONV_GYRO_RESOLUTION;
    
    v1 = (int)((int)msgbuf[10]<<8|msgbuf[11]);
    v2 = (int)((int)msgbuf[12]<<8|msgbuf[13]);
    v3 = (int)((int)msgbuf[14]<<8|msgbuf[15]);

    msg->accel[X_AXIS] = (float)v1/CONV_ACCEL_RESOLUTION;
    msg->accel[Y_AXIS] = (float)v2/CONV_ACCEL_RESOLUTION;
    msg->accel[Z_AXIS] = (float)v3/CONV_ACCEL_RESOLUTION;
    
    // Calcula o time_stamp da IMU (nao do sistema)
    TimerTicksSincePPS =     ((unsigned long)msgbuf[16])<<24 | ((unsigned long)msgbuf[17])<<16 |
                ((unsigned long)msgbuf[18])<< 8 | ((unsigned long)msgbuf[19]);

    //Calcula o time since PPS
    msg->time_stamp = (double)TimerTicksSincePPS/((double)TIMER_FREQUENCY_HZ);

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Esta funcao transmite qualquer pacote de dados atraves da porta serial da IMU
    - MessageType corresponde ao tipo de mensagem enviada
    - pMessageBuffer contem a mensagem a ser enviada
    - len eh o tamanho da mensagem enviada*/
void rt_transmit_serial_message(int fd, unsigned char MessageType, unsigned char* pMessageBody,unsigned char len)
{
    //Buffer for the complete message including header and CRCs.
    unsigned char MessageBuffer[MAX_MESSAGE_LEN];  
    
    unsigned char idx=0;
    unsigned short MessageCRC;
    unsigned char i;

    if (len>8)  //Sanity check on message size (all messages to the IMU are 8 bytes or less).
        return;

    // Constroi o cabecalho da mensagem
    MessageBuffer[idx++]=SYNC_IMU_SER_0;
    MessageBuffer[idx++]=SYNC_IMU_SER_1;
    MessageBuffer[idx++]=MessageType;
    MessageBuffer[idx++]=len;

    //Copy the message body.
    for(i=0 ; i<len ; i++) 
        MessageBuffer[idx++] = pMessageBody[i];
        
    // Empacota o CRC
    MessageCRC = CRC16(MessageBuffer,len+MESSAGE_HEADER_LEN);
    MessageBuffer[idx++]=(unsigned char)(MessageCRC>>8);    //High byte
    MessageBuffer[idx++]=(unsigned char)(MessageCRC&0xFF);    //Low byte

    //Transmit the data
    for (i=0;i<idx;++i)
        rt_putch_serial(fd,MessageBuffer[i]);

    //Flush the data out to the serial port.
    rt_flush_serial(fd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Envia mensagem que seta os parametros da IMU.
    - SampleRateHz eh a taxa de amostragem em Hz
    - OversampleRatio eh a taxa de superamostragem - Geralmente vale 255 
    - CommsMode eh o modo de comunicacao (neste caso serial)
    - SampleMode eh a forma como os dados sao amostrados (convertidos ou crus)*/
void rt_send_imu_settings_serial_message(float SampleRateHz,unsigned short OversampleRatio,unsigned char CommsMode, unsigned char SampleMode)
{
    unsigned char Msg[8];  //Buffer for the message body.
    unsigned long SamplePeriod; //microsec
    unsigned char idx=0;

    // SamplePeriod = 1/frequencia de amostragem (em microsegundos)
    SamplePeriod = (unsigned long)(1000000.0/*us*/*(1.0/SampleRateHz));
    //printk("\nPeriodo de amostragem = %lu us", SamplePeriod);

    //Form the message
    Msg[idx++]=CommsMode;
    Msg[idx++]=SampleMode;
    Msg[idx++]=(unsigned char)(OversampleRatio>>8);
    Msg[idx++]=(unsigned char)(OversampleRatio & 0xFF);

    //Encode the sample period as unsigned long big endian.
    Msg[4] = (unsigned char)((SamplePeriod >> 24) & 0xFF);
    Msg[5] = (unsigned char)((SamplePeriod >> 16) & 0xFF);
    Msg[6] = (unsigned char)((SamplePeriod >> 8) & 0xFF);
    Msg[7] = (unsigned char)(SamplePeriod & 0xFF);

    //Send the settings message.
    rt_transmit_serial_message(IMU_PORT,SET_SETTINGS_IMU_MSG,Msg,8);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Envia mensagem requisitando a configuracao da IMU*/
void rt_request_config_imu_serial_message(void)
{
     unsigned char Msg[8];  //Buffer for the message body.
    
    //Form the message
    Msg[0]=0;

    //Send the req config message.    
    rt_transmit_serial_message(IMU_PORT,REQ_CONFIG_IMU_MSG,Msg,1);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Implementa um filtro digital Butterwoth de 4 ordem com frequencia de corte de 10 Hz em 30 de amostragem.*/
float filter_data(float x, float m[]) 
{    
    float y;    // Saida do filtro
    
    /* Calcula a saida do filtro de acordo com a expressao canonica:
    
    m(k) = x(k) - b1m(k-1) - b2m(k-2) - b3m(k-3) - b4m(k-4);
    y(k)= a0m(k) + a1m(k-1) +a2m(k-2) +a3m(k-3) +a4m(k-4);
    
    Filtrando os dados:    */
    m[K] = x - B1*m[K_1] - B2*m[K_2] - B3*m[K_3] - B4*m[K_4];
    
    y = A0*m[K] + A1*m[K_1] + A2*m[K_2] + A3*m[K_3] + A4*m[K_4];
    
    // Incrementando o tempo k para a proxima interacao (esta ordem eh importante)
    m[K_4] = m[K_3];    m[K_3] = m[K_2];
    m[K_2] = m[K_1];    m[K_1] = m[K];
    
    return y;    // Saida do filtro
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Efetua a filtragem dos dados.*/
int rt_filter_imu_data(msg_imu_t* msg) 
{    
    // Variaveis estaticas que armazenam as 4 ultimas amostras para o calculo do filtro digital.
    // m(k) diz respeito ao aramzenamento dos dados para calculo do filtro.
    static float m_gx[FILTER_ORDER+1],m_gy[FILTER_ORDER+1],m_gz[FILTER_ORDER+1]; 
    static float m_ax[FILTER_ORDER+1],m_ay[FILTER_ORDER+1],m_az[FILTER_ORDER+1];
    
    msg->gyro[X_AXIS]  = filter_data(msg->gyro[X_AXIS], m_gx);
    msg->gyro[Y_AXIS]  = filter_data(msg->gyro[Y_AXIS], m_gy);
    msg->gyro[Z_AXIS]  = filter_data(msg->gyro[Z_AXIS], m_gz);
    msg->accel[X_AXIS] = filter_data(msg->accel[X_AXIS], m_ax);
    msg->accel[Y_AXIS] = filter_data(msg->accel[Y_AXIS], m_ay);
    msg->accel[Z_AXIS] = filter_data(msg->accel[Z_AXIS], m_az);

    return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Abre a imu para comunicacao e configura a frequencia de comunicacao desejada*/
int rt_open_imu(void) 
{
    unsigned char msgbuf[MAX_MESSAGE_LEN];
    int count_recv = 0; // Conta as tentativas de receber pacotes validos
    int count_ok   = 0; // Conta as tentativas bem sucedidas
    int count_try  = 0; // Conta todas as tentativas
    int type_message;
    unsigned long SamplePeriod;
    
    // Abre a porta serial da IMU para comunicacao ah 
    if (rt_open_serial(IMU_PORT,115200) < 0) {
        //rt_printk("Nao abriu a serial da IMU\n");
                 return -1; // Erro
        }
    
    // Tenta setar a IMU 10 vezes, do contrario desisti
    for (count_try=1; count_try<=10; count_try++) {
    
        // Seta os parametros da imu para comunicacao ah 50 Hz
        rt_send_imu_settings_serial_message(    IMU_SAMPLERATE_HZ,
                            IMU_OVERSAMPLE, 
                            RS232_COMMS_MODE, 
                            CONVERTED_SAMPLE_MODE);
        
        // Requisita a nova configuracao da IMU
        rt_request_config_imu_serial_message();
        
        count_recv = 0; // Reseta o contador para a proxima interacao.
        
        // Enquanto a imu nao responder dentro de um certo tempo
        while (count_recv++ < 50) {
            if (rt_process_imu_serial(msgbuf,&type_message) == 1)
            // Recebeu um pacote valido
                if (type_message == SETTINGS_IMU_MSG)
                // Resposta desejada ao pedido de configuracao
                    break;
            rt_busy_sleep(10000000); // Dorme 10 ms (Isso eh muito necessario)
        }
        
        // Caso tenha ocorrido time_out de recebimento de pacotes
        if (count_recv >= 50)
            continue; // Retorna ao comeco do while
            
        else 
        {    // Testa a configuracao de comunicacao serial
            if (msgbuf[4] != RS232_COMMS_MODE)
                continue;
            // Testa a configuracao de conversao de dados
            if (msgbuf[5] != CONVERTED_SAMPLE_MODE)
                continue;
    
            // Testa a configuracao de Oversampleratio
            if (msgbuf[6] != ((unsigned char)(IMU_OVERSAMPLE>>8)))
                continue;
            if (msgbuf[7] != ((unsigned char)(IMU_OVERSAMPLE & 0xFF))) 
                continue;
                
            // Testa a configuracao de SampleRate
            // Converte a frequencia de amostragem em periodo de amostragem
            SamplePeriod = (unsigned long)(1000000.0*(1.0/IMU_SAMPLERATE_HZ));
            
            if (msgbuf[8] != ((unsigned char)((SamplePeriod >> 24) & 0xFF)))
                continue;
            if (msgbuf[9] != ((unsigned char)((SamplePeriod >> 16) & 0xFF)))
                continue;
            if (msgbuf[10] != ((unsigned char)((SamplePeriod >> 8) & 0xFF)))
                continue;
            if (msgbuf[11] != ((unsigned char)(SamplePeriod & 0xFF))) 
                continue;
            
            // Se o loop chegou ate aqui, entao a configuracao foi bem sucedida
            count_ok++; // Conta as tentativas bem sucedidas
        } // end if
        
        if (count_ok >= 2) 
            break; // Sucesso
    } // end for
    
    // Contabiliza o resultado. Quatro tentativas bem sucedidas garantem o sucesso
    if (count_ok >= 2) 
        return 0; // Sucesso
    else {
        rt_close_serial(IMU_PORT); // Fecha a serial
        return -1; // Fracasso    
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
// Obtencao dos dados preenchidos e retorna se o dado e novo
int rt_get_imu_data(msg_imu_t *msg)
{
    // Preenche a variavel recebida com o valor da variavel global
    msg->gyro[X_AXIS] = global_msg_imu.gyro[X_AXIS];
    msg->gyro[Y_AXIS] = global_msg_imu.gyro[Y_AXIS];
    msg->gyro[Z_AXIS] = global_msg_imu.gyro[Z_AXIS];
    
    msg->accel[X_AXIS] = global_msg_imu.accel[X_AXIS];
    msg->accel[Y_AXIS] = global_msg_imu.accel[Y_AXIS];
    msg->accel[Z_AXIS] = global_msg_imu.accel[Z_AXIS];
    
    msg->time_stamp = global_msg_imu.time_stamp;
    
    if (global_new_data == 1) // Entao o dado eh novo
    {
        // Resetar a variavel de dado novo
        global_new_data = 0;
        return 1; // Dado novo
    }

    return 0; // Dado nao eh novo
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*    Funcao executada pela tarefa de tempo real da imu*/
void func_imu(int t)
{
    unsigned char msgbuf[MAX_MESSAGE_LEN];
    int type_message;     // Armanzena o tipo de mensagem recebida
    
    while (1) { // Enquanto nao for determinado o fim do modulo        
        // Tenta processar pacotes 
        global_msg_imu.validade = rt_process_imu_serial(msgbuf,&type_message);
                    
        // Se o pacote e valido atualiza a variavel global 
        if (global_msg_imu.validade == 1)
        {
            // Se a mensagem processada nao for a de interesse, vai para a proxima interacao
            if (type_message != HS_SERIAL_IMU_MSG)
                continue;
        
            rt_convert_imu_data(&global_msg_imu, msgbuf);
            
            // Seta a variavel de dado novo
            global_new_data = 1;
        }
        
        //Espera completar o periodo de 10 milisegundos (100 Hz)
        rt_task_wait_period();
    }

    return (void)0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inicio do modulo da imu
int init_module(void)
{
    RTIME tick_period;    // Determmina o periodo da tarefa
    RTIME now;        // Tempo atual em ns
    
    // Abre e configura a IMU para comunicacao
    if (rt_open_imu() < 0) {
        rt_printk("Nao abriu o dispositivo IMU\n");
        return -1;
        }

    // Dispara a unica tarefa de tempo real principal
    if (rt_task_init(&task_imu, func_imu, 0, 5000, IMU_TASK_PRIORITY, 0, 0) < 0) {
        rt_printk("Falha ao criar a tarefa de tempo real da imu\n");
        return -1;
    }
    
    // Determina o periodo de execussao da tarefa como sendo multiplo de 1 ms (PERIOD* 1 ms)
    tick_period = IMU_PERIOD*start_rt_timer(nano2count(UM_MILI_SEGUNDO));
    now = rt_get_time();
    
    //Inicia a tarefa principal periodicamente
    if (rt_task_make_periodic(&task_imu, now + tick_period, tick_period) < 0) {
        rt_printk("Nao consegui lancar tarefa de tempo real periodicamente\n");
                return -1;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// Fim do modulo da imu
int cleanup_module(void)
{
    //Termina a tarefa de tempo real principal    
    rt_task_delete(&task_imu);
    
    rt_close_serial(IMU_PORT);    
    
    return 0;
}
