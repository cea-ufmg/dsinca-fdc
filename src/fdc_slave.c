/*!*******************************************************************************************
**********************************************************************************************

Programa para aquisicao de dados  - fdc_slave 

    Este programa efetua a coleta de dados da placa de aquisicao de dados do modulo PC104,
do sistema de atitude e referência de direção (AHRS) e do modulo gps, tambem do PC104, e 
coloca estes dados em fifos de tempo real, de onde serao lidos por um outro processo que roda
fora do kernel do linux.
Alem disso, os dados tambem sao colocados em uma fila serial para a transmissao via modem para
uma estacao de solo.
    A frequencia de execussao da tarefa de tempo real e de 50 Hz, assim como a captura de dados
da placa DAQ de da IMU. O GPS roda a 1 Hz, e a transmissao via modem sera de 10 Hz.

*********************************************************************************************
********************************************************************************************/
#include "fdc_slave.h"
#include "modem.h"

MODULE_AUTHOR("Armando Alves Neto e Guilheme A. S. Pereira");
MODULE_DESCRIPTION("RTAI real time data acquisition");
MODULE_LICENSE("GPL");

// Estrutura de variaveis globais ao modulo de tempo real
struct {
    // variaveis globais do modulo
    RT_TASK task_slave;

    // Sinaliza o fim da tarefa    
    int volatile end_slave;
} global;

/*!*******************************************************************************************
*********************************************************************************************/
///                FUNCAO DA PLACA DAQ
/*!*******************************************************************************************
*********************************************************************************************/
/*    Esta funcao coleta os dados da placa DAQ, preenche a estrutura da mensagem a ser
enviada via modem e coloca os dados na fila de tempo real da placa daq:
    INT daq_enable determina se a coleta de dados da placa daq esta ativa. */
static void rt_func_daq(configure *config)
{
    msg_daq_t msg;// Tipo utilizados para a transmissao da mensagem
    

    if (config->daq_enable) { // Se estiver habilitada a coleta de dados

        // Captura os dados dos 16 canais e retorna a validade destes dados
        msg.validade = rt_process_daq_16(&msg);

        msg.time_sys = rt_get_time_ns(); // Pega o tempo de coleta dos dados
	
        rtf_put(RT_FIFO_DAQ, &msg, sizeof(msg)); //Poem na fila
	if (config->modem_enable) modem_send_daq_data(&msg);
    }
    return (void)0;
}

/*!*******************************************************************************************
*********************************************************************************************/
///                FUNCAO DO GPS
/*!*******************************************************************************************
*********************************************************************************************/
/*    Esta funcao coleta os dados do modulo GPS, preenche a estrutura da mensagem a ser enviada
via modem e coloca os dados na fila de tempo real do gps. */
static void rt_func_gps(configure* config)
{
    if (config->gps_enable){ // Caso a coleta de dados do gps esteja habilitada
        msg_gps_t msg;

        // Captura os dados do gps e retorna a validade destes dados
        //msg.validade = rt_process_GPS_data(&msg);
        rt_get_gps_data(&msg);

        msg.time_sys = rt_get_time_ns(); // Pega o tempo de coleta dos dados
	
        rtf_put(RT_FIFO_GPS, &msg, sizeof(msg)); //Poe na fila
	if (config->modem_enable) modem_send_gps_data(&msg);
    }
}

/*
 *                 FUNCAO DO AHRS
 *     Esta função coleta os dados do ahrs e os coloca na fila de tempo real do ahrs
 *
 */
static void rt_func_ahrs(configure* config){
    msg_ahrs_t msg; // tipo de mensagem do ahrs
    //Se a coleta de dados do ahrs estiver ativa
    if(config->ahrs_enable) {
        msg.validade = rt_get_ahrs_data(&msg); //Busca os dados do ahrs
        msg.time_sys = rt_get_time_ns(); //Pega o tempo de coleta dos dados
        rtf_put(RT_FIFO_AHRS, &msg, sizeof(msg)); // poe na fila
	if (config->modem_enable) modem_send_ahrs_data(&msg);
    }
    return (void)0;
}

/*
 *                 FUNCAO DO NAV
 *     Esta função coleta os dados do nav e os coloca na fila de tempo real do nav
 *
 */
