/********************************************************************************************
**********************************************************************************************
PROJETO SIDEVAAN

Programa de controle da tarefa de tempo real - FDC_MASTER

    Este programa lanca uma sub-tarefa para monitorar a fila de mensagem onde o usuario
efetua comandos para o sistema. Além disso, ele recebe os comandos traduzidos pelo processo
"fdc_cmd_parser", e toma as providencias cabiveis (enviando se preciso ordens para o 
programa de tempo real). 
    Tambem ha uma thread "save_data" responsavel por salvar os dados recebidos 
atraves das filas de dados, da placa daq, da imu e do gps. 
Isso garante que os dados serao sempre salvos, mesmo que o programa fique preso 
por algum motivo.
*********************************************************************************************
********************************************************************************************/
#include "fdc_master.h"

int main(int argc,char *argv[])
{ 
    
    initialize();

    // Interpreta os argumentos passados em linha de comando.
    parse_args(argc,argv);

    // Carrega o arquivo de configuracao.
    if (!load_config_file()) {
        fprintf(stderr,"Falha ao carregar arquivo de configuracao.\n");
        master_log(ERROR_LOG,"Main: Falha ao carregar arquivo de configuracao.(exit)");
        terminate(0);
        exit(EXIT_FAILURE);
    }

    // Lanca o processo responsavel pelo tratamento de comandos
    // enviados pelo usuario.
    fdc_cmd_parser(NULL);

    main_loop();

    terminate(0);
    
    master_log(STATUS_LOG,"Main: FIM DO FDC_MASTER.");
    
    return EXIT_SUCCESS;
}

