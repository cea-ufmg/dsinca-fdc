#ifndef _FDC_CMD_PARSER_H
#define _FDC_CMD_PARSER_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "messages.h"

// Menor e maior numero de canal para a placa DAQ.
#define DAQ_CHMIN 0
#define DAQ_CHMAX 15

// Variaveis globais. :-(

int theend, // Indica o fim do programa.
    debug;  // Indica se o modo de depuracao estah ativo.

// Descritor de arquivo do canal de envio das mensagens processadas.
// Em condicoes normais, serah uma das pontas do 'pipe'.
int out;

// Mensagem, resultado final da analise lexicografica,
// enviada a 'fdc_master', apos a analise de cada comando
// escrito pelo usuario na FIFO de controle do UAV.
parser_cmd_msg_t result;

// Funcao para "limpar" o resultado do analisador lexico.
void clear_msg(void);

#endif

