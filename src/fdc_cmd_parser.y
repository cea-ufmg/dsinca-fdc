/*
   Descricao lexicografica de comandos para
   interacao com o programa "fdc_master".
*/

/* COMMANDS		start | stop | change | nodata | enable | disable| assign */
/* OPTIONS		ts | datfile | daqchannel | daq | gps | ahrs | temperature | alpha | beta | pstat | pdyn | nav | pitot */

%option case-insensitive noyywrap


%{
#include "fdc_cmd_parser.h"
%}


/*Definicoes das condições 'captura de string',
'captura de numero' e 'captura de lista de canais da placa DAQ'.*/
%x STRING_CAPTURE INTEGER_CAPTURE FLOAT_CAPTURE CHANNEL_LIST COMMENTS

%%

"%" {
	if (debug) printf("Comentario (ignorado):\n\t");
	BEGIN(COMMENTS);
}
<COMMENTS>[^\n] { if (debug) ECHO; }
<COMMENTS>\n {
		if (debug)
			printf("\n");
		BEGIN(INITIAL);
}

"start"|"begin"	{
		result.msg.cmd = START;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug)
			printf("Iniciar coleta de dados e controle em tempo real.\n");
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
	}

"stop"|"pause" {
		result.msg.cmd = STOP;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug)
			printf("Finalizar coleta de dados e controle em tempo real.\n");
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
	}

"quit"|"exit"|"terminate" {
		result.msg.cmd = QUIT;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug)
			printf("Terminar sistema de aquisicao de dados e controle.\n");
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
		theend = 1;
	}

"change"|"chg"  {
		result.msg.cmd = CHANGE;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug) printf("Mudar\n");
	}

"nodata"  {
		result.msg.cmd = NODATA;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug) printf("Eliminar saida de dados.\n");
	}
	
"filteron"|"filter"	{
		result.msg.cmd = FILTER_ON;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug)
			printf("Ativar fltragem de dados.\n");
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
	}

"filteroff"|"nofilter"	{
		result.msg.cmd = FILTER_OFF;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug)
			printf("Desativar fltragem de dados.\n");
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
	}

"enable"|"en" {
		result.msg.cmd = ENABLEDAQ;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug) printf("Habilitar canal da placa DAQ.\n");
	  }

"disable"|"dis" {
		result.msg.cmd = DISABLEDAQ;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug) printf("Desabilitar canal da placa DAQ.\n");
	  }

"assign"|"link"|"connect" {
		result.msg.cmd = ASSIGN;
		result.msg.option = NO_OPTION;
		result.msg.data = 0;
		result.name[0] = '\0';
		if (debug) printf("Atribuir canal da placa DAQ a variavel analogica.\n");
	}

