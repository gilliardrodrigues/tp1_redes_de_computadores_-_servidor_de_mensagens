#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 500
#define MAX_SENSORES_POR_VEZ 3

bool sistema[4][4] = { false };
int qtdSensoresDisponiveis = 15;


int validaId(char *strId) {
    if(strlen(strId) != 2)
        return -1;
    int id = atoi(strId);
    printf("ID QUE CHEGOU NO VALIDADOR: %d\n", id);
    if(id < 1 || id > 4)
        return -1;
    return 0;
}

void instalarSensor(char *instrucao, int socketCliente) {
    char msg[500];
    memset(msg, 0, 500);
    int sensores[MAX_SENSORES_POR_VEZ];
    char *entrada = strtok(instrucao, " ");
    entrada = strtok(instrucao, " ");
    int numSensores = 0;
    bool atingiuMaxSensores = false;
    bool entradaInvalida = false;
    while(strcmp(entrada, "in") != 0 && !entradaInvalida) { //Para de iterar após o último sensor
        if(validaId(entrada) == -1){
            entradaInvalida = true;
            strcat(msg, "invalid sensor ");
        }
        else {
            sensores[numSensores] = atoi(entrada);
            numSensores++;
        }
        entrada = strtok(NULL, " ");
    }
    entrada = strtok(NULL, " ");
    char msgSensorDuplicado[100];
    memset(msgSensorDuplicado, 0, 100);
    if(!entradaInvalida){
        if(validaId(entrada) == -1)
            strcat(msg, "invalid equipment ");
        else {
            int equipamentoId = atoi(entrada);
            strcat(msg, "sensor");
            char idStr[4];
            bool fezAlteracao = false;
            for(int i = 0; i < numSensores; i++){
                if(!sistema[equipamentoId-1][sensores[i]-1]){
                    if(qtdSensoresDisponiveis > 0){
                        sistema[equipamentoId-1][sensores[i]-1] = true;
                        sprintf(idStr, " 0%d", sensores[i]);
                        strcat(msg, idStr);
                        qtdSensoresDisponiveis -= 1;
                        fezAlteracao = true;
                    }
                    else {
                        atingiuMaxSensores = true;
                        memset(msg, 0, 500);
                        strcpy(msg, "limit exceeded ");
                        break;
                    }
                }
                else {
                    sprintf(idStr, "0%d ", sensores[i]);
                    strcat(msgSensorDuplicado, idStr);
                    strcat(msgSensorDuplicado, "already exists in ");
                    sprintf(idStr, "0%d ", equipamentoId);
                    strcat(msgSensorDuplicado, idStr);
                }
            }
            if(!atingiuMaxSensores && (strcmp(msgSensorDuplicado, "") != 0 && fezAlteracao))
                strcat(msg, " added ");
            else if(!atingiuMaxSensores && fezAlteracao)
                strcat(msg, " added ");
            else
                strcat(msg, " ");
            strcat(msg, msgSensorDuplicado);
        }
    }
    msg[strlen(msg)-1] = '\n';
    int numBytes = send(socketCliente, msg, strlen(msg), 0);
    if(numBytes != strlen(msg))
        exibirLogSaida("send");
}

void consultarEquipamento(char *instrucao, int socketCliente) {
    char msg[500];
    memset(msg, 0, 500);
    bool entrou = false;
    char *texto = strtok(instrucao, " ");
    while(strcmp(texto, "in") != 0) {
        texto = strtok(NULL, " ");
    }
    char *id = strtok(NULL, " ");
    int idEquipamento = atoi(id);
    char idSensor[4];
    for(int i = 0; i < 4; i++) {
        if(sistema[idEquipamento-1][i] == true) {
            sprintf(idSensor, "0%d ", i+1);
            strcat(msg, idSensor);
            entrou = true;
        }
    }
    if(!entrou)
        strcat(msg, "none ");
    msg[strlen(msg)-1] = '\n';
    int numBytes = send(socketCliente, msg, strlen(msg), 0);
    if(numBytes != strlen(msg))
        exibirLogSaida("send");
}

int avaliarComando(char *comando, int socketCliente) {
    char *instrucao = strtok(comando, " ");
    while(instrucao != NULL) {
        if(strcmp(instrucao, "add") == 0) {
            instalarSensor(NULL, socketCliente);
        }
        else if(strcmp(instrucao, "remove") == 0) {
            //removerSensor();
        }
        else if(strcmp(instrucao, "list") == 0) {
            consultarEquipamento(NULL, socketCliente);
        }
        else if(strcmp(instrucao, "read") == 0) {
            //consultarVariavelProcesso();
        }
        else {
            close(socketCliente);
            return -1;
        }
        instrucao = strtok(NULL, " ");
    }
    return 0;
}

void exibirInstrucoesDeUso(int argc, char **argv) {
    printf("Uso: %s <v4|v6> <Porta do servidor>\n", argv[0]);
    printf("Exemplo: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        exibirInstrucoesDeUso(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != inicializarSockAddrServer(argv[1], argv[2], &storage)) {
        exibirInstrucoesDeUso(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        exibirLogSaida("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        exibirLogSaida("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        exibirLogSaida("bind");
    }

    if (0 != listen(s, 10)) {
        exibirLogSaida("listen");
    }

    char addrstr[BUFSZ];
    converterEnderecoEmString(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            exibirLogSaida("accept");
        }

        char caddrstr[BUFSZ];
        converterEnderecoEmString(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);

        char buf[BUFSZ];
        int auxRetorno = 0;
        unsigned totalBytes;
        size_t numBytes;
        char *linhas;
        while(1){
            memset(buf, 0, BUFSZ);
            totalBytes = 0;
            while(buf[strlen(buf)-1] != '\n') {
                numBytes = recv(csock, buf + totalBytes, BUFSZ - totalBytes, 0);
                if(numBytes == 0){
                    auxRetorno = -1;
                    break;
                }
                totalBytes += numBytes;
            }
            if(auxRetorno == -1)
                break;
            printf("%s", buf);
            linhas = strtok(buf, "\n");
            if(strcmp(buf, "kill") == 0){
                close(csock);
                exit(EXIT_SUCCESS);
                return 0;
            }

            while(linhas != NULL){
		auxRetorno = avaliarComando(buf, csock);
		if(auxRetorno == -1){
                    close(csock);
                    exit(EXIT_SUCCESS);
                    return 0;
                }
                linhas = strtok(NULL, "\n");
            }
	    if(auxRetorno == -1)
                break;
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}

