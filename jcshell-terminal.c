// author Edson XG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include "Ficheiro.h"




int main(int argc, char **argv){
    system("clear");

	char buffer[80];
    int filed[2],num_pross_exce = 0;
    
    /* ABRIR OS PIPE'S CRIADOS NO JCSHELL */
    filed[0] = open(argv[1], O_WRONLY); // ABRIR ESVREVER
    filed[1]= open("jcshell-out",O_RDONLY); // ABRIRI LER

    sprintf(buffer,"PID: %d",getpid()); // CONVERTER O INT PID EM STRING
    write(filed[0],buffer,strlen(buffer)+1); // ENVIAR O PID DO TERINAL NO JCSHELL

    printf("\n********************* JCSHELL-TERMINAL ********************* = %s = \n",buffer);
    

	/// ITERACAO PARA ESCREVER E LER OS COMANDOS E DADOS NO JCSHELL VIA PIPE
		while(1){
            printf("\n Insert command_$ >:");
            fflush(stdin);
            fgets(buffer,80,stdin); // LEITURA DO COMANDO FGETS PEGA ATE \N

            if(strcmp(buffer,"\n") == 0){

				printf("\n = Comando invalido = \nn ");
                continue;

			}else if(strcmp(buffer,"exit\n") == 0){
                
                sprintf(buffer,"exit %d",getpid());
                if(write(filed[0],buffer,strlen(buffer)+1) == -1){
                   printf(" erro no envio do comando\n"); 
                   continue;
                } //ENVIAR O COMANDO NO JCSHELL
                exit(EXIT_SUCCESS); // TERMINAR O JCSHELL TERMINAL

            }else if(strcmp(buffer,"stats\n") == 0){
                ler_dados(); // BUSCAR NUM_TEMPO_TOTAL NO FICHEIRO
                if(write(filed[0],buffer,strlen(buffer)+1) == -1){
                   printf(" erro no envio do comando\n"); 
                   continue;
                } //ENVIAR O COMANDO NO JCSHELL
                
                read(filed[1],&num_pross_exce,sizeof(int)); //RECEBER NUM PROCESSO EM EXECUCAO
                
                printf("\n************* = PROCESS STATUS = *************\n\n");
                printf("\t\tNum_Pross_exec: [%d] \n\n",num_pross_exce);
                printf("\t\tTotal_time_Process: [%d s]\n\n",t_total);
                printf("\n**********************************************\n\n");

                continue;

            }else{

            
             if(write(filed[0],buffer,strlen(buffer)+1) == -1){ // ENVIAR O COMANDO NO JCSHELL
                        printf(" erro no envio do comando\n");

             }
             
            }

        }
       
return 0;
}