"datfile"|"dat "|"file" {
		if (result.msg.cmd == CHANGE) {
			result.msg.cmd = CHANGEDATFILE;
			result.msg.option = DATFILE;
			if (debug)
				printf("Arquivo de dados.\n");
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"ts"|"t_s"|"samp_time" {
		if (result.msg.cmd == CHANGE) {
			result.msg.cmd = CHANGETS;
			result.msg.option = TS;
			if (debug)
				printf("Tempo de amostragem.\n");
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"daqchannel"|"channel"|"ch"|"daqch" {
		if ((result.msg.cmd == ENABLEDAQ) ||
		    (result.msg.cmd == DISABLEDAQ)) {
			result.msg.option = DAQCHANNEL;
			if (debug)
				printf("Canal da placa DAQ.\n");
			// Prepara 'data' para receber os canais.
			result.msg.data = -1;
			BEGIN(CHANNEL_LIST);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}

	}

"gps"	{
		if ((result.msg.cmd == CHANGEDATFILE)||
		    (result.msg.cmd == CHANGETS)||
		    (result.msg.cmd == NODATA)) {
			result.msg.option = GPS;
			if (debug)
				printf("GPS.\n");
			if (result.msg.cmd == CHANGEDATFILE)
				BEGIN(STRING_CAPTURE);
			if (result.msg.cmd == CHANGETS)
				BEGIN(INTEGER_CAPTURE);
			if (result.msg.cmd == NODATA)
				write(out,&result,sizeof(parser_cmd_msg_t));
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}
	
"reset_gps"	{
			result.msg.cmd = RESET_GPS;
			result.msg.option = NO_OPTION;
			result.msg.data = 0;
			result.name[0] = '\0';
			if (debug)
				printf("Requisicao para resetar modulo ET-102 via software.\n");
			else
				write(out,&result,sizeof(parser_cmd_msg_t));
	}

"ahrs"|"imu"	{
		if ((result.msg.cmd == CHANGEDATFILE)||
		    (result.msg.cmd == CHANGETS)||
		    (result.msg.cmd == NODATA)) {
			result.msg.option = AHRS;
			if (debug)
				printf("AHRS.\n");
			if (result.msg.cmd == CHANGEDATFILE)
				BEGIN(STRING_CAPTURE);
			if (result.msg.cmd == CHANGETS)
				BEGIN(INTEGER_CAPTURE);
			if (result.msg.cmd == NODATA)
				write(out,&result,sizeof(parser_cmd_msg_t));
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}
	
"nav"	{
		if ((result.msg.cmd == CHANGEDATFILE)||
		    (result.msg.cmd == CHANGETS)||
		    (result.msg.cmd == NODATA)) {
			result.msg.option = NAV;
			if (debug)
				printf("NAV.\n");
			if (result.msg.cmd == CHANGEDATFILE)
				BEGIN(STRING_CAPTURE);
			if (result.msg.cmd == CHANGETS)
				BEGIN(INTEGER_CAPTURE);
			if (result.msg.cmd == NODATA)
				write(out,&result,sizeof(parser_cmd_msg_t));
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}
	
"pitot" {
		if ((result.msg.cmd == CHANGEDATFILE)||
		    (result.msg.cmd == CHANGETS)||
		    (result.msg.cmd == NODATA)) {
			result.msg.option = PITOT;
			if (debug)
				printf("PITOT.\n");
			if (result.msg.cmd == CHANGEDATFILE)
				BEGIN(STRING_CAPTURE);
			if (result.msg.cmd == CHANGETS)
				BEGIN(INTEGER_CAPTURE);
			if (result.msg.cmd == NODATA)
				write(out,&result,sizeof(parser_cmd_msg_t));
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"daq"	{
		if ((result.msg.cmd == CHANGEDATFILE)||
		    (result.msg.cmd == CHANGETS)||
		    (result.msg.cmd == NODATA)) {
			result.msg.option = DAQ;
			if (debug)
				printf("DAQ.\n");
			if (result.msg.cmd == CHANGEDATFILE)
				BEGIN(STRING_CAPTURE);
			if (result.msg.cmd == CHANGETS)
				BEGIN(INTEGER_CAPTURE);
			if (result.msg.cmd == NODATA)
				write(out,&result,sizeof(parser_cmd_msg_t));
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"modem" {
		if (result.msg.cmd == CHANGETS) {
			result.msg.option = MODEM;
			if (debug)
				printf("Modem.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"internal_temperature"|"internal_temp"|"int_temp" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = INTERNAL_TEMPERATURE;
			if (debug)
				printf("Temperatura interna da caixa de instrumentacao.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"engine_temperature"|"engine_temp"|"eng_temp" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = INTERNAL_TEMPERATURE;
			if (debug)
				printf("Temperatura do motor.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"alpha"|"alfa"	{
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = ALPHA;
			if (debug)
				printf("Alfa: angulo de ataque.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"beta" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = BETA;
			if (debug)
				printf("Beta: angulo de derrapagem.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
       }

"pstat"|"ps"|"pstatic"|"static_pressure" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = PSTAT;
			if (debug)
				printf("Pressao estatica.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"pdyn"|"pd"|"dynamic_pressure"|"pdynamic" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = PDYN;
			if (debug)
				printf("Pressao dinamica.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"loadcell"|"ldcell"|"straingauge"|"strain_gauge"|"sg" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = LOADCELL;
			if (debug)
				printf("Celula de carga -> tracao.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}

"rpm"|"enginerpm"|"engine_rpm"|"engine" {
		if (result.msg.cmd == ASSIGN) {
			result.msg.option = ENGINE_RPM;
			if (debug)
				printf("Rotacao do motor.\n");
			BEGIN(INTEGER_CAPTURE);
		}
		else {
			clear_msg();
			fprintf(stderr,"Falha no analisador lexico. Token = %s.\n",yytext);
		}
	}


<STRING_CAPTURE>[^[:blank:]\n]*	{
		/* Obtem as strings, eliminando os caracteres "brancos" iniciais
		e parando caso ocorra um espaco no meio da string.*/
		strncpy(result.name,yytext,MAX_STRLEN);
		result.name[MAX_STRLEN-1]= '\0';
		if (debug)
			printf("String = %s.\n",result.name);
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
		/* Volta ao estado inicial quando encontrar '\n' */
		BEGIN(INITIAL);
	}

<INTEGER_CAPTURE>([[:digit:]]*) {
		/* Captura um inteiro.*/
		result.msg.data = atoi(yytext);
		
		if (debug)
			printf("Numero = %s\n",yytext);
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
		
		BEGIN(INITIAL);
	}

<FLOAT_CAPTURE>([0-9]+"."+[0-9]+) {
		/* Captura um numero float.*/
		result.msg.data = ((fdc_cmd_data_t) atof(yytext));
		
		if (debug) 
			printf("Numero = %s\n",yytext);
		else
			write(out,&result,sizeof(parser_cmd_msg_t));
		
		BEGIN(INITIAL);
	}

<CHANNEL_LIST>([[:digit:]]*) {
		int ch = -1;
		if (debug) printf("Canal = %s\n",yytext);
		ch = atoi(yytext);
		/*Seta os bits correspondentes aos canais na variavel data.*/
		if ((ch >= DAQ_CHMIN) && (ch <= DAQ_CHMAX)) {
			if (result.msg.data == -1)
				result.msg.data = 0;
			result.msg.data |= (1 << ch);
			if (debug) printf("Data = %d\n",result.msg.data);
		}
	}
<CHANNEL_LIST>(,) {
		if (debug) printf("mais um...\n");
	}
<CHANNEL_LIST>\n {
		if ((!debug) && (result.msg.data != -1))
			write(out,&result,sizeof(parser_cmd_msg_t));
		BEGIN(INITIAL);
	}

%%

void clear_msg(void)
{
	result.msg.cmd = NO_CMD;
	result.msg.option = NO_OPTION;
	result.msg.data = 0;
	result.name[0] = '\0';
}

void main(int argc, char *argv[])
{
	int n, cfg_file_arg;
	FILE *ctrl_fifo;
	FILE *cfg_file;

	// Inicializa o ponteiro para o arquivo de configuracao.
	// Incialmente nao ha argumento relativo ao arquivo de configuracao.
	cfg_file = NULL;
	cfg_file_arg = 0;

	// Inicializa as variaveis globais.
	theend = 0;
	debug = 0; // Modo de depuracao desativado por default.

	// Canal de saida para mensagens default eh a saida padrao.
	out = STDOUT_FILENO;

	// Processa os argumentos da linha de comando.
	// -d             -> ativa modo de depuracao.
	// -f config_file -> aponta um arquivo de configuracao para processamento.
	for(n=1;n < argc;n++) {
		// Verifica se o modo de depuracao deve ser ativado.
		if ((strncmp("-d",argv[n],2) == 0) && (debug == 0)) {
			debug = 1;
			yyout = stdout;
		}

		// Recebe o arquivo de configuracao como entrada a ser analisada.
		if (strncmp(argv[n],"-f",2) == 0) {
			if ((n+1) <= argc) {
				n++;
				// Armazena a posicao do argumento que define o nome do
				// arquivo de configuracao.
				cfg_file_arg = n;
			}
			else {
				fprintf(stderr,"Argumento invalido na linha de comando.\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	// Eliminar a saida de mensagens de erro do analisador lexicografico.
	/*if (!debug) {
		fclose(stderr);
		stderr = NULL;
	}*/

	// Saida de mensagens de erro do analisador lexico vao para a saida padrao de erro.
	if (!debug)
		yyout = stderr;

	// Verifica se a tarefa eh ler um arquivo de configuracao ou a FIFO de controle.
	if (cfg_file_arg) {
		cfg_file = fopen(argv[cfg_file_arg],"r");
		if (cfg_file == NULL) {
			fprintf(stderr, "Falha ao abrir o arquivo texto '%s' para processamento: ",argv[cfg_file_arg]);
			perror("");
			exit(EXIT_FAILURE);
		}

		yyin = cfg_file;

		// Processa o arquivo de configuracao.
		yylex();

		fclose(cfg_file);
		close(out);
	}
	else {
		// Abre a FIFO de comando para leitura.
		ctrl_fifo = fopen(CTRL_FIFO,"r");
		if (ctrl_fifo == NULL) {
			perror("Falha ao abrir a FIFO para leitura.");
			exit(EXIT_FAILURE);
		}

		yyin = ctrl_fifo;

		// Processa as strings enviadas para a FIFO de controle.
		do {
			clear_msg();
			yylex();
		}while (!theend);
	}

	exit(EXIT_SUCCESS);
}


