// author Edson XG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "list.h"
#include "Ficheiro.h"
#include "commandlinereader.h"

#define MAXPAR 20 //quantidade maxima de processo em execucao

/// Prototipos da :
void *tarefa_monitora();// funcao que monitora os processos filhos
void imprimir_dados(list_t *list, int pid); // funcao que escreve os dados do ps_filho no ficheiro log.txt
void ler_dados(); //funcao que le os dados do ps_filho no ficheiro log.txt," iteracao e tempo total"
void terminal_kill_abruta(); //funcao para matar os processo por meio do sinal SIGINT




int num_filho = 0,finalizar = 0; 
pthread_t tid; // tid e a id da tarefa monitora
pthread_mutex_t mutex; // mutex para control de acesso das zonas critica
pthread_cond_t cond, cond_1; // variaveis de condicao para limitar a qtd de ps_filho em execucao


//lista de dados  dos processos filhos
list_t *list;
//lista dos pid's dos jcshell - terminal
list_pid *list_p;


///......................... FUNCAO PRINCIPAL DO JC-SHELL .....................................
int main(int argc, char **argv){
	system("clear");
	printf("\n\t\t = JC-SHELL EM EXECUCAO...  =\n");
	

	// funcao para capturar sinal ou evento no jcshell do tipo SIGINT
	signal(SIGINT,terminal_kill_abruta);

	char *args[7],buffer[100],fileps[50];
	int filed_p[2];

	/// inicializacao do mutex e das variaveis de condicao
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_cond_init(&cond_1, NULL);

	/* initialize list */
  	list = lst_new();
	list_p = lst_new_pid();  
	


	  /* ler dados guardados */
		ler_dados();

	/* criacao de pipe's  filheicros especias para comunicacao entre processos */
	   mkfifo("jcshell-in",0777);// para entrada de dados do jcshell-terminal para jscshell
	   mkfifo("jcshell-out",0777); //para saida de dados do jcshell para jscshell-terminal
 
 // abertura dos pipes's
	   filed_p[0]= open("jcshell-in",O_RDONLY); // para leitura
	   filed_p[1]= open("jcshell-out",O_WRONLY); // para escrita
	
	if(pthread_create(&tid,NULL,tarefa_monitora,NULL) != 0){ // criar mult-tarefa para func tarefa moonitoras
		printf("ERROR to CREATE PTHREAD");
		exit(EXIT_FAILURE);
	}else{
	
		while(1){

				strcpy(buffer," ");
				if(read(filed_p[0],buffer,sizeof(buffer)) == -1){ // leitura do comandado no pipe
					 printf(" erro na leitura do comando\n");
					  sleep(1);
					 continue;	
				}
     			if(readLineArguments(args,7, buffer) == -1){ // repartir o comando lido em substring para o ARGS
					 sleep(1);
					 continue;
				 }
				
	                                                                                                            
			 if(strcmp(args[0],"PID:") == 0){
				
				insert_new_pid(list_p, args[1]); // pegar pid dos jcshel-terminal
			
				continue;
			}else if(strcmp(args[0],"stats") == 0){
				
				write(filed_p[1],&num_filho,sizeof(num_filho)); // escrever num_processo e no stats jcshell-terminal
				
				continue;
			}else if(strcmp(args[0],"exit") == 0) {

				remover_pid(list_p, args[1]);
				printf("\n\t = JCSHELL-TERMINAL CLOSED PID: %s =\n",args[1]);

				continue;
			}else{
			                                                                                                                   
				if(strcmp(args[0],"exit-global") == 0 ){ // ver se comando for exit global para terminar

					
	
					
						finalizar = 1; // para terminal a tarefa monitora

						pthread_cond_signal(&cond_1); // dar sinal na var de cond para deixar de esperar
						pthread_join(tid,NULL); // esperar ate tarefa monitorar terminar

						terminal_kill(list_p); // mata dos processo jcshell terminal
						

						lst_destroy(list);// destruir a lista de dados dos ps_filho
						lst_pid_destroy(list_p); // destruir a lista de pid
						
							// destruir a mutex e as var de cond
						pthread_mutex_destroy(&mutex);
						pthread_cond_destroy(&cond);
						pthread_cond_destroy(&cond_1);

						
						 // fechar os pipe;s
						close(filed_p[0]);
						close(filed_p[1]);
						
						//  apagar os ficheiros pipes
						unlink("jcshell-in");
						unlink("jcshell-out");

						printf("\n\t\t= JC-SHELL TERMINADO =\n");
						exit(EXIT_SUCCESS); /// *******TERMINAR O JCSHELL*******
				 
		    	}else{
		    		//............................ ZONA CRITICA DO var de cond ESPERAR.............................................................
					pthread_mutex_lock(&mutex);	
					while(num_filho == MAXPAR) pthread_cond_wait(&cond, &mutex); 
					pthread_mutex_unlock(&mutex);
					//............................ ........ZONA CRITICA .............................................................
					int pid, filed;
					pid = fork(); // CRIAR UM NOVO PROCESSO
					
					if(pid == 0){ // SE PID == 0 BLOCO DO PROCESSO FILHO
						
						sprintf(fileps,"jcshell-out-%d.txt",getpid()); // CRIAR NOME FILE PARA STDOUT CAT COM SEU PID
						filed = open(fileps,O_CREAT | O_RDWR,S_IWUSR | S_IRUSR);// ABRIR O FICHEIRO PARA ESCREVER
						dup2(filed,STDOUT_FILENO); // DUPLICAR O STDOUT PARA O FICHEIRO 

						if(execv(args[0], args) == -1){
						printf("Process pid <%d> Programa inexistente\n\n",getpid());
						close(filed); // FECHAR O FICHEIRO
						
						exit(EXIT_FAILURE); // TERMINAL PS_FILHO SE PROGRAMA NAO EXISTE
						
						
						}

					}else if(pid == -1){  // ERRO NA CRIACAO DE UM NOVO PROCESSO

							printf(" erro no fork \n");
							exit(EXIT_FAILURE);
						
						  }else if(pid > 0){ // SE PID > 0 BLOCO DO PROCESSO PAI
							  
//............................ ZONA CRITICA DO PROCESSO PAI.............................................................
								pthread_mutex_lock(&mutex);	
								insert_new_process(list, pid , time(NULL)); 
								++num_filho;
								pthread_cond_signal(&cond_1);
								pthread_mutex_unlock(&mutex);
									
//............................fim zona critica..........................................................................
		
								continue;
							}
					}
				}	
			}
    
		}
	return 0;
}



