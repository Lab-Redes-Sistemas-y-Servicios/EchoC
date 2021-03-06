#include <stdio.h>
#include <stdlib.h>
#include <string.h>         //Funciones con cad de caracteres
#include <locale.h>         //Formato español
#include <signal.h>         //CRTL-C
#include <stdbool.h>        //Variables booleanas
#include <unistd.h>         
#include <arpa/inet.h>      //Funciones orientadas a conexión
#include <sys/time.h>       //Cronometrar tiempos

//-------------------- CTRL-C STOP --------------------
static bool keepRunning = true;
void intHandler(int x) {
    keepRunning = false;
}
//-----------------------------------------------------
void timeEstad(float *t, float *tmin, float *tmax) {            //Ésta función compara los tiempos para 
    if(*tmax == 0 && *tmin == 0) {                              //determinar cual es el tmin y el tmax
        *tmax = *tmin = *t;
    }
    else if(*t > *tmax) {
        *tmax = *t;
    }
    else if(*t < *tmin) {
        *tmin = *t;
    }
}

int main(int argc, char const *argv[]){

    //Variables sockets
    int servSock, cliSock, len;
    int servPort = atoi(argv[1]);
    char dataRecv[1024];
    ssize_t bytesRecv;

    struct sockaddr_in servAddr, cliAdrr;
    int cliAdrrLen = sizeof(cliAdrr);

    //Variables del tiempo
    struct timeval ini;
    struct timeval fin;
    float time;
    float tmax = 0.0;
    float tmin = 0.0;
    int numEnv = 0;

    //CTRL-C STOP
    struct sigaction signCTRL;
    signCTRL.sa_handler = intHandler;
    sigaction(SIGINT, &signCTRL, NULL);

    if(argc != 2){                                      //Comprobamos el nº de argumentos
        printf("\nERR: nº de argumentos no válido\n");
        exit(-1);
    }

    if(servPort < 1023){                                //Comprobamos el nº de puerto
        printf("\nERR: El nº de puerto debe ser mayor que 1023\n");
        exit(-1);
    }

    servSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    //Inicializamos el socket UDP

    if(servSock < 0) {
        printf("\nERR: No se pudo crear el socket\n");
        exit(-1);
    }

    //Estructura de direcciones
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servPort);
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {   //Asociación del socket
        printf("\nERR: No se pudo conectar\n");
        exit(-1);
    }

    while (keepRunning) {                   //Búcle infinito
    
        //Recibimos bytes del cliente
        bytesRecv = recvfrom(servSock, dataRecv, sizeof(dataRecv), 0, (struct sockaddr*)&cliAdrr, &cliAdrrLen);

        gettimeofday(&ini, 0);              //Iniciamos el "cronómetro"
        
        printf("-Cliente: %s/%d   ", inet_ntoa(cliAdrr.sin_addr), ntohs(cliAdrr.sin_port));
        
        if(bytesRecv < 0 && numEnv == 0) {
            printf("\nERR: No se pudo recibir datos del cliente\n");
            exit(-1);
        }
        else if(bytesRecv < 0 && numEnv != 0){
            break;
        }
        else{
            printf("bytes= %d   ", bytesRecv);
        }

        //Enviamos los bytes recibidos al cliente
        ssize_t bufferSend = sendto(servSock, dataRecv, sizeof(dataRecv), 0, (struct sockaddr*)&cliAdrr, cliAdrrLen);

        if (bufferSend < 0) {
            printf("\nERR: No se pudo enviar");
            exit(-1);
        }
        else if(strlen(dataRecv) != bytesRecv) {
            printf("\nERR: Nº de bytes inesperados");
            exit(-1);
        }

        gettimeofday(&fin, 0);              //Apagamos el "cronómetro"
        
        //Calculamos el tiempo en realizar las conexiones
        time = (fin.tv_sec - ini.tv_sec) * 1000.0f + (fin.tv_usec - ini.tv_usec) / 1000.0f;
        timeEstad(&time, &tmin, &tmax);

        printf("time= %.3f\n", time);
        numEnv++;                           //Incremento del nº de paquetes
    }

    printf("\n------Estadísticas------\n %i paquetes transmitidos\n tmax= %.3f tmin= %.3f tmed= %.3f\n", numEnv, tmax, tmin, ((tmax+tmin)/2));

    close(servSock);                        //Finalizamos el socket UCP

    return 0;
}