#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LONGITUD_MSG 100           // Payload del mensaje
#define LONGITUD_MSG_ERR 200       // Mensajes de error por pantalla

// Códigos de exit por error
#define ERR_ENTRADA_ERRONEA 2
#define ERR_SEND 3
#define ERR_RECV 4
#define ERR_FSAL 5

#define NOMBRE_FICH "primos.txt"
#define NOMBRE_FICH_CUENTA "cuentaprimos.txt"
#define CADA_CUANTOS_ESCRIBO 5

// rango de búsqueda, desde BASE a BASE+RANGO
#define BASE 800000000
#define RANGO 2000

// Intervalo del temporizador para RAIZ
#define INTERVALO_TIMER 5

// Códigos de mensaje para el campo mesg_type del tipo T_MESG_BUFFER
#define COD_ESTOY_AQUI 5           // Un calculador indica al SERVER que está preparado
#define COD_LIMITES 4              // Mensaje del SERVER al calculador indicando los límites de operación
#define COD_RESULTADOS 6           // Localizado un primo
#define COD_FIN 7                  // Final del procesamiento de un calculador

// Mensaje que se intercambia

typedef struct {
    long mesg_type;
    char mesg_text[LONGITUD_MSG];
} T_MESG_BUFFER;

int Comprobarsiesprimo(long int numero);
void Informar(char *texto, int verboso); //si verboso=0 no informa de si un número encontrado es primo
void Imprimirjerarquiaproc(int pidraiz,int pidservidor, int *pidhijos, int numhijos);	//imprime PIDs
int ContarLineas(); //cuenta las lineas de un fichero
static void alarmHandler(int signo);
void escribirPrimos(int num);	//función para escribir los números en un fichero

int cuentasegs;                   // Variable para el cómputo del tiempo total

int main(int argc, char* argv[])
{
	int i,j;
	long int numero;
	long int numprimrec;
    long int nbase=BASE;
    int nrango=RANGO;
    int nfin;
    time_t tstart,tend; 
	double cpu_time_used;
	
	key_t key;
    int msgid;    
    int pid, pidservidor, pidraiz, parentpid, mypid, pidcalc;
    int *pidhijos;
    int intervalo,inicuenta;
    int verbosity;
    T_MESG_BUFFER message;
    char info[LONGITUD_MSG_ERR];
    FILE *fsal, *fc;
    int numhijos;

	
    // Control de entrada, después del nombre del script debe figurar el número de hijos y el parámetro verbosity
	
	//correccion de errores del control de entrada
	if(argc<3){
		printf("Error, debes introducir 2 números\n");
	}else{
		numhijos = atoi(argv[1]);  //recibe un char por argv y lo convierte a int
		verbosity = atoi(argv[2]);
		if((verbosity!=0)&&(verbosity!=1)){	//si es distinto de 0 y 1 da error
			printf("El valor de verbosity introducido no es válido\nSe inicializará a 0 por defecto\n");
			verbosity=0;
		}
	}
    
	
	
    pid=fork();       // Creación del SERVER	//pid server
    
    if (pid == 0)     // Rama del hijo de RAIZ (SERVER)
    {
		pid = getpid();
		pidservidor = pid;
		mypid = pidservidor;	   
		
		// Petición de clave para crear la cola
		if ( ( key = ftok( "/tmp", 'C' ) ) == -1 ) {
		  perror( "Fallo al pedir ftok" );
		  exit( 1 );
		}
		
		printf( "Server: System V IPC key = %u\n", key );

        // Creación de la cola de mensajería
		if ( ( msgid = msgget( key, IPC_CREAT | 0666 ) ) == -1 ) {
		  perror( "Fallo al crear la cola de mensajes" );
		  exit( 2 );
		}
		printf("Server: Message queue id = %u\n", msgid );

        i = 0;
        // Creación de los procesos CALCuladores
		while(i < numhijos) {
			if (pid > 0) { // Solo SERVER creará hijos
				pid=fork(); 
				if (pid == 0) {   // Rama hijo
					parentpid = getppid();
					mypid = getpid();
			    }
			}
			i++;  // Número de hijos creados
		}

        // AQUI VA LA LOGICA DE NEGOCIO DE CADA CALCulador. 
		if (mypid != pidservidor) {

			message.mesg_type = COD_ESTOY_AQUI;
			sprintf(message.mesg_text,"%d",mypid);
			msgsnd( msgid, &message, sizeof(message), IPC_NOWAIT);
			
			//A la espera de recibir el mensaje de límites de operación
			msgrcv(msgid, &message, sizeof(message), COD_LIMITES, 0);
			sscanf(message.mesg_text, "%ld %d", &nbase, &nrango);
			
			//buscar num primos en un rango
			for(numero=nbase; numero<nbase+nrango; numero++){
				if((Comprobarsiesprimo(numero))==1){
					
					//crear función para escribir en primos.txt
					escribirPrimos(numero);
					
					//envio num primo
					message.mesg_type = COD_RESULTADOS;
					sprintf(message.mesg_text, "%d %ld", mypid, numero);
					msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);
					
					//LLAMAR A LA FUNCION INFORMAR AQUÍ
					Informar(message.mesg_text, verbosity);
					
				}
			}
			
			//aviso fin de procesamiento
			message.mesg_type = COD_FIN;
			sprintf(message.mesg_text, "%d", mypid);
			msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);			
			
			exit(0);
		}
		
		// SERVER
		
		else{ 
			// Pide memoria dinámica para crear la lista de pids de los hijos CALCuladores
			pidhijos=(int*) malloc(numhijos*sizeof(int));
		  
			//Recepción de los mensajes COD_ESTOY_AQUI de los hijos
			for (j=0; j <numhijos; j++){
				msgrcv(msgid, &message, sizeof(message), 0, 0);
				sscanf(message.mesg_text,"%d",&pidhijos[j]); // Tendrás que guardar esa pid
				printf("\nMe ha enviado un mensaje el hijo %d\n",pidhijos[j]);
			}
		  
			//sleep(60); // Esto es solo para que el esqueleto no muera de inmediato, quitar en el definitivo
		  
			// Mucho código con la lógica de negocio de SERVER
			Imprimirjerarquiaproc(pidraiz, pidservidor, pidhijos, numhijos);
			
			//rango de busqueda
			inicuenta=BASE;
			intervalo=(int)(RANGO/numhijos);
			
			for(int i=0; i<numhijos; i++){
				message.mesg_type=COD_LIMITES;
				sprintf(message.mesg_text, "%d %d", inicuenta, intervalo);
				msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT);
				inicuenta=inicuenta+intervalo;
			}
			
			// Borrar la cola de mensajería, muy importante. No olvides cerrar los ficheros
			msgctl(msgid,IPC_RMID,NULL);
		  
		}
    }

    // Rama de RAIZ, proceso primigenio
    
    else
    {
		cuentasegs=0;
		alarm(INTERVALO_TIMER);
		signal(SIGALRM, alarmHandler);
			wait(NULL);	// Espera del final de SERVER
		printf("RESULTADO: %d primos detectados \n", ContarLineas());
	  
		// El final de todo
    }
	
	//Tiempo total del proceso
	tend = clock();
  	cpu_time_used = ((double) (tend - tstart)) / CLOCKS_PER_SEC;
  	printf("Tiempo %.0f (segundos)",cpu_time_used);
	
}

