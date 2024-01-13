#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //funcion sleep
#include <ctype.h> //funcion isdigit (lo usamos para la funcion pasar a binario)
#include <time.h> //para la funcion clock (tiempo medio de acceso)
#include <math.h> //para pasar de binario a deciaml (lo usamos para las potencias de 2)
//la libreria math.h no está en linux, asi que se tiene que compilar con -lm al final (gcc MEMsym.c -lm -o memsym)
//https://xbalban.wordpress.com/2014/08/16/no-compila-error-con-libreria-math-h-en-gnulinux-solucion/

#define NUM_FILAS 8
#define MAX_LINE 100
#define TAM_LINEA 16
#define LRAM 4096

typedef struct {
	unsigned char ETQ;
	unsigned char Data[TAM_LINEA];
} T_CACHE_LINE;

void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]);
void VolcarCACHE(T_CACHE_LINE *tbl);
void ParsearDireccion(char binario [],int direccion_cortada []);
void TratarFallo(T_CACHE_LINE *memoria, int *direccion, char MRAM[]);  // en la variable direccion  tenemos guardadas la etiqueta, el bloque, la palabra y la linea
int * PasarABinario(int * direccion_cortada,char MRAM []);
int PasarADecimal(long long n);

int globaltime = 0; //iniciamos la variable globaltime a 0 cada vez que se inicia el programa          variable global
int numfallos = 0; //iniciamos variable de numeor de fallos a 0 cadea vez que se inicie el programa    variable golabl
int accesos=0;  //numero de accesos que se realizan durando la ejecucion del programa
char texto[100];

//inicializar array de Simul_RAM
//char Simul_RAM[LRAM];
char Simul_RAM[LRAM];


int main (int argc, char** argv){
	
	//inicializar array de tipo T_CACHE_LINE 
	T_CACHE_LINE memoria[NUM_FILAS];
	LimpiarCACHE(memoria);
	
	
	
	//Abrirmos los archivos necesarios para el programa de calculo de cache en modo lectura
	FILE *CONTENTS_RAM;
	CONTENTS_RAM = fopen("CONTENTS_RAM.bin", "rb"); //abrir en bianrio para aceeder bit a bit
	
	if(CONTENTS_RAM == NULL){
		printf("Error, archivo CONTENTS_RAM.bin no encontrado\n");  //condicion en la que si el archivo no se encuentra en el directorio acaba el programa con un error
		return -1;
	}else{	//guardar contenido de CONTENTS_RAM en Simul_RAM
				
		fread(Simul_RAM,sizeof(Simul_RAM),1,CONTENTS_RAM);  // Introducimos en el array de Simul_RAM toda la string del archivo CONTENTS_RAM

		//printf("%s", Simul_RAM);			   //Prueba viendo por pantalla de que se guarda correctamente
			
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	FILE *dirs_memoria;
	dirs_memoria = fopen("accesos_memoria.txt", "r");   //Abre en binario numeros en hexadecimal
	int direccion_cortada[4];
	int * direccion;	
	char MRAM[5];
	
	
	if(dirs_memoria == NULL){
		printf("Error, archivo accesos_memoria.txt no encontrado\n");  //condicion en la que si el archivo no se encuentra en el directorio acaba el programa con un error
		return -1;
	}else{
		
		while(fgets(MRAM, sizeof(MRAM), dirs_memoria)!=NULL)
		{
			globaltime++;		
			direccion=PasarABinario(direccion_cortada, MRAM);
			MRAM[strcspn(MRAM, "\n")];  //Calcula el tamaño de MRAM sin el salto de linea "\n" en otras palabras elimina el salto de linea
			
			if((int)memoria[direccion[1]].ETQ==direccion[0])  //direccion[1]==Linea , direccion[0]==etiqueta y direccion[3]==Bloque
			{
				 printf("T: %d, Acierto de CACHE %d, ADDR %s, Label %X, linea %02X, palabra %02X, DATO %02X \n", globaltime, accesos-numfallos+1, MRAM, direccion[0], direccion[1],direccion[2], memoria[direccion[1]].Data[direccion[2]]);
			}else{
				TratarFallo(memoria, direccion, MRAM/*aqui tenemos la etiqueta, la linea y el bloque*/);			
			}
			
			texto[accesos]=memoria[direccion[1]].Data[direccion[2]];		
			accesos++;
			sleep(1);	
			
		}			
	}
	
	//cerrar los archivos
	fclose(CONTENTS_RAM);
	fclose(dirs_memoria);
	
	VolcarCACHE(memoria);
	printf("\nAccesos totales: %d, Fallos: %d, Tiempo medio: %f ms \n",accesos,numfallos,(double)clock()/1000); //mostrar estadisticas de la ejecucion
   printf("Texto leido: %s \n",texto); //Muestra de texto

	return 0;
}

void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]){
	for(int i=0; i<NUM_FILAS; i++){
		tbl->ETQ=0xFF;
		for(int j=0; j<TAM_LINEA; j++){
			tbl[i].Data[j]=(char)0x23;
		}
			
	}
	
	//imprimir todo
	/*for(int i=0; i<NUM_FILAS; i++){
		//printf("%c",tbl->ETQ);
		for(int j=0; j<TAM_LINEA; j++){
			printf("%x", tbl[i].Data[j]);
		}
		printf("\n");
			
	}*/
}

