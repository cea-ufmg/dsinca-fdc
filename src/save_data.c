#include "save_data.h"

// Variável global que contem os dados retirados da FIFO ahrs
 msg_ahrs_t msg_ahrs;
// Variável global que contem os dados retirados da FIFO daq
 msg_daq_t msg_daq;
// Variável global que contem os dados retirados da FIFO gps
 msg_gps_t msg_gps;
// Variável global que contem os dados retirados da FIFO nav
 msg_nav_t msg_nav;
 // Variável global que contem os dados retirados da FIFO pitot
 msg_pitot_t msg_pitot;

               

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para escrita dos cabecalhos dos arquivos
int write_headers (FILE* arq_daq, FILE* arq_ahrs, FILE* arq_gps, FILE* arq_nav, FILE* arq_pitot) {

    /// Escreve os cabecalhos dos arquivos
    fprintf(arq_daq,
    "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
    "%% Arquivo da Placa de Aquisicao de Dados (DAQ) - %s"
    "\n%% Valores das tensoes dos canais, Tempo do sistema e Validade dos dados"

    "\n%% <can00>\t<can01>\t<can02>\t<can03>\t<can04>\t<can05>\t<can06>\t<can07>\t"
           "<can08>\t<can09>\t<can10>\t<can11>\t<can12>\t<can13>\t<can14>\t<can15>\t"
           "<time_stamp>\t<validade>\n"

    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n" ,global.file_daq_name);

    fflush(arq_daq);
    
    fprintf(arq_ahrs,
    "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
    "%% Arquivo do Sistema de Atitude e Referência de Direção (AHRS) - %s"
    "\n%% Valores dos ângulos fornecidos pelo filtro de Kalman do AHRS (º), Velocidades Angulares(º/s),"
          " Acelerações nos três eixos (g), Campo Magnético medido nos três eixos (Gauss), Temperatura interna do AHRS,"
    " Tempo do AHRS (us), Tempo do Sistema (nanosegundos) e Validade dos dados"

    "\n%% <phi>\t<theta>\t<psi>\t<p>\t<q>\t<r>\t<x''>\t<y''>\t<z''>\t<x_mag>\t<y_mag>\t"
         "<z_mag>\t<Temp>\t<time_stamp>\t<time_sys>\t<validade>\n"
     
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n", global.file_ahrs_name);

    fflush(arq_ahrs);
    
    fprintf(arq_gps,
    "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
    "%% Arquivo do Sistema de Posicionamento GLOBAL (GPS) - %s"
    "\n%% Valores de latitude,longitude,altitude,hdop,geoid_separation, north_south,"
    "east_west, validade, n_satellites, units_altitude, units_geoid_separation, "
    "GPS_time(tempo em segundos), vel_leste (m/s), vel_norte (m/s), vel_cima (m/s),"
    "erro_horizontal (m), erro_vertical (m), erro_estimado (m), status, Ground Speed (kts),"
    "course (deg),data, declinaçao magnética (deg), direcao da declinacao, modo de operacao"
    "Tempo do sistema(em nanosegundos), validade "
    
    "\n%% <latitude>\t<longitude>\t<altitude>\t<hdop>\t<geoid_separation>\t"
    "<north_south>\t<east_west>\t<n_satellites>\t<units_altitude>\t<units_geoid_separation>\t"
    "<GPS_time>\t<east_v>\t<north_v>\t<up_v>\t<hpe>\t<vpe>\t<epe>\t<gspeed>\t<course>\t"
    "<date>\t<magvar>\t<magvardir>\t<mode>\t<time_stamp>\t<validade>\n"
    
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n",  global.file_gps_name);

    fflush(arq_gps);

    fprintf(arq_nav,
    "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
    "%% Arquivo do Sistema de Navegação Inercial (NAV) - %s"
    "\n%% Valores dos ângulos fornecidos pelo filtro de Kalman do AHRS (º), Velocidades Angulares(º/s),"
    " Acelerações nos três eixos (g), Velocidade norte(m/s), leste(m/s), baixo(m/s), latitude(º), longitude(º), altitude(m),"
    "Temperatura interna do NAV(ºC), byte de erro, byte de status, Tempo do NAV (ms), Tempo do Sistema e Validade dos dados"

    "\n%% <phi>\t<theta>\t<psi>\t<p>\t<q>\t<r>\t<x''>\t<y''>\t<z''>\t<nVel>\t<eVel>\t"
         "<dVel>\t<Long>\t<Lat>\t<Alt>\t<Temp>\t<erro>\t<status>\t<time_stamp>\t<time_sys>\t<validade>\n"
     
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n", global.file_nav_name);

    fflush(arq_nav);

    fprintf(arq_pitot,
    "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
    "%% Arquivo de aquisição do tubo de pitot - %s"
    "\n%% Pressão estática (Pa), Temperatura (C), Pressão dinâmica(int), Ângulo de ataque (int),"
    "Ângulo de deslizamento (int) , Tempo do Sistema e Validade dos dados"

    "\n%% <static>\t<temperature>\t<dynamic>\t<attack>\t<sideslip>\t<time_sys>\t<validade>\n"
     
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n", global.file_pitot_name);

    fflush(arq_pitot);

//    master_log(ERROR_LOG, "Vivo!");
    
    return 1;
}

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para a leitura dos dados da fifo da placa daq  
int get_daq()
{

    if (read(global.fifo_daq, &msg_daq, sizeof(msg_daq)) == sizeof(msg_daq))  // Leitura efetuada com sucesso
        return 1;
    else
        return 0;
}    

/*!*******************************************************************************************
 * *********************************************************************************************/
// Funcao para armazenagem dos dados no arquivo da placa daq. A função get_daq deve ser chamada antes
int save_daq(FILE* arquivo_daq)
{
    int i;

    fprintf(arquivo_daq,"\n");
        
    for(i=0;i<16;i++)
    // Imprime em arquivo o dado convertido em tensao de 0 a 5 volts
        fprintf(arquivo_daq,"%f\t",(float)(msg_daq.tensao[i]));

    // Imprime a validade do dado e o tempo de amostragem do sistema
    fprintf(arquivo_daq,"%lld\t%d", msg_daq.time_sys,msg_daq.validade);

    fflush(arquivo_daq);
        
    return 1; // Sucesso de leitura da fifo e escrita no arquivo
}
/*!*****************************************************************************************
 * ******************************************************************************************/
// Função para a leitura dos dados da fifo do AHRS
int get_ahrs()
{
    if (read(global.fifo_ahrs, &msg_ahrs, sizeof(msg_ahrs)) == sizeof(msg_ahrs))
        return 1; //Leitura efetuada com sucesso
    else
        return 0;
}

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para e armazenagem dos dados calculado no arquivo do ahrs
int save_ahrs(FILE* arquivo_ahrs)
{
    //Imprime os ângulos estimados
    fprintf(arquivo_ahrs,"\n%f\t%f\t%f\t", msg_ahrs.angle[0],msg_ahrs.angle[1],msg_ahrs.angle[2]);
    //Imprime as velocidades angulares
    fprintf(arquivo_ahrs,"%f\t%f\t%f\t", msg_ahrs.gyro[0],msg_ahrs.gyro[1],msg_ahrs.gyro[2]);
    //Imprime as acelerações
    fprintf(arquivo_ahrs,"%f\t%f\t%f\t", msg_ahrs.accel[0],msg_ahrs.accel[1],msg_ahrs.accel[2]);
    //Imprime os campos magnéticos
    fprintf(arquivo_ahrs,"%f\t%f\t%f\t", msg_ahrs.magnet[0],msg_ahrs.magnet[1],msg_ahrs.magnet[2]);
    //Imprime a temperatura interna, o tempo do AHRS, o tempo do sistema  e a validade dos dados
        fprintf(arquivo_ahrs,"%f\t%f\t%lld\t%d", msg_ahrs.temp,msg_ahrs.time_stamp,msg_ahrs.time_sys,msg_ahrs.validade);
     //Força a escrita no arquivo
        fflush(arquivo_ahrs);
        
    return 1;
}

/*!*******************************************************************************************
 * *********************************************************************************************/
// Funcao para a leitura dos dados da fifo do gps
 int get_gps()
 {
         if (read(global.fifo_gps, &msg_gps, sizeof(msg_gps)) == sizeof(msg_gps)) // Leitura efetuada com sucesso
             return 1; // Sucesso de leitura da fifo e escrita no arquivo
     else
            return 0; // Falha de leitura da fifo e escrita no arquivo         
 }

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para a leitura dos dados da fifo do gps e armazenagem destes dados no 
// arquivo do gps
int save_gps(FILE* arquivo_gps)
{
    //msg_gps_t msg;
    
    //if (read(global.fifo_gps, &msg, sizeof(msg)) == sizeof(msg)) { // Leitura efetuada com sucesso

        fprintf(arquivo_gps,"\n%f\t%f\t%f\t%f\t%f\t", msg_gps.latitude, msg_gps.longitude, msg_gps.altitude, msg_gps.hdop, msg_gps.geoid_separation);
        
        fprintf(arquivo_gps,"%d\t%d\t%d\t%d\t%d\t",msg_gps.north_south, msg_gps.east_west, msg_gps.n_satellites, msg_gps.units_altitude, msg_gps.units_geoid_separation);
        
        fprintf(arquivo_gps,"%f\t%f\t%f\t%f\t",msg_gps.GPS_time_gga,msg_gps.east_v,msg_gps.north_v,msg_gps.up_v);

        fprintf(arquivo_gps,"%f\t%f\t%f\t",msg_gps.hpe,msg_gps.vpe,msg_gps.epe);
        
        fprintf(arquivo_gps,"%f\t%f\t%d\t",msg_gps.gspeed,msg_gps.course,msg_gps.date);
        
        fprintf(arquivo_gps,"%f\t%d\t%d\t",msg_gps.magvar,msg_gps.magvardir,msg_gps.mode);
        
        fprintf(arquivo_gps,"%lld\t%d", msg_gps.time_sys, msg_gps.validity);
        
        // Força a escrita dos dados do gps em disco
        fflush(arquivo_gps);

        return 1; // Sucesso de leitura da fifo e escrita no arquivo
    //}
    //else
    //    return 0; // Falha de leitura da fifo e escrita no arquivo
}

/*!*****************************************************************************************
 * ******************************************************************************************/
// Função para a leitura dos dados da fifo do NAV
int get_nav()
{
    if (read(global.fifo_nav, &msg_nav, sizeof(msg_nav)) == sizeof(msg_nav))
        return 1; //Leitura efetuada com sucesso
    else
        return 0;
}

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para e armazenagem dos dados calculado no arquivo do nav
int save_nav(FILE* arquivo_nav)
{
    //Imprime os ângulos estimados
    fprintf(arquivo_nav,"\n%f\t%f\t%f\t", msg_nav.angle[0],msg_nav.angle[1],msg_nav.angle[2]);
    //Imprime as velocidades angulares
    fprintf(arquivo_nav,"%f\t%f\t%f\t", msg_nav.gyro[0],msg_nav.gyro[1],msg_nav.gyro[2]);
    //Imprime as acelerações
    fprintf(arquivo_nav,"%f\t%f\t%f\t", msg_nav.accel[0],msg_nav.accel[1],msg_nav.accel[2]);
    //Imprime as velocidades
    fprintf(arquivo_nav,"%f\t%f\t%f\t", msg_nav.nVel,msg_nav.eVel,msg_nav.dVel);
    //Imprime as posições
    fprintf(arquivo_nav,"%f\t%f\t%f\t", msg_nav.latitude,msg_nav.longitude,msg_nav.altitude);
    //Imprime a temperatura interna, byte de erro, byte de status
    fprintf(arquivo_nav,"%f\t%d\t%d\t", msg_nav.temp,msg_nav.internal_error, msg_nav.internal_status);
    //Imprime o tempo do NAV, o tempo do sistema  e a validade dos dados
        fprintf(arquivo_nav,"%ld\t%lld\t%d", msg_nav.time_stamp,msg_nav.time_sys,msg_nav.validade);
     //Força a escrita no arquivo
    fflush(arquivo_nav);
        
    return 1;
}

/*!*****************************************************************************************
 * ******************************************************************************************/
// Função para a leitura dos dados da fifo do PITOT
int get_pitot()
{
    if (read(global.fifo_pitot, &msg_pitot, sizeof(msg_pitot)) == sizeof(msg_pitot))
        return 1; //Leitura efetuada com sucesso
    else
        return 0;
}

/*!*******************************************************************************************
*********************************************************************************************/
// Funcao para e armazenagem dos dados calculado no arquivo do pitot
int save_pitot(FILE* arquivo_pitot)
{
    //Imprime os valores (raw)
    fprintf(arquivo_pitot,"\n%f\t%f\t%f\t", msg_pitot.static_pressure,msg_pitot.temperature,msg_pitot.dynamic_pressure);
    //Imprime os valores (raw)
    fprintf(arquivo_pitot,"%f\t%f\t", msg_pitot.attack_angle,msg_pitot.sideslip_angle);
    //Imprime o tempo do sistema  e a validade dos dados
    fprintf(arquivo_pitot,"%lld\t%d", msg_pitot.time_sys,msg_pitot.validade);
     //Força a escrita no arquivo
    fflush(arquivo_pitot);
        
    return 1;
}

/*!*******************************************************************************************
*********************************************************************************************/
int create_new_dir (void)
{
    time_t time_new_dir;     // Armazena o tempo do sistema
    char dir[MAX_STRLEN];    // Armazena o nome do novo diretorio
    char file_daq[MAX_STRLEN], file_ahrs[MAX_STRLEN], file_gps[MAX_STRLEN], file_nav[MAX_STRLEN], file_pitot[MAX_STRLEN];
    char *a;
        
    a = (char*) malloc(24*sizeof(char));
    
    // Pega o tempo de execussao do sistema e o separa em ano, mes, dia, hora, minuto e segundo
    time_new_dir = time(NULL);
    a = ctime(&time_new_dir);
    
    // Configura os nomes dos arquivos de armazenamento de dados
    strncpy(global.file_daq_name,ARQ_DAQ,MAX_STRLEN-1);
    global.file_daq_name[MAX_STRLEN-1] = '\0';
    
    strncpy(global.file_ahrs_name,ARQ_AHRS,MAX_STRLEN-1);
    global.file_ahrs_name[MAX_STRLEN-1] = '\0';
    
    strncpy(global.file_gps_name,ARQ_GPS,MAX_STRLEN-1);
    global.file_gps_name[MAX_STRLEN-1] = '\0';

    strncpy(global.file_nav_name,ARQ_NAV,MAX_STRLEN-1);
    global.file_nav_name[MAX_STRLEN-1] = '\0';

    strncpy(global.file_pitot_name,ARQ_PITOT,MAX_STRLEN-1);
    global.file_pitot_name[MAX_STRLEN-1] = '\0';
    
    // Converte os valores de tempo para strings
    sprintf(dir,"%sVoo_%c%c%c_%c%c%c_%c%c_%c%c%c%c%c%c%c%c_%c%c%c%c/",FILES_PATH,
    a[0], a[1],a[2],    // Dia da semana XXX
    //a[3],            // espaco
    a[4],a[5],a[6],        // Mes YYY
    //a[7],            // espaco
    a[8],a[9],        // Dia do mes
    //a[10],
    a[11],a[12],a[13],a[14],a[15],a[16],a[17],a[18], // Hora, minuto e segundo.
    //a[19],        // espaco
    a[20],a[21],a[22],a[23]); // ano
    
    // Cria o novo diretorio
    if (mkdir( dir, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH|S_IXOTH|S_IXUSR|S_IXGRP) == -1){
        master_log(ERROR_LOG, "Create_new_dir (thread: Falha ao criar o novo diretorio de arquivos.");
        perror("Falha ao criar o novo diretorio de arquivos.");
        exit(EXIT_FAILURE);
    }
    
    // Muda o nome do arquivo da placa daq, da imu ou do gps
    strncpy(file_daq, dir, MAX_STRLEN-1);
    strncpy(file_ahrs, dir, MAX_STRLEN-1);
    strncpy(file_gps, dir, MAX_STRLEN-1);
    strncpy(file_nav, dir, MAX_STRLEN-1);
    strncpy(file_pitot, dir, MAX_STRLEN-1);
    
    // Monta o resto dos nomes dos arquivos
    strcat(file_daq, global.file_daq_name);
    strcat(file_ahrs, global.file_ahrs_name);
    strcat(file_gps, global.file_gps_name);
    strcat(file_nav, global.file_nav_name);
    strcat(file_pitot, global.file_pitot_name);
    
    // Muda o nome do arquivo da placa daq, da imu ou do gps
    strncpy(global.file_daq_name,file_daq,MAX_STRLEN-1);
    global.file_daq_name[MAX_STRLEN-1] = '\0';    
    
    strncpy(global.file_ahrs_name,file_ahrs,MAX_STRLEN-1);
    global.file_ahrs_name[MAX_STRLEN-1] = '\0';

    strncpy(global.file_gps_name,file_gps,MAX_STRLEN-1);
    global.file_gps_name[MAX_STRLEN-1] = '\0';

    strncpy(global.file_nav_name,file_nav,MAX_STRLEN-1);
    global.file_nav_name[MAX_STRLEN-1] = '\0';

    strncpy(global.file_pitot_name,file_pitot,MAX_STRLEN-1);
    global.file_pitot_name[MAX_STRLEN-1] = '\0';

    return 0;    
}

/*!*******************************************************************************************
*********************************************************************************************/
/*    Thread para a coleta dos dados. Primeiramente eh resetada a variavel de fim da thread,
 e na sequencia sao abertos os arquivos default ou novos.Entao se escreve os cabecalhos dos
 arquivos e so entao passasse a captar os dados, ate que a thread seja terminada.*/
void *save_data(void *arg)
{
    // Arquivos de escrita de dados
    FILE *arquivo_daq = NULL, *arquivo_ahrs = NULL, *arquivo_gps = NULL, *arquivo_nav = NULL, *arquivo_pitot = NULL;
    int daq_ok=0, gps_ok=0, ahrs_ok=0, nav_ok = 0, pitot_ok = 0;
      int local_end_save_data; 
    
    
    // Escreve na variavel de fim da thread
    sem_wait(&global.end_thread_save_data);
    global.end_save_data = SAVE_SEND;
    sem_post(&global.end_thread_save_data);
    
    sem_wait(&global.file_names); // Espera para poder ler os nomes de arquivos
    
    /* Cria um novo diretorio para os arquivos dentro de "/tmp/data", cujo nome
    eh funcao do tempo.*/
    create_new_dir();
    
    /*     Abre os arquivos para a escrita de dados. Se algum arquivo jah existir,
    (por exemplo, quando se nomear um novo arquivo e os outros permanecerem com os 
    mesmos nomes), entao ele serah reaberto para a escrita a partir do ultimo dado 
    escrito. */
    if ((arquivo_daq = fopen(global.file_daq_name,"w")) < 0) {
        printf("\n Erro na abertura do arquivo");
        master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (DAQ).(exit)");
        exit(1);
    }
    if ((arquivo_ahrs = fopen(global.file_ahrs_name,"w")) < 0) {
        printf("\n Erro na abertura do arquivo");
        master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (AHRS).(exit)");
        exit(1);
    }
    if ((arquivo_gps = fopen(global.file_gps_name,"w")) < 0) {
        printf("\n Erro na abertura do arquivo");
        master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (GPS).(exit)");
        exit(1);
    } 
    if ((arquivo_nav = fopen(global.file_nav_name,"w")) < 0) {
        printf("\n Erro na abertura do arquivo");
        master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (NAV).(exit)");
        exit(1);
    }
    if ((arquivo_pitot = fopen(global.file_pitot_name,"w")) < 0) {
        printf("\n Erro na abertura do arquivo");
        master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (PITOT).(exit)");
        exit(1);
    }

    // Abre os arquivos e escreve os cabecalhos
    write_headers(arquivo_daq, arquivo_ahrs, arquivo_gps, arquivo_nav, arquivo_pitot);    
    sem_post(&global.file_names); // Libera o semaforo

    while(1) { // Salva os dados de todos os dispositivos (placa daq, ahrs e gps)
    

        // Acessa a variavel global do while
        sem_wait(&global.end_thread_save_data);
        
        if (global.end_save_data==STOPPED) { // Enquanto o programa nao termina de salvar os dados
            // Libera o semaphoro e sai do loop
            sem_post(&global.end_thread_save_data);
            break;
        }
        else { // Pega os dados das FIFOS
            ahrs_ok=get_ahrs();
            daq_ok=get_daq();
            gps_ok=get_gps();
            nav_ok=get_nav();
            pitot_ok=get_pitot();
        }
                
        local_end_save_data=global.end_save_data;    
        
        // Libera o semaphoro
        sem_post(&global.end_thread_save_data);
        
        if ((local_end_save_data==SAVE)||(local_end_save_data==SAVE_SEND)){
            if (daq_ok) save_daq(arquivo_daq);
            if (gps_ok) save_gps(arquivo_gps);
            if (ahrs_ok) save_ahrs(arquivo_ahrs);
            if (nav_ok) save_nav(arquivo_nav);
            if (pitot_ok) save_pitot(arquivo_pitot);
        }
        
    } // end while
    
    // Enquanto as fifos de dados nao estiverem vazias, salva os dados
    
    if ((global.end_save_data==SAVE)||(global.end_save_data==SAVE_SEND)){
        while(get_ahrs()) save_ahrs(arquivo_ahrs);
        while(get_daq()) save_daq(arquivo_daq);
        while(get_gps()) save_gps(arquivo_gps);
        while(get_nav()) save_nav(arquivo_nav);
        while(get_pitot()) save_pitot(arquivo_pitot);
    }
    
    // Fecha os arquivos de armazenamento dos dados e zera os apontadores
    fclose(arquivo_daq);
    fclose(arquivo_ahrs);
    fclose(arquivo_gps);
    fclose(arquivo_nav);
    fclose(arquivo_pitot);
    
    // Anula os apontadores dos arquivos
    arquivo_daq = NULL;
    arquivo_ahrs = NULL;
    arquivo_gps = NULL;
    arquivo_nav = NULL;
    arquivo_pitot = NULL;
    
    master_log(STATUS_LOG, "Save_data (thread): Fim da thread.");
    // Retorno da thread (Apaga o descritor)
    pthread_exit(NULL);
    
    return (void*)0;
}
