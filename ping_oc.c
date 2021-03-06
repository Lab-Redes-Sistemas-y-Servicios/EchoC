#include <stdio.h>
#include <stdlib.h>
#include <string.h>         //Funciones con cad de caracteres
#include <locale.h>         //Formato español
#include <signal.h>         //CRTL-C
#include <stdbool.h>        //Variables booleanas
#include <unistd.h>         
#include <netdb.h>
#include <arpa/inet.h>      //Funciones orientadas a conexión
#include <sys/time.h>       //Cronometrar tiempos

//-------------------- CTRL-C STOP --------------------
static bool keepRunning = true;
void intHandler(int x) {
    keepRunning = false;
}
//-----------------------------------------------------

void timeEstad(float *t, float *tmin, float *tmax) {		//Esta función compara los tiempos para 
    if(*tmax == 0 && *tmin == 0) {				            //determinar cual es el tmin y el tmax
        *tmax = *tmin = *t;
    }
    else if(*t > *tmax) {
        *tmax = *t;
    }
    else if(*t < *tmin) {
        *tmin = *t;
    }
}

void hostNameToIP(char* hostName, char* IP) {			//Esta función obtiene del DNS la
                                                        //dirección IPV4 del host
    struct hostent *h;
	struct in_addr **add;
	int i;
		
    h = gethostbyname(hostName);
	add = (struct in_addr **) h->h_addr_list;
	
	for(i = 0; add[i] != NULL; i++) 
	{
		strcpy(IP , inet_ntoa(*add[i]) );
	}
}

int main(int argc, char const *argv[]){    

    //Variables sockets
    int cliSock;
    int servPort = atoi(argv[2]);  
    char *host = argv[1];
    char servIP[50];

    hostNameToIP(host, servIP);       //Obtenemos la dcc IP del host       
       

    char dataRecv[1024];

    char * echo = "abcd";
    int echoLen = strlen(echo);

    struct sockaddr_in server;

    //Variables de tiempo
    struct timeval ini;
    struct timeval fin;
    float time;
    float tmax = 0.0;
    float tmin = 0.0;
    float tmed = 0.0;
    int numEnv = 0;

    //CTRL-C STOP
    struct sigaction signCTRL;
    signCTRL.sa_handler = intHandler;
    sigaction(SIGINT, &signCTRL, NULL);

    if(argc != 3){                                      //Comprobamos el nº de argumentos
        printf("\nERR: nº de argumentos no válido\n");
        exit(-1);
    }

    if(servPort < 1023){                                //Comprobamos el nº de puerto
        printf("\nERR: El nº de puerto debe ser mayor que 1023\n");
        exit(-1);
    }


    while (keepRunning) {
        cliSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    //Inicializamos el socket TCP
        if(cliSock < 0) {
            printf("\nERR: No se pudo crear el socket\n");
            exit(-1);
        }

        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;

        //Análisis de la dirección del cliente
        int ctrlVal = inet_pton(AF_INET, servIP, &server.sin_addr.s_addr);
        if(ctrlVal == 0) {
            printf("\nERR: Direccion no válida\n");
            exit(-1);
        }else if(ctrlVal < 0) {
            printf("\nERR: Se produjo un error en la función inet_pton()\n");
            exit(-1);            
        }
        server.sin_port = htons(servPort);

        if(connect(cliSock, (struct sockaddr * ) & server, sizeof(server)) < 0) {   //Conexión con serv
            printf("\nERR: No se pudo conectar\n");
            exit(-1);
        }

        gettimeofday(&ini, 0);                  //Iniciamos el "cronómetro"

        ssize_t bytesSent = send(cliSock, echo, echoLen, 0);    //Enviamos echo= “abcd”
        if(bytesSent < 0) {
            printf("\nERR: No se pudo enviar\n");
            exit(-1);
        }else if (bytesSent != echoLen){
            printf("\nERR: No se pudo enviar\n");
            exit(-1);
        }

        unsigned int totalBytesRecv = 0;
        while (totalBytesRecv < echoLen) {
            ssize_t bytesRecv = recv(cliSock, dataRecv, 1023, 0);   	//Recibimos datos del servidor
            gettimeofday(&fin, 0);                                      //Apagamos el "cronómetro"

            if (bytesRecv < 0) {
                printf("\nERR: No se pudo recibir datos del servidor\n");
                exit(-1);
            }

            totalBytesRecv += bytesRecv;

            //Calculamos el tiempo en realizar las conexiones
            time = (fin.tv_sec - ini.tv_sec) * 1000.0f + (fin.tv_usec - ini.tv_usec) / 1000.0f;
            timeEstad(&time, &tmin, &tmax); //Obtenemos los tiempos tmax y tmin

            printf("-Servidor: %s/%d   bytes= %d   time= %.3f\n", servIP, servPort, bytesRecv, time);
            numEnv++;
        }
        close(cliSock);
        sleep(1);
    }

    //Al terminar el bucle (CTRL-C) mostramos las estadísticas del ping
    printf("\n------Estadísticas------\n %i paquetes transmitidos\n tmax= %.3f tmin= %.3f tmed= %.3f\n", numEnv, tmax, tmin, ((tmax+tmin)/2));

    return 0;
}