// Manejador de la alarma en el RAIZ
static void alarmHandler(int signo)
{
	FILE *fc;
	int numPrimo;
	cuentasegs = cuentasegs + INTERVALO_TIMER;
	
	if((fc=fopen(NOMBRE_FICH_CUENTA, "r")) != NULL){
		fscanf(fc, "%d", &numPrimo);
		fclose(fc);
		printf("%02d (segs): %d primos encontrados \n", cuentasegs, numPrimo);
	}else{
		printf("%02d (segs) \n", cuentasegs);
	}
    //printf("SOLO PARA EL ESQUELETO... Han pasado 5 segundos\n");
    alarm(INTERVALO_TIMER);

}

// Encontrar primos
int Comprobarsiesprimo(long int numero) {
	if (numero < 2){
		return 0; // 0 y 1 no son primos
	}else{
		for (int i = 2; i <= (numero / 2) ; i++){
			if (numero % i == 0){
				return 0;
			} 
		}
	}
	
	return 1;
}

//contar lineas de un fichero
int ContarLineas(){
	int contador=0;
	char c;
	FILE *contar = fopen(NOMBRE_FICH, "r");
	
	if(contar==NULL){
		printf("Error al contar lineas de primos.txt\n");
	}else{
		while((c==fgetc(contar)) != EOF){	//mientras sea distinto del End Of File
			if(c=='\n'){	//si es \n significa que acaba una linea
				contador++;
			}
		}
	}
	
	fclose(contar);
	return contador;
	
}

//imprimir PIDs
void Imprimirjerarquiaproc(int pidraiz,int pidservidor, int *pidhijos, int numhijos){
	printf("\nRAIZ \tSERV \tCALC\n");
	printf("%d \t%d \t", pidraiz, pidservidor);
	for(int i=0; i<numhijos; i++){
		printf("%d\n\t \t", pidhijos[i]);	//imprime el pid de todos los hijos
	}
	printf("\n");
}

//informa cuando se encuentra un primo
void Informar(char *texto, int verboso){
	//se llama a la función en la linea 140
	
	//se tiene que imprimir el pid del proceso hijo que encontró un numero primo y el numero
	
	if(verboso==1){
		printf("El hijo %s, encontró que el segundo número es primo\n", texto);
	}else{
		;	//si es 0 no informa
	}
}

void escribirPrimos(int num){
	//printf("Funciono escribir, %d\n", num);
	FILE *escribir = fopen(NOMBRE_FICH, "a");	//a añade datos al final del fichero
	
	if(escribir==NULL){
		perror("Error: fallo al crear primos.txt");
	}else{
		fprintf(escribir, "%d\n", num);
	}
	
	fclose(escribir);
}