static void rt_func_nav(configure* config){
    msg_nav_t msg; // tipo de mensagem do nav
    //Se a coleta de dados do ahrs estiver ativa
    if(config->nav_enable) {
        msg.validade = rt_get_nav_data(&msg); //Busca os dados do nav
        msg.time_sys = rt_get_time_ns(); //Pega o tempo de coleta dos dados
        rtf_put(RT_FIFO_NAV, &msg, sizeof(msg)); // poe na fila
	if (config->modem_enable) modem_send_nav_data(&msg);
    }
    
    return (void)0;
}

/*
 *                 FUNCAO DO PITOT
 *     Esta função coleta os dados do pitot e os coloca na fila de tempo real do pitot
 *
 */
static void rt_func_pitot(configure* config){
    msg_pitot_t msg; // tipo de mensagem do pitot
    //Se a coleta de dados do ahrs estiver ativa
    if(config->pitot_enable) {
        msg.validade = rt_get_pitot_data(&msg); //Busca os dados do nav
        msg.time_sys = rt_get_time_ns(); //Pega o tempo de coleta dos dados
        rtf_put(RT_FIFO_PITOT, &msg, sizeof(msg)); // poe na fila
	if (config->modem_enable) modem_send_pitot_data(&msg);
    }
    return (void)0;
}

/*!*******************************************************************************************
*********************************************************************************************/
///            FUNCAO DE TRANSMISSAO DO MODEM
/*!*******************************************************************************************
*********************************************************************************************/
/*    Esta funcao e chamada quando se deseja transmitir um conjunto de dados por meio do link
de radio. Neste ponto, a estrutura de dados do modem ja foi preenchida pelas outras funcoes
bastando agora transmiti-la. */
static void rt_func_modem(configure *config) {
}