int * PasarABinario(int * direccion_cortada,char MRAM []){
    char binario[100];
    binario[0]='\0'; //borrar contenido string

    for (int i = 0; i < 3; ++i){
        if(isdigit(MRAM[i])){
            switch(MRAM[i]){
                case '0' :
                    strcat(binario,"0000");
                    break;
                case '1' :
                    strcat(binario, "0001");
                    break;
                case '2' :
                    strcat(binario, "0010");
                    break;
                case '3' :
                    strcat(binario, "0011");
                    break;
                case '4' :
                    strcat(binario, "0100");
                    break;
                case '5' :
                    strcat(binario, "0101");
                    break;
                case '6' :
                    strcat(binario, "0110");
                    break;
                case '7' :
                    strcat(binario, "0111");
                    break;
                case '8' :
                    strcat(binario, "1000");
                    break;
                case '9' :
                    strcat(binario, "1001");
                    break;
                default : printf("\nERROR en %c",MRAM[i]);
		    }
        }else{
            switch(MRAM[i]){
                case 'A' :
                    strcat(binario, "1010");
                    break;
                case 'B' :
                    strcat(binario, "1011");
                    break;
                case 'C' :
                    strcat(binario, "1100");
                    break;
                case 'D' :
                    strcat(binario, "1101");
                    break;
                case 'E' :
                    strcat(binario, "1110");
                    break;
                case 'F' :
                    strcat(binario, "1111");
                    break;
                default : printf("\nERROR en %c",MRAM[i]);
		    }
        }              
    }

    //Una vez tenemos la direccion de memoria la parseamos.
    ParsearDireccion(binario,direccion_cortada);

    return direccion_cortada;
}

int PasarADecimal(long long n) {
  int decimal=0;
  int i=0;
  int rem=0;

  while (n!=0) {
    rem = n % 10;
    n= n/10;
    decimal += rem * pow(2, i);
    ++i;
  }

  return decimal;
}

void ParsearDireccion(char binario [],int direccion_cortada []){
	char bloque_separado[150];
   memcpy(bloque_separado,&binario[0],8); //8 bits=bits etiqueta+bits linea
   int bloque_separado2 = atoi(bloque_separado);	//pasa de una cadena a un entero (utilizamos otra variable porque no nos dejaba aplicar la funcion a la variable inicial)
   direccion_cortada[3]=PasarADecimal(bloque_separado2);	//guarda en direccion cortada el bloque pero en decimal (lo mismo en las siguientes variables)
	
	char etiqueta_separada[150];
   memcpy(etiqueta_separada,&binario[0],5);
   int etiqueta_separada2 = atoi(etiqueta_separada);
   direccion_cortada[0]=PasarADecimal(etiqueta_separada2);

   char linea_separada[150];
   memcpy(linea_separada,&binario[5],3);
   int linea_separada2 = atoi(linea_separada);
   direccion_cortada[1]=PasarADecimal(linea_separada2);

   char palabra_separada[150];
   memcpy(palabra_separada,&binario[8],4);
   int palabra_separada2 = atoi(palabra_separada);
   direccion_cortada[2]=PasarADecimal(palabra_separada2);
}

void TratarFallo(T_CACHE_LINE *memoria, int *direccion, char MRAM[]){
	numfallos++;
	globaltime = globaltime + 10;
	
	 printf("T: %d, Fallo de CACHE %d, ADDR %s Label %X linea %02X palabra %02X bloque %02X \n", globaltime, numfallos, MRAM, direccion[0], direccion[1], direccion[2], direccion[3]);
	 printf("Cargando el bloque %02X en la linea %02X \n",direccion[3], direccion[1]);
	 
	memoria[direccion[1]].ETQ=direccion[0];	 //Cargamos la nueva etiqueta 
	
	for(int i = 0; i<TAM_LINEA; i++)  //Actualizar los datos con la extraccion del texto
	{
		memoria[direccion[1]].Data[i] = Simul_RAM[direccion[3] + TAM_LINEA*i];	
	}
}

void VolcarCACHE(T_CACHE_LINE *tbl)
{
	FILE * volcado;
	char tambuffer[128]; //Tamñao de la informacion a meter en el archivo (tamaño en bytes)
	int aux=0;	
	
	
	printf("\n");
    for(int i=0;i<NUM_FILAS;i++){
        printf("ETQ: %02X \t",tbl[i].ETQ);
        printf("Data ");
        for(int j=TAM_LINEA-1;j>=0;j--){ //printea del reves para mostrar correcatmente
            printf(" %x ",tbl[i].Data[j]);
        }
        printf("\n");
    }
    
    volcado = fopen("CONTENTS_CACHE.bin", "w+b"); // sino exixte se crea el archivo en modo escritura y en binario
	
	if (volcado == NULL){
		printf("Error al crear el fichero CONTENTS_CACHE.bin\n");
	}
	else {
		 for(int i=0;i<NUM_FILAS;i++){
            for(int j=0;j<TAM_LINEA;j++){
                tambuffer[aux]=tbl[i].Data[j];  //guardamos la inftomacion de la estructura en una array de su tamaño para luego poder pasarlo al archivo
                aux++;
            }
        }

		for (int i = 0; i < 128; i++){
            fwrite(tambuffer, sizeof(tambuffer),1,volcado);  //pasamos al archivo la informacion del array byte  a byte
        }        
	}
	fclose(volcado);
}