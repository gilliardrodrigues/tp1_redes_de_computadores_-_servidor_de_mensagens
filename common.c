#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

void exibirLogSaida(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int parsearEndereco(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
    if(addrstr == NULL || portstr == NULL)
        return -1;
    
    uint16_t port = (uint16_t) atoi(portstr); // Unsigned short
    if(port == 0)
        return -1;
    port = htons(port); // Host para network short

    struct in_addr inaddr4; // Endereço IP de 32 bits
    if(inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // Endereço IP de 128 bits
    if(inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }
    return -1;
}

void converterEnderecoEmString(const struct sockaddr *addr, char *str, size_t strsize) {
    int versao;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if(addr->sa_family == AF_INET) {
        versao = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            exibirLogSaida("ntop");
        }
        port = ntohs(addr4->sin_port); // Network para host short
    }
    else if(addr->sa_family == AF_INET6) {
        versao = 6;
        struct sockaddr_in6 *addr6 =  (struct sockaddr_in6 *) addr;
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            exibirLogSaida("ntop");
        }
        port = ntohs(addr6->sin6_port); //
    } else {
        exibirLogSaida("unknown protocol family.");
    }
    if(str)
        snprintf(str, strsize, "IPv%d %s %hu", versao, addrstr, port);
}

int inicializarSockAddrServer(const char *protocolo, const char *portstr,struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host para network short

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(protocolo, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    } else if (0 == strcmp(protocolo, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    } else {
        return -1;
    }
}