///......................... FUNCAO TAREFA MONITORA .....................................
void *tarefa_monitora(){
	int pid_filho, status;

 while(1){
//............................ ZONA CRITICA ESPERA ATIVA.............................................................

	pthread_mutex_lock(&mutex);
	 if(num_filho == 0){
		 if(finalizar == 1){
			pthread_mutex_unlock(&mutex); 
			pthread_exit(NULL); // TERMINAR TAREFA MONITORA
		}
	 			while(num_filho == 0 && finalizar == 0)pthread_cond_wait(&cond_1, &mutex); // DORMIR SE NUM_FILHO == 0
				pthread_mutex_unlock(&mutex);
		
		continue;
	}else{		
	pthread_mutex_unlock(&mutex);
	//............................  .............................................................


				pid_filho = wait(&status); // ESPERA PS FILHO TERMINA

//............................ ZONA CRITICA DA TAREFA MONITORA.............................................................
			pthread_mutex_lock(&mutex);
				sleep(1);
				update_terminated_process(list, pid_filho, time(NULL)); // ATUALIZAR DADOS DO PD_FILHO NA LISTA
				
				imprimir_dados(list, pid_filho); // GUARADAR OS DADOS NO FICHEIRO LOG.TXT 
				lst_print(list,pid_filho); // MOSTRAR ALGUMAS INFO NA TELA
			
				--num_filho;
			pthread_cond_signal(&cond); // ASSINALAR PARA DEIXAR DE ESPERA
			pthread_mutex_unlock(&mutex);
			
			
			
			continue;
//............................fim zona critica..........................................................................
		}
		
	
		}
	}
	///*************** funcao para matar os processo por meio do sinal SIGINT ****************
void terminal_kill_abruta(){
	lst_pid *item;

	item = list_p->first;
	while (item != NULL){
		kill(item->pid,SIGKILL);
		item = item->next;
	}
	
	printf("\n\t= TERMINACAO ABRUTA DOS PROCESSOS=\n");
}