/*!*******************************************************************************************
*********************************************************************************************/
///            FUNCAO DE RECEBIMENTO DO MODEM
/*!*******************************************************************************************
*********************************************************************************************/
/*static void rt_func_modem_recev()
{
    char ch;    // Recebe os comandos via modem

    // Le o primeiro byte de sincronismo
    if (rt_spread(MODEM_PORT, &ch, sizeof(char)) != 0)
        return (void)1; 
    if (ch != SYNC_SER_0)
        return (void)1; 
    
    // Le o segundo byte de sincronismo
    if (rt_spread(MODEM_PORT, &ch, sizeof(char)) != 0)
        return (void)1; 
    if (ch != SYNC_SER_1)
        return (void)1; 
    
    // Le o comando desejado
    if (rt_spread(MODEM_PORT, &ch, sizeof(char)) != 0) 
        return (void)1; 
    
    switch (ch) {    // Envia o comando para o uav_jedi
        case START_CMD:
            rtf_put(RT_FIFO_COMAND, &ch, sizeof(char)); //Poe na fila
        break;
        
        case STOP_CMD:
            rtf_put(RT_FIFO_COMAND, &ch, sizeof(char)); //Poe na fila
        break;
        
        case RESET_GPS_CMD:
            rtf_put(RT_FIFO_COMAND, &ch, sizeof(char)); //Poe na fila
        break;
        
        default:
        break;
    }
    return (void)0;
}
*/
/*!*******************************************************************************************
*********************************************************************************************/
///            THREAD DE TEMPO REAL DE CONTROLE
/*!*******************************************************************************************
*********************************************************************************************/
/*    Esta funcao trata os comandos de controle enviados pelo programa mestre (fdc_master) e
reporta a este a resposta ao comando por meio da fifo de status.*/
static int rt_func_control(configure * config)
{
    int n;
    cmd_status_t result;
    cmd_msg_t from_master; // Messagem do tipo parser_cmd_msg_t, porem sem o topico de caracters


    // Le a fifo de comunicacao entre 'fdc_master' e 'fdc_slave'.
    // Somente leh os bytes se os mesmos compuserem uma mensagem completa.
    n = rtf_get(RT_FIFO_CONTROL, &from_master, sizeof(from_master)); //Le da fila de controle

    if (n == sizeof(from_master)) { // Trata a mensagem recebida
        switch (from_master.cmd) {
            case START:
                // Habilita todas as funcoes do modulo
                config->daq_enable   = 1;
                config->gps_enable   = 1;
                config->ahrs_enable  = 1;
                config->nav_enable   = 1;
                config->pitot_enable = 1;
                config->modem_enable = 1;
		
                result = OK;
            break;
    
            case STOP:
                // Desabilita todas as funcoes do modulo
                config->daq_enable   = 0;
                config->gps_enable   = 0;
                config->ahrs_enable  = 0;
                config->nav_enable   = 0;
                config->pitot_enable = 0;
                config->modem_enable = 0;
                
                result = OK;
            break;
    
            case QUIT:
                // Finaliza o sistema de aquisicao e controle do UAV.
                global.end_slave = 1; // Seta o fim da tarefa de tempo real
                
                result = OK;
            break;
            
            case NODATA:
                // Cancela a coleta de dados de algum dispositivo
                if (from_master.option == DAQ)
                    config->daq_enable = 0;
                if (from_master.option == GPS)
                    config->gps_enable = 0;
                if (from_master.option == AHRS)
                    config->ahrs_enable = 0;
                if (from_master.option == NAV)
                    config->nav_enable = 0;
                if (from_master.option == PITOT)
                    config->pitot_enable = 0;
                result = OK;
            break;
    
            case IS_ALIVE:
                // Apenas verifica se o modulo de tempo real esta vivo
                result = OK; // Estou vivo
            break;
            
            case RESET_GPS:
                // Reseta o modulo gps
                rt_request_gps_reset();
                result = OK;
            break;
            
            default: 
                result = NOT_OK;
            break;
        } // end switch
        
        //Poe na fila de status o resultado do comando
        if (rtf_put(RT_FIFO_STATUS, &result, sizeof(result)) == sizeof(result))
            return 0; // Sucesso
        else {
            result = NOT_OK; 
            rtf_put(RT_FIFO_STATUS, &result, sizeof(result)); 
            return 1; // Fracasso
        }
    } // end if
    
    return 1; // Fracasso
}
/*!*******************************************************************************************
*********************************************************************************************/
///                THREAD DE TEMPO REAL PRINCIPAL
/*!*******************************************************************************************
*********************************************************************************************/
/*    Funcao da tarefa de tempo real, que simula um comportamento multi-tarefa dos dispositivos
a serem manipulados, da transmissao via modem e das comunicacoes entre os processos por meio
das fifos de controle e de status.*/
static void func_fdc_slave(int t)
{
    // Contadores que definem o periodo de cada funcao
    int count_gps = 0;

    configure config; // Configuracao de execucao do modulo fdc_slave

    // Desabilita todos os dispositivos
    config.daq_enable   = 0;
    config.gps_enable   = 0;
    config.ahrs_enable  = 0;
    config.nav_enable   = 0;
    config.pitot_enable = 0;
    config.modem_enable = 0;
    
    while (!global.end_slave) { // Enquanto nao for determinado o fim do modulo

        // Incrementa os contadores do escalonador
        count_gps++;
        //count_modem++;
        //count_modem_recev++;

        // Recebe comandos do fdc_master com uma frequencia maxima de 50 Hz
        rt_func_control(&config);

        // Acessa a placa daq com uma frequencia maxima de 50 Hz
        rt_func_daq(&config);

        //Acessa o ahrs com uma frequência máxima de 50 Hz
        rt_func_ahrs(&config);
        
        //Acessa o nav com uma frequência máxima de 50 Hz
        rt_func_nav(&config);

        //Acessa o pitot com uma frequência máxima de 50 Hz
        rt_func_pitot(&config);

        // Acessa o gps com uma frequencia maxima de 1 Hz    
        if (count_gps >= _05_HZ){
            rt_func_gps(&config);
            count_gps = 0;
        }

	//Escreve os dados do modem
	rt_func_modem(&config);
        
        
        //if (count_modem_recev >= _01_HZ){
        //    rt_func_modem_recev();
        //    count_modem_recev = 0;
        //}

        //Espera completar o periodo de 20 milisegundos (50 Hz)
        rt_task_wait_period();
    }
    // A tarefa de tempo real se suspende esperando pelo fim
    rt_task_suspend(&global.task_slave);

    return (void)0;
}
/*!*******************************************************************************************
*********************************************************************************************/
///            FUNCAO DE TERMINO DO MODULO
/*!*******************************************************************************************
*********************************************************************************************/
/*    Funcao que determina o fim do modulo de tempo real */
int terminate_module(void)
{
    global.end_slave = 1;    // Seta o fim da tarefa de tempo real

    stop_rt_timer();         //Para o tempo

    //Termina a tarefa de tempo real principal
    rt_task_delete(&global.task_slave);    

    rtf_destroy(RT_FIFO_AHRS);
    rtf_destroy(RT_FIFO_DAQ);
    rtf_destroy(RT_FIFO_GPS);
    rtf_destroy(RT_FIFO_NAV);
    rtf_destroy(RT_FIFO_PITOT);
    rtf_destroy(RT_FIFO_CONTROL);
    rtf_destroy(RT_FIFO_STATUS);
    //rtf_destroy(RT_FIFO_COMAND);
    
    return 0;
}