/*!*******************************************************************************************
*********************************************************************************************/
/* Inicializa o programa fdc_master. */
void initialize(void)
{
    // Marca o inicio.
    global.theend = 0;
    
    
    // Zera a mascara de acesso aos arquivos
    umask(0);
    
    /* Criando o arquivo de log. A partir daqui todas as mensagens de erro e 
    stay=tus serao armazenadas neste arquivo.*/
    if ((global.log_file = fopen(LOG_FILE,"a+")) < 0) {
        printf("\n Erro na abertura do arquivo de log");
        exit(1);
    }
    
    // Inicializa o semaforo da funcao fdc_log
    if (sem_init(&global.sem_log_file, 0, SEM_SIZE) != 0) {
        fprintf(stderr,"Erro ao abrir semaforo da funcao fdc_log.\n");
        exit(1);    
    }

    
    // Carrega os modulos do kernel
    load_modules();

    
    // Aponta para o arquivo de configuracao padrao.
    strncpy(global.config_file,DEFAULT_CONFIG_FILE,MAX_STRLEN-1);
    global.config_file[MAX_STRLEN-1] = '\0';

    // Inicializa os descritores de leitura e escrita, respectivamente,
    // do pipe de comunicacao entre 'fdc_master' e 'fdc_cmd_parser'.
    global.mypipe[0] = 0;
    global.mypipe[1] = 0;

    // Assegura-se de que a funcao 'terminate' serah
    // chamada quando o processo receber um sinal para terminar.
    signal(SIGQUIT,terminate);
    signal(SIGTERM,terminate);
    signal(SIGINT,terminate);
    signal(SIGTSTP,terminate);

    // Tenta apagar a FIFO de controle, caso tenha ocorrido
    // algum problema anteriormente e a mesma não tenha sido apagada.
    unlink(CTRL_FIFO);

    // Cria a FIFO de controle. (permite que o proprio criador e outros leiam e escrevam)
    if (mkfifo(CTRL_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) == -1){
        master_log(ERROR_LOG, "Initialize: falha ao criar a FIFO de controle do FDC.(exit)");
        exit(EXIT_FAILURE);
    }
    
    // Abre as fifos de controle e status
    // A fifo de controle flui do programa de modo usuario para o modulo de tempo real
    // Ela eh nao bloqueante e write_only
    if ((global.fifo_control = open(FIFO_CONTROL, O_WRONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO de controle.(exit)");
        fprintf(stderr,"Error opening FIFO de controle\n");
        exit(1);
    }
    
    // Abre as fifos para leitura dos dados
    if ((global.fifo_ahrs = open(FIFO_AHRS, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO AHRS.(exit)");
        fprintf(stderr,"Error opening FIFO AHRS\n");
        exit(1);
    }
    
    if ((global.fifo_daq = open(FIFO_DAQ, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO DAQ.(exit)");
        fprintf(stderr,"Error opening FIFO DAQ\n");
        exit(1);
    }
    
    if ((global.fifo_gps = open(FIFO_GPS, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO GPS.(exit)");
        fprintf(stderr,"Error opening FIFO GPS\n");
        exit(1);
    }
    
    if ((global.fifo_nav = open(FIFO_NAV, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO NAV.(exit)");
        fprintf(stderr,"Error opening FIFO NAV\n");
        exit(1);
    }

    if ((global.fifo_pitot = open(FIFO_PITOT, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO PITOT.(exit)");
        fprintf(stderr,"Error opening FIFO PITOT\n");
        exit(1);
    }
    // A fifo de status flui do modulo de tempo real para o programa de modo usuario 
    // Ela eh nao-bloqueante e read_only
    if ((global.fifo_status = open(FIFO_STATUS, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO de status.(exit)");
        fprintf(stderr,"Error opening FIFO de status\n");
        exit(1);
    }
    // Esta fifo recebe um comando do modulo de tempo real, enviada pelo 
    //usuario via modem
    /*if ((global.fifo_cmd = open(FIFO_COMMAND, O_RDONLY|O_NONBLOCK)) < 0) {
        master_log(ERROR_LOG, "Initialize: Error opening FIFO de comandos do modem.(exit)");
        fprintf(stderr,"Error opening FIFO de comandos do modem\n");
        exit(1);
    }*/


    
    // Inicializa o semaforo de escrita de nome dos arquivos
    if (sem_init(&global.file_names, 0, SEM_SIZE) != 0) {
        master_log(ERROR_LOG, "Initialize: Erro ao abrir semaforo dos nomes de arquivos.(exit)");
        fprintf(stderr,"Erro ao abrir semaforo dos nomes de arquivos.\n");
        exit(1);    
    }
    
    // Inicializa o semaforo de fim da thread save_data()
    if (sem_init(&global.end_thread_save_data, 0, SEM_SIZE) != 0) {
        master_log(ERROR_LOG, "Initialize: Erro ao abrir semaforo do fim da Thread Save_data.(exit)");
        fprintf(stderr,"Erro ao abrir semaforo do fim da Thread Save_data.\n");
        exit(1);    
    }

    
    // O estado do programa eh PARADO.
    global.state = (fdc_state_t)STOPPED;
    
    // Escreve na variavel de fim da thread
    // Indispensavel para a implementacao do primeiro start
    sem_wait(&global.end_thread_save_data);
    global.end_save_data = STOPPED;
    sem_post(&global.end_thread_save_data);
    
    master_log(STATUS_LOG,"Initialize: **INICIALIZA FDC_MASTER**.");
    
}

/*!*******************************************************************************************
*********************************************************************************************/
void terminate(int s)  
{
    global.theend = 1;

    // Escreve na variavel de fim da thread
    if (global.state == RUNNING) {
        // Para a thread de coleta de dados e espera pelo seu fim
        sem_wait(&global.end_thread_save_data);
        global.end_save_data = STOPPED;
        sem_post(&global.end_thread_save_data);
        
        pthread_join(global.salva_dados,NULL);
        
        // Muda a prioridade de 'fdc_master' para o default 0.
        setpriority(PRIO_PROCESS,0,0);    
    }
        
    // Fecha as fifos de comunicacao
    close(global.fifo_ahrs);
    close(global.fifo_daq);
    close(global.fifo_gps);
    close(global.fifo_nav);
    close(global.fifo_pitot);
    close(global.fifo_control);
    close(global.fifo_status);
    //close(global.fifo_cmd);
    
    // Destroi todos os semaforos
    sem_destroy(&global.file_names);
    sem_destroy(&global.end_thread_save_data);
    
    
    if (global.ctrl_fifo != NULL) {
        fclose(global.ctrl_fifo);
        global.ctrl_fifo = NULL;
    } 
    
    // Descarrega os modulos do kernel
    unload_modules();    
    sleep(2);

    if (unlink(CTRL_FIFO)==-1){
        master_log(ERROR_LOG,"Terminate:falha ao apagar FIFO de controle criada por fdc_master.(exit)");
        exit(EXIT_FAILURE);
    }

    // Destroi o ultimo semaforo
    sem_destroy(&global.sem_log_file);
    
    fclose(global.log_file);
    
    if (s) {
        master_log(ERROR_LOG,"Terminate: **FIM ABRUPTO **. Segue tipo de sinal recebido:");
        master_log(ERROR_LOG,(const char *) strsignal(s));
        psignal(s,"** Fim **.");
        exit(EXIT_FAILURE);
    }
    
    master_log(STATUS_LOG,"Terminate: **FIM SUAVE**.");
}

/*!*******************************************************************************************
*********************************************************************************************/
/* Configura os argumentos recebidos, para o lancamento do fdc_cmd_parser. */
void parse_args(int argc,char *argv[])
{
    int n;

    if (argc == 1) return;

    for(n=1;n < argc;n++){
        if (strncmp(argv[n],"-f",2) == 0) {
            if ((n+1) <= argc) {
                n++;
                // Compoe os argumentos para chamada de fdc_cmd_parser.
                strncpy(global.config_file,argv[n],MAX_STRLEN-1);
                global.config_file[MAX_STRLEN-1] = '\0';
            }
            else {
                
                fprintf(stderr,"Argumento invalido na linha de comando.\n");
                master_log(ERROR_LOG,"Parse_args: Argumento invalido na linha de comando.(exit)");
                terminate(0);
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*!*******************************************************************************************
*********************************************************************************************/
/* Carrega os arquivo de configuracao. */
int load_config_file(void)
{
    int parser_status;
    int theend = 0;
    pid_t parser_pid;
    time_t t_now;

    parser_pid = fdc_cmd_parser(global.config_file);

    // Obtem o instante atual para inicializar a abordagem timeout
    t_now = time(NULL);

    do {
        process_message();
        
        // Verifica se fdc_cmd_parser jah terminou.
        waitpid(parser_pid,&parser_status,WNOHANG);
        if (WIFEXITED(parser_status)) {
            theend = 1;
        }

        if ((time(NULL) - t_now) > PARSER_TIMEOUT)
            theend = 2;

    }while(!theend);
    
    if (theend == 2) {
        // Mata o processo filho e retorna um erro.
        kill(parser_pid,SIGTERM);
        master_log(ERROR_LOG,"Load_config_file: Timeout excedido no processamento do arquivo de configuracao.");
        fprintf(stderr,"Timeout excedido no processamento do arquivo de configuracao.\n");
        return 0;
    }

    if (WEXITSTATUS(parser_status) != EXIT_SUCCESS) 
        return 0;
    
    // Evita que exista um 'zumbi' fdc_cmd_parser.
    waitpid(parser_pid,&parser_status,0);
        
    return 1;
}

/*!*******************************************************************************************
*********************************************************************************************/
/* Sub-processo especifico com o objetivo de
 monitorar a FIFO de comandos,
 retirando constantemente caracteres escritos e
 interpretando-os de acordo com a lexicografia definida
 em "fdc_cmd_parser.y", gerando estruturas de mensagens que podem
 ser repassadas diretamente ao processo Pai "fdc_master". */

pid_t fdc_cmd_parser(const char *args)
{
    int fdflags;
    pid_t pid;

    /* Cria pipe que sera usada para comunicacao entre o
    processo parser e o pocesso fdc_master. */
    if (pipe(global.mypipe) == -1){
        master_log(ERROR_LOG,"Uav_cmd_parser: Falha ao criar pipe entre 'fdc_cmd_parser' e 'fdc_master'.");
        perror("Falha ao criar pipe entre 'fdc_cmd_parser' e 'fdc_master'.");
        terminate(0);
        exit(EXIT_FAILURE);
    }

    /* Muda o modo de leitura do descritor de leitura do pipe
    para nao-bloqueante.*/
    fdflags = fcntl(global.mypipe[0],F_GETFL);
    if (fdflags == -1){
        master_log(ERROR_LOG,"Uav_cmd_parser: Falha ao consultar flags do descritor do pipe.(exit)");
        perror("Falha ao consultar flags do descritor do pipe.");
        terminate(0);
        exit(EXIT_FAILURE);
    }
    fdflags |= O_NONBLOCK;
    if (fcntl(global.mypipe[0],F_SETFL,fdflags) == -1) {
        master_log(ERROR_LOG,"Uav_cmd_parser: Falha ao modificar modo de acesso do pipe para nao-bloqueante.(exit)");
        perror("Falha ao modificar modo de acesso do pipe para nao-bloqueante.");
        terminate(0);
        exit(EXIT_FAILURE);
    }

    /* Lanca o processo 'fdc_cmd_parser' que cuidarah exclusivamente
    da leitura e interpretacao de comandos enviados a FIFO de controle
    do FDC, ou do arquivo de configuracao se args != NULL. */
    pid = fork();
    if (pid < ((pid_t) 0)) {
        master_log(ERROR_LOG,"Uav_cmd_parser: Falha ao criar processo filho.(exit)");
        perror("Falha ao criar processo filho.");
        terminate(0);
        exit(EXIT_FAILURE);
    }

    // Processo filho.
    if (pid == (pid_t) 0) {
        // Fecha o acesso de leitura. Somente escreve na pipe.
        close(global.mypipe[0]);

        // Associa o descritor de escrita da pipe com
        // o descritor da saida padrao, antes de exec.
        dup2(global.mypipe[1],STDOUT_FILENO);

        // Executa 'fdc_cmd_parser' indicando o arquivo de configuracao se for o caso.
        if (args == NULL)
            execl(PARSER_NAME,PARSER_NAME,NULL);
        else
            execl(PARSER_NAME,PARSER_NAME,"-f",global.config_file,NULL);

        // Se o comando acima retornou, entao houve um erro.
        master_log(ERROR_LOG,"Uav_cmd_parser: (processo filho) falha ao executar 'fdc_cmd_parser'.(exit)");
        perror("Processo filho: Falha ao executar 'fdc_cmd_parser'.");
        terminate(0);
        exit(EXIT_FAILURE);
    }

    // Processo pai.
    if (pid != (pid_t) 0){
        global.child = pid;
        // Fecha o acesso de escrita. Somente le dados.
        close(global.mypipe[1]);

        /*Verifica se o processo filho terminou abruptamente. */
        if (args == NULL) {
            pid = waitpid(global.child,NULL,WNOHANG);
            if (pid == global.child) {
                // Se 'fdc_cmd_parser' nao estah rodando => erro.
                master_log(ERROR_LOG,"Uav_cmd_parser: (processo pai)'fdc_cmd_parser' terminado abruptamente.(exit)");
                perror("Processo pai: 'fdc_cmd_parser' terminado abruptamente.");
                terminate(0);
                exit(EXIT_FAILURE);
            }
        }
    }

    return pid;
}

/*!*******************************************************************************************
*********************************************************************************************/
/*    No loop principal, uma thread eh lancada para capturar os dados enviados pela tarefa de tempo real, evitando com isso que o programa fdc_master fique preso em outro ponto, sem salvar os dados.
 Tambem, sao processados os dados enviados pelo parser e pelo modulo rt. */
void main_loop(void)
{    
    time_t led_now = time(NULL); // Verifica se o sistema esta vivo (acende led)
    
        /*strncpy(global.file_daq_name,"./teste.txt",MAX_STRLEN-1);
    
    FILE *arquivo_daq = NULL;
    if ((arquivo_daq = fopen(global.file_daq_name,"w")) < 0) {
        printf("\n Erro na abertura do arquivo");
        master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (DAQ).(exit)");
        exit(1);
    }
        
    fclose(arquivo_daq);
    */

    
    while(!global.theend) {

        /*if ((arquivo_daq = fopen(global.file_daq_name,"at")) < 0) {
                printf("\n Erro na abertura do arquivo");
                master_log(ERROR_LOG, "Save_data (thread): Erro na abertura do arquivo (DAQ).(exit)");
                exit(1);
        }*/
            
        // Processa mensagens de comando vindas do usuario.
        process_message();
        
        // Processa mensagens de comando vindas do modulo de tempo real via modem
        //modem_comand();
        
        /*if (save_daq(arquivo_daq))
            printf("Minha Falha de leitura da FIFO.\n"); // Se funcionar, aqui vai o programa de fusão de dados
        else
            printf("ok\n");
        fclose(arquivo_daq);*/    
    }
}

/*!*******************************************************************************************
*********************************************************************************************/
/*    Esta funcao recebe um comando emviado pelo fdc_cmd_parser, e o processa, repassando ao 
 modulo de tempo real, caso seja necessario. */
void process_message(void)
{
    int n;
    cmd_status_t result;
    parser_cmd_msg_t from_parser;
    char dir_files[MAX_STRLEN] = FILES_PATH; // Diretorios do arquivos -> /tmp/data
    
    // Le pipe de comunicacao entre 'fdc_master' e 'fdc_cmd_parser'.
    // Somente leh os bytes se os mesmos compuserem uma mensagem completa.
    n = read(global.mypipe[0],&from_parser,sizeof(parser_cmd_msg_t));
    if (n != sizeof(parser_cmd_msg_t))
        return;
    
    switch (from_parser.msg.cmd) {
    
        // Inicia a coleta de dados e controle em tempo real.
        case START:

        // Se o programa estiver parado, pode ser inicializado            
        if (global.state == STOPPED) {
            global.state = RUNNING; // Programa executando
            
            // Muda a prioridade de 'fdc_master' para a mais alta possivel.
            setpriority(PRIO_PROCESS,0,-20);        
            
            // Se a thread nao esta ativa (global.end_save_data = STOPPED);
            sem_wait(&global.end_thread_save_data);
            
            if (global.end_save_data == STOPPED) {
                // Lanca thread para armazenar os dados coletados em arquivos
                if (pthread_create(&global.salva_dados, NULL, save_data, (void*)0) != 0) {
                    master_log(ERROR_LOG, "Process_message: Erro ao criar a thread para salvar os dados apos comando START.(exit)");
                    fprintf(stderr,"Erro ao criar a thread para salvar os dados\n");
                    terminate(0);
                    exit(EXIT_FAILURE);
                }
            }
            // Libera o semaforo
            sem_post(&global.end_thread_save_data);
            
            result = sendcommand(&from_parser);    
            // Libera o semaforo
            sem_post(&global.end_thread_save_data);
            
            if (result == OK) {
                fprintf(stderr,"Mensagem START - OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem START - OK.");
            }
            if (result == NOT_OK) {
                fprintf(stderr,"Mensagem START - NOT_OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem START - NOT_OK.");
            }
            if (result == TIMEOUT) {
                fprintf(stderr,"Mensagem START - TIME_OUT.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem START - TIME_OUT.");
            }
        }
        else {
            fprintf(stderr,"START ja foi implementado.\n");
            master_log(STATUS_LOG, "Process_message: START ja foi implementado.");
        }
            
        break;
        ///////////////////////////////////////////////////////////////////////
        // Para a coleta de dados e controle em tempo real.
        case STOP:
        
        // Se o programa estiver rodando, ele pode ser parado.
        if (global.state == RUNNING) {
            global.state =(fdc_state_t) STOPPED;
            
            // Para a thread de coleta de dados e espera pelo seu fim
            sem_wait(&global.end_thread_save_data);
            if (global.end_save_data != STOPPED) { // Se a thread esta viva
                global.end_save_data = STOPPED;
                sem_post(&global.end_thread_save_data);// Libera o semaforo
            
                if (pthread_join(global.salva_dados,NULL) != 0)
                    master_log(ERROR_LOG,"Process_message: Falha ao terminar a thread durante STOP.");
            }
            else    // Libera o semaforo
                sem_post(&global.end_thread_save_data);
            
            // Muda a prioridade de 'fdc_master' para o default 0.
            setpriority(PRIO_PROCESS,0,0);    
            result = sendcommand(&from_parser);    
            
            if (result == OK) {
                fprintf(stderr,"Mensagem STOP - OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem STOP - OK.");
            }
            if (result == NOT_OK) {
                fprintf(stderr,"Mensagem STOP - NOT_OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem STOP - NOT_OK.");
            }
            if (result == TIMEOUT) {
                fprintf(stderr,"Mensagem STOP - TIME_OUT.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem STOP - TIME_OUT.");
            }
        }
        else {
            fprintf(stderr,"STOP ja foi implementado.\n");
            master_log(STATUS_LOG, "Process_message: STOP ja foi implementado.");
        }
            
        break;
        ///////////////////////////////////////////////////////////////////////
        // Finaliza o sistema de aquisicao e controle do FDC.
        case QUIT:
            // Espera pelo fim da thread
            if (global.state == RUNNING) {
                // Para a thread de coleta de dados e espera pelo seu fim
                sem_wait(&global.end_thread_save_data);
                if (global.end_save_data != STOPPED) { // Se a thread esta viva
                    global.end_save_data = STOPPED;
                    sem_post(&global.end_thread_save_data);// Libera o semaforo
                
                    if (pthread_join(global.salva_dados,NULL) != 0)
                        master_log(ERROR_LOG,"Process_message: Falha ao terminar a thread durante STOP.");
                }
                else    // Libera o semaforo
                    sem_post(&global.end_thread_save_data);
            }
            
            global.state = QUITTED;
            
            // Muda a prioridade de 'fdc_master' para o default 0.
            setpriority(PRIO_PROCESS,0,0);    
            result = sendcommand(&from_parser);
                
            if (result == OK) {                
                fprintf(stderr,"Mensagem QUIT - OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem QUIT - OK.");
            }
            if (result == NOT_OK) {
                fprintf(stderr,"Mensagem QUIT - NOT_OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem QUIT - NOT_OK.");
            }
            if (result == TIMEOUT) {
                fprintf(stderr,"Mensagem QUIT - TIME_OUT.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem QUIT - TIME_OUT.");
            }
        
            global.theend = 1;
            
            // Escreve na variavel de fim da thread
            sem_wait(&global.end_thread_save_data);
            global.end_save_data = STOPPED;
            sem_post(&global.end_thread_save_data);
    
        break;
        ///////////////////////////////////////////////////////////////////////
        // Muda o nome do arquivo de dados
        case CHANGEDATFILE:
        
            // Para a thread de coleta de dados e espera pelo seu fim
            sem_wait(&global.end_thread_save_data);
            global.end_save_data = STOPPED;
            sem_post(&global.end_thread_save_data);
            
            if (global.state == RUNNING) {
                if (pthread_join(global.salva_dados,NULL)!= 0)
                    master_log(ERROR_LOG,"Process_message: Falha ao terminar a thread durante CHANGEDATFILE.");
                
                global.state =(fdc_state_t) STOPPED;
            }
            
            // Espera para poder acessar os nomes de arquivos
            sem_wait(&global.file_names); 
            
            // Muda o nome do arquivo da placa daq, do ahrs, do gps, do nav ou do pitot
            if (from_parser.msg.option == DAQ) { 
            
                strncpy(global.file_daq_name,dir_files,MAX_STRLEN-1);
                global.file_daq_name[MAX_STRLEN-1] = '\0';    
                fprintf(stderr,"Novo arquivo DAQ - %s.\n",dir_files);
                master_log(STATUS_LOG, "Process_message: Mudanca de nome de arquivo (DAQ).");
            }
            
            if (from_parser.msg.option == AHRS) { 

                strncpy(global.file_ahrs_name,dir_files,MAX_STRLEN-1);
                global.file_ahrs_name[MAX_STRLEN-1] = '\0';
                fprintf(stderr,"Novo arquivo AHRS - %s.\n",dir_files);
                master_log(STATUS_LOG, "Process_message: Mudanca de nome de arquivo (AHRS).");
            }
            
            if (from_parser.msg.option == GPS) { 

                strncpy(global.file_gps_name,dir_files,MAX_STRLEN-1);
                global.file_gps_name[MAX_STRLEN-1] = '\0';
                fprintf(stderr,"Novo arquivo GPS - %s.\n",dir_files);
                master_log(STATUS_LOG, "Process_message: Mudanca de nome de arquivo (GPS).");
            }

            if (from_parser.msg.option == NAV) { 

                strncpy(global.file_nav_name,dir_files,MAX_STRLEN-1);
                global.file_nav_name[MAX_STRLEN-1] = '\0';
                fprintf(stderr,"Novo arquivo NAV - %s.\n",dir_files);
                master_log(STATUS_LOG, "Process_message: Mudanca de nome de arquivo (NAV).");
            }

            if (from_parser.msg.option == PITOT) { 

                strncpy(global.file_pitot_name,dir_files,MAX_STRLEN-1);
                global.file_pitot_name[MAX_STRLEN-1] = '\0';
                fprintf(stderr,"Novo arquivo PITOT - %s.\n",dir_files);
                master_log(STATUS_LOG, "Process_message: Mudanca de nome de arquivo (PITOT).");
            }
            
            sem_post(&global.file_names); // libera o semaforo
            
            if (global.state == STOPPED) {
                global.state = RUNNING;
                
            // Lanca thread novamente para armazenar os dados coletados no novo arquivo
                if (pthread_create(&global.salva_dados, NULL, save_data, (void*)0) != 0) {
                    master_log(ERROR_LOG, "Process_message: Erro ao criar a thread para salvar os dados apos comando CHANGEDATFILE.(exit)");
                    fprintf(stderr,"Erro ao criar novamente a thread para salvar os dados\n");
                    terminate(0);
                    exit(EXIT_FAILURE);
                }
                else {
                    fprintf(stderr,"Mensagem CHANGEDATFILE - OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem CHANGEDATFILE - OK.");
                }
            }
             
        break;
        ///////////////////////////////////////////////////////////////////////
        // Cancela a coleta de dados de um dos dispositivos
        case NODATA:
            
            // Envia o comando para o modulo 
            result = sendcommand(&from_parser);    
            
            // Retorna o status de acordo com a option
            if (from_parser.msg.option == DAQ) { 
                if (result == OK) {
                    fprintf(stderr,"Mensagem NODATA DAQ - OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA DAQ - OK.");
                }
                if (result == NOT_OK) {
                    fprintf(stderr,"Mensagem NODATA DAQ - NOT_OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA DAQ - NOT_OK.");
                }
                if (result == TIMEOUT) {
                    fprintf(stderr,"Mensagem NODATA DAQ - TIME_OUT.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA DAQ - TIME_OUT.");
                }
            }
            
            if (from_parser.msg.option == AHRS) { 
                if (result == OK) {
                    fprintf(stderr,"Mensagem NODATA AHRS - OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA AHRS - OK.");
                }
                if (result == NOT_OK) {
                    fprintf(stderr,"Mensagem NODATA AHRS - NOT_OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA AHRS - NOT_OK.");
                }
                if (result == TIMEOUT) {
                    fprintf(stderr,"Mensagem NODATA AHRS - TIME_OUT.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA AHRS - TIME_OUT.");
                }
            }
            
            if (from_parser.msg.option == GPS) { 
                if (result == OK) {
                    fprintf(stderr,"Mensagem NODATA GPS - OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA GPS - OK.");
                }
                if (result == NOT_OK) {
                    fprintf(stderr,"Mensagem NODATA GPS - NOT_OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA GPS - NOT_OK.");
                }
                if (result == TIMEOUT) {
                    fprintf(stderr,"Mensagem NODATA GPS - TIME_OUT.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA GPS - TIME_OUT.");
                }
            }
            
            if (from_parser.msg.option == NAV) { 
                if (result == OK) {
                    fprintf(stderr,"Mensagem NODATA NAV - OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA NAV - OK.");
                }
                if (result == NOT_OK) {
                    fprintf(stderr,"Mensagem NODATA NAV - NOT_OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA NAV - NOT_OK.");
                }
                if (result == TIMEOUT) {
                    fprintf(stderr,"Mensagem NODATA NAV - TIME_OUT.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA NAV - TIME_OUT.");
                }
            }

            if (from_parser.msg.option == PITOT) { 
                if (result == OK) {
                    fprintf(stderr,"Mensagem NODATA PITOT - OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA PITOT - OK.");
                }
                if (result == NOT_OK) {
                    fprintf(stderr,"Mensagem NODATA PITOT - NOT_OK.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA PITOT - NOT_OK.");
                }
                if (result == TIMEOUT) {
                    fprintf(stderr,"Mensagem NODATA PITOT - TIME_OUT.\n");
                    master_log(STATUS_LOG, "Process_message: Mensagem NODATA PITOT - TIME_OUT.");
                }
            }

        break;
        ///////////////////////////////////////////////////////////////////////
        // Inicia a filtragem dos dados
        case FILTER_ON:
        
            result = sendcommand(&from_parser);    
            
            if (result == OK) {
                fprintf(stderr,"Mensagem FILTER_ON - OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem FILTER_ON - OK.");
            }
            if (result == NOT_OK) {
                fprintf(stderr,"Mensagem FILTER_ON - NOT_OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem FILTER_ON - NOT_OK.");
            }
            if (result == TIMEOUT) {
                fprintf(stderr,"Mensagem FILTER_ON - TIME_OUT.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem FILTER_ON - TIME_OUT.");
            }
        break;
        ///////////////////////////////////////////////////////////////////////
        // Desabilita a filtragem dos dados
        case FILTER_OFF:
        
            result = sendcommand(&from_parser);    
            
            if (result == OK) {
                fprintf(stderr,"Mensagem FILTER_OFF - OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem FILTER_OFF - OK.");
            }
            if (result == NOT_OK) {
                fprintf(stderr,"Mensagem FILTER_OFF - NOT_OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem FILTER_OFF - NOT_OK.");
            }
            if (result == TIMEOUT) {
                fprintf(stderr,"Mensagem FILTER_OFF - TIME_OUT.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem FILTER_OFF - TIME_OUT.");
            }
        break;
        ///////////////////////////////////////////////////////////////////////
        // Inicia a filtragem dos dados
        case RESET_GPS:
        
            result = sendcommand(&from_parser);    
            
            if (result == OK) {
                fprintf(stderr,"Mensagem RESET_GPS - OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem RESET_GPS - OK.");
            }
            if (result == NOT_OK) {
                fprintf(stderr,"Mensagem RESET_GPS - NOT_OK.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem RESET_GPS - NOT_OK.");
            }
            if (result == TIMEOUT) {
                fprintf(stderr,"Mensagem RESET_GPS - TIME_OUT.\n");
                master_log(STATUS_LOG, "Process_message: Mensagem RESET_GPS - TIME_OUT.");
            }
        break;
        ///////////////////////////////////////////////////////////////////////
        default: 
        
        break;
    } // end switch
}
/*!*******************************************************************************************
*********************************************************************************************/
/*    Esta funcao envia um comando para o modulo de tempo real e permanece a espera da 
 resposta deste comando de forma bloqueada. */
cmd_status_t sendcommand(parser_cmd_msg_t* parser_msg_to_rt)
{
    cmd_status_t msg_from_slave;  // Retorna o status de leitura
    cmd_msg_t cmd_to_slave;    // Mensagem enviada contendo opcoes e comandos 
    int n, i;
            
    // Fatora a menssagem recebida do parser, isolando somente a parte de comando (.msg)
    cmd_to_slave = parser_msg_to_rt->msg;
    
    // Escrita na fifo de controle de modo nao-bloqueante
    write(global.fifo_control,&cmd_to_slave, sizeof(cmd_to_slave));
            
    for (i=0; i<10; i++) {
        usleep(10000); // Dorme cerca de 10 ms 

        // Efetua agora a leitura da resposta de forma nao-bloqueante
        n = read(global.fifo_status, &msg_from_slave, sizeof(msg_from_slave));
    
        if (n == sizeof(msg_from_slave))
            return msg_from_slave; // Retorna a mensagem recebida
    }
    
    // Caso nao tenha conseguido efetuar a leitura, retorna um erro de comunicacao
    return TIMEOUT;
}
/*!*******************************************************************************************
*********************************************************************************************/
/*    Efetua o log do sistema, salvando em arquivo as mensagens de erro e status.
    - type_message = STATUS_LOG (messagem de status) ou ERROR_LOG (mensagem de erro).*/
int master_log(int type_message, const char* msg)
{
    time_t time_log;     
    char *time_log_char;
    
    // Pega o tempo de execussao do sistema e o salva no arquivo de log
    time_log = time(NULL);
    time_log_char = ctime(&time_log);
    
    // Requisita o semaforo
    sem_wait(&global.sem_log_file);
    
    // Imprime o tempo 
    fprintf(global.log_file,"%s\t",time_log_char);
    
    // Imprime mensagem no arquivo de LOG.
    fprintf(global.log_file,msg);
    
    if (type_message == STATUS_LOG)
        fprintf(global.log_file,"\n\n");

    if (type_message == ERROR_LOG)
        // Imprime tambem o numero do ultimo erro (errno).
        fprintf(global.log_file,"> %s.\n\n",strerror(errno));
        
    // Forca sempre a escrita dos dados no arquivo
    fflush(global.log_file);
        
    // Libera o semaforo
    sem_post(&global.sem_log_file);
        
    return 0;
}

/*!*******************************************************************************************
*********************************************************************************************/
int load_modules (void) 
{
    if (system("modprobe rtai_serial") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_serial");
    if (system("modprobe rtai_fifos") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_fifos");
    if (system("modprobe crc8") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo crc8");
    
    if (system("insmod ./object/rtai_daq.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_daq");
    if (system("insmod ./object/rtai_gps.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_gps");
    if (system("insmod ./object/rtai_ahrs.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_ahrs");
    if (system("insmod ./object/rtai_nav.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_nav");
    if (system("insmod ./object/rtai_pitot.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo rtai_pitot");
    if (system("insmod ./object/modem.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo modem");
    if (system("insmod ./object/epos.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo epos");
    if (system("insmod ./object/fdc_slave.o") < 0)
        master_log(ERROR_LOG, "Inicialize: Falha ao carregar modulo fdc_slave");
    
    master_log(STATUS_LOG, "Inicialize: modulos carregados.");
    
    return 1;
}

/*!*******************************************************************************************
*********************************************************************************************/
int unload_modules (void) 
{
    // Fecha as fifos de comunicacao
    close(global.fifo_daq);
    //close(global.fifo_gps);
    close(global.fifo_control);
    close(global.fifo_status);

    if (system("rmmod fdc_slave") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo fdc_slave");
    if (system("rmmod rtai_pitot") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_pitot");
    if (system("rmmod rtai_nav") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_nav");
    if (system("rmmod rtai_gps") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_gps");
    if (system("rmmod rtai_ahrs") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_ahrs");
    if (system("rmmod rtai_daq") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_daq");
    if (system("rmmod modem") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo modem");
    if (system("rmmod epos") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo epos");
    
    /*
    if (system("rmmod rtai_fifos") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_fifos");
    if (system("rmmod rtai_serial") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_serial");
    if (system("rmmod rtai_sem") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_sem");
    if (system("rmmod rtai_ksched") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_ksched");
    if (system("rmmod rtai_hal") < 0)
        master_log(ERROR_LOG, "Terminate: Falha ao descarregar modulo rtai_hal");
    */
    master_log(STATUS_LOG, "Terminate: modulos descarregados.");
    
    return 1;
}

/*!*******************************************************************************************
*********************************************************************************************/
/*    Processa as mensagens enviadas via modem e intermediadas pelo modulo de tempo real.*/
int modem_comand(void)
{
    char ch;
    // Le o comando da fifo de comandos e coloca um mensagem para o comando parser
    if (read(global.fifo_cmd, &ch, sizeof(char)) == sizeof(char)) { // Leitura efetuada com sucesso
        
        switch (ch) {    // Envia o comando para o fdc_master
        
            case START_CMD:
                system("echo -e \" start\n \" > /tmp/fdc_ctrl");
            break;
            
            case STOP_CMD:
                system("echo -e \" stop\n \" > /tmp/fdc_ctrl");
            break;
            
            case RESET_GPS_CMD:
                system("echo -e \" reset_gps\n \" > /tmp/fdc_ctrl");
            break;
            
            default:
            break;
        }
    }
    return 0;
}