/*!*******************************************************************************************
*********************************************************************************************/
///                INICIALIZA O MODULO
/*!*******************************************************************************************
*********************************************************************************************/
int init_module(void)
{
    RTIME tick_period;    // Determmina o periodo da tarefa
    RTIME now;        // Tempo atual em ns
    int terminate = 0;    // Seta o fim do modulo caso algo falhe

    global.end_slave = 0; // Varaivel global que sinaliza o fim da tarefa de tempo real
    
    //Cria a fila de mensagens

    if (rtf_create_using_bh(RT_FIFO_AHRS,   20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_AHRS\n");
        terminate = 1;
    }
    if (rtf_create_using_bh(RT_FIFO_DAQ,     20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_DAQ\n");
        terminate = 1;
    }
    if (rtf_create_using_bh(RT_FIFO_GPS,     20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_GPS\n");
        terminate = 1;
    }
    if (rtf_create_using_bh(RT_FIFO_NAV,   20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_NAV\n");
        terminate = 1;
    }
    if (rtf_create_using_bh(RT_FIFO_PITOT,   20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_PITOT\n");
        terminate = 1;
    }
    if (rtf_create_using_bh(RT_FIFO_CONTROL,20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_CONTROL\n");
        terminate = 1;
    }
    if (rtf_create_using_bh(RT_FIFO_STATUS, 20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_STATUS\n");
        terminate = 1;
    }
    /*if (rtf_create_using_bh(RT_FIFO_COMAND, 20000, 0) < 0) {
        rt_printk("Falha ao abrir fifo: FIFO_STATUS\n");
        terminate = 1;
    }*/
    
    /////////////////////////////////////////////////////////////////////////////////
    // Dispara a unica tarefa de tempo real principal
    if (rt_task_init(&global.task_slave, func_fdc_slave, 0, 5000, TASK_PRIORITY, 0, 0) < 0) {
        rt_printk("Falha ao criar a tarefa de tempo real\n");
        terminate = 1;
    }
    /////////////////////////////////////////////////////////////////////////////////
    // Abre e configura o Controlador de Servos 
    /*if (rt_open_modem() < 0) {
        rt_printk("Nao abriu o dispositivo Modem\n");
                 terminate = 1;
        }*/
    /////////////////////////////////////////////////////////////////////////////////
    // Determina o periodo de execucao da tarefa como sendo multiplo de 1 ms (PERIOD* 1 ms)
    tick_period = PERIOD*start_rt_timer(nano2count(UM_MILI_SEGUNDO));
    now = rt_get_time();
    
    //Inicia a tarefa principal periodicamente
    if (rt_task_make_periodic(&global.task_slave, now + tick_period, tick_period) < 0) {
        rt_printk("Nao consegui lancar tarefa de tempo real periodicamente\n");
                terminate = 1;
    }
    
    if (terminate == 1)
        terminate_module();
    else    
        rt_printk("\nMODULO FDC_SLAVE\n");

    rt_busy_sleep(1000000000);

    return 0;
} 
/*!*******************************************************************************************
*********************************************************************************************/
///                FINALIZA MODULO
/*!*******************************************************************************************
*********************************************************************************************/
int cleanup_module(void)
{
    terminate_module();
    rt_printk("FIM DO MODULO FDC_SLAVE\n");
    
    return 0;
}
