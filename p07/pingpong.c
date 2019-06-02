/*
Equipe:
Amanda S. RA: 1658921
Matheus F. RA: 1794027
Sistemas Operacionais 2019/1
LAB 07.
*/
#include "pingpong.h"
#include "datatypes.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && \
(!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). \
A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

//Definição de variáveis globais.
//#define DEBUG
#define STACKSIZE 32768	// Tamanho de pilha das threads
struct itimerval timer;	// Estrutura do timer REAL
struct itimerval virtual;	// Estrutura do timer VIRTUAL (para pausas)
struct sigaction action ;	// Estrutura do tratador de sinais


int contador_tarefa;	// Para gerar os IDs das tarefas.
task_t *PingPongMain;	// A main do Ping Pong tem que ser uma tarefa também.
task_t *taskAtual;	// Para saber qual é a tarefa sendo executada no momento.
task_t* despachante;	// Para ter sempre uma referência para o despachante.
task_t* fila_prontas;	// Fila de tarefas prontas
task_t* fila_suspensas;	// Fila de tarefas suspensas.
int alfa;	// Constante de envelhecimento.
int quantum;	// Define o quantum geral do sistema.
int total_time;	// Relógio universal do sistema.

void dispatcher_body();	// Declaração da função do despachante.
void decrem_ticks();	// Declaração da função que contabiliza o quantum
void timer_inicio();	// Declaração da função que inicializa o timer do sistema.
void timer_arm();	// Declaração da função que "arma" o timer.

// Escalonador por Prioridades.

task_t *escalonador()
{
	task_t *prioritaria = fila_prontas;	// Iniciamos com a head sendo a\
	 prioritária
	task_t *aux_comp = fila_prontas->next;	// Percorremos do elemento seguinte\
	 até o fim da fila (circular)
	while(aux_comp != fila_prontas)
	{
		if(prioritaria->prio_dim >= aux_comp->prio_dim)	// Se aux_comp for mais\
		 prioritária
		{
			if(prioritaria->prio_dim > -20)	// Envelhecemos a prioritaria
			{
				prioritaria->prio_dim = prioritaria->prio_dim + alfa;
			}
			prioritaria = aux_comp;	// Atualizamos a tarefa mais prioritária\
			 encontrada até então
		}
		else
		{
			if(aux_comp->prio_dim > -20)	// Envelhecemos a aux_comp
			{
				aux_comp->prio_dim = aux_comp->prio_dim + alfa;
			}
		}
		aux_comp = aux_comp->next;	// Movemos para o próximo elemento da fila.
	}
	task_t *tarefa_escolhida = (task_t*)queue_remove((queue_t**)&fila_prontas,\
	 (queue_t*)prioritaria);
	return tarefa_escolhida;	// retorna a task mais prioritária

}

void pingpong_init()	// Inicializa as estruturas internas do Ping Pong OS.
{
	setvbuf(stdout, 0, _IONBF, 0);	// Desativa o buffer da saida padrao (stdout),\
	 usado pela função printf
	PingPongMain = malloc(sizeof(task_t));	// Inicializamos a Ping Pong Main com a\
	 estrutura de tarefas
	PingPongMain->tid = 0;	// O ID da Ping Pong Main será 0, o primeiro de todos.
	taskAtual = PingPongMain;	// Nossa tarefa atual passa a ser nossa main.
	contador_tarefa = 1;	// As próximas tarefas terão ID de 1 em diante.
	alfa = -1;	// Seta a constante de envelhecimento para -1.
	quantum = 20;
	timer_inicio();
	despachante = malloc(sizeof(task_t));	// Cria o despachante.
	task_create(despachante, dispatcher_body, "");	// Cria a tarefa do despachante
	fila_prontas = NULL;	// Inicializa a fila de tarefas prontas vazia
	fila_suspensas = NULL;	// Inicializa a fila de tarefas suspensas vazia

	#ifdef DEBUG
		printf("PingPong iniciado com sucesso.\n");
	#endif
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
	getcontext(&(task->context));	//Pega o contexto da tarefa/task recebida\
	 (no parametro;)

	// Seção igual ao context.c
	// Com adição da linha que cria a pilha
	char* stack;	// Cria a pilha.
	stack = malloc (STACKSIZE);	// Aloca a pilha
	if (stack)
	{
		task->context.uc_stack.ss_sp = stack ;	// Ponteiro para a pilha.
		task->context.uc_stack.ss_size = STACKSIZE;	// Tamanho da pilha, definido\
		 nas variáveis globais.
		task->context.uc_stack.ss_flags = 0;	// Recebe flags = 0 (não ativa\
			 nenhuma das flags)
		task->context.uc_link = 0;	// Para quando realizar troca de contextos.
	}
	else
	{
		printf("Erro ao inicializar a pilha.");
		return (-1);	// Retorna valor negativo para erros.
	}

	makecontext(&(task->context), (void*) (*start_func), 1, arg);	// Associamos\
	 a função corpo a tarefa ao nosso descritor.
	task->tid = contador_tarefa;	// O ID é igual o da contagem de tarefas atuais.
	contador_tarefa++;	// Atualiza o ID. (Nunca vai repetir um número).
	if(task != despachante)	// Se a tarefa criada não for o despachante, põe\
	 ela na fila.
	{
		queue_append((queue_t**) &fila_prontas, (queue_t*) task);
		task->status = pronta;
		task->prio_dim = 0;	// Seta a prioridade dinamica para default (0)
		task->prio_est = 0;	// Seta a prioridade estatica para default (0)
	}

	// Inicializa as variáveis em relação aos tempos.
	task->num_ativ = 0;
	task->ticks_final = total_time;

	#ifdef DEBUG
		printf("Tarefa %d criada com sucesso. \n", task->tid);
	#endif

	return(task->tid);
}

void task_exit (int exitCode)	// Termina a tarefa corrente, indicando um\
 valor de status encerramento
{
	taskAtual->status = terminada;	// Atualiza o estado da tarefa atual para\
	 terminada.
	#ifdef DEBUG	// Imprime a mensagem de depuração para a tarefa atual.
		printf("Iniciando o encerramento da tarefa %d \n", taskAtual->tid);
		printf("Status de Encerramento: %d \n", exitCode);	// Imprime o status de\
		 encerramento.
	#endif
	// Atualiza variáveis de tempo
	// 1 - Tempo de Execução
	// É o tempo que levou desde que ela foi criada até o tempo atual
	taskAtual->tempo_exec = total_time - taskAtual->ticks_inicial;
	// 2 - Tempo de Processamento
	/* É uma soma contínua do tempo que ele ficou executando até receber um
	switch. Ele pega o tempo que estava quando ele executou pela última vez e
	subtrai do tempo do sistema. Aqui, ele soma essa última etapa até o fim
	da sua execução.*/
	taskAtual->tempo_proc += total_time - taskAtual->ticks_final;
	// Imprime informações de tempo.
	printf("Task %d exit: Execution time %dms. Processor time: %dms, %d \
		Activations. \n", taskAtual->tid, taskAtual->tempo_exec, \
		taskAtual->tempo_proc, taskAtual->num_ativ);

	// Retorna para a função Despachante se for uma tarefa qualquer
	// Ou retorna para main do Ping Pong se for o Despachante
	if( taskAtual == despachante)
	{
		// Se estamos no despachante, é porque não teve nenhuma tarefa mais no \
			escalonador.
		task_switch(PingPongMain);	// Retornamos para a main.
	}
	else
	{
		task_switch(despachante);	//  Caso contrário, mudamos para o despachante.
	}
}

// Alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
	// A task recebida pelo parâmetro é a tarefa na qual iremos entrar.
	// Precisamos saber se a tarefa não é nula antes de trocar de contexto
	if(task == NULL)
	{
		printf("Erro durante a troca de tarefas. A tarefa não existe. (NULL)\n");
		return (-1);
	}

	#ifdef DEBUG
		printf("Iniciando a troca de contexto %d para %d. \n", taskAtual->tid,\
		 	task->tid);
	#endif
	task->status = executando;	// Atualizamos o estado da tarefa que vai entrar\
	 	para executando
	// Atualizamos as variáveis de tempo.
	// 1 - Tempo de processamento.
	// É uma soma contínua do tempo que ele ficou executando até receber um\
	 	switch. Ele pega o tempo que estava quando ele executou pela última vez\
		e subtrai do tempo do sistema.
	taskAtual->tempo_proc += total_time - taskAtual->ticks_final;
	// 2 - Número de ativações.
	// Incrementa a cada switch de entrada.
	task->num_ativ++;
	// 3 - Ticks da última execução (ticks_final)
	// É o exato tempo do sistema quando esse switch foi executado.
	task->ticks_final = total_time;

	// Nossa "TaskAtual" vai mudar para a tarefa que iremos entrar.
	// Por isso já se altera a informação antes da troca de contexto.
	task_t* aux;
	aux = taskAtual;
	taskAtual = task;
	swapcontext(&(aux->context),&(taskAtual->context));	// Realizamos a troca de\
	 contexto
	return 0;
}

// Retorna o identificador da tarefa corrente (main eh 0)

int task_id ()
{
	#ifdef DEBUG
		printf("Identificador da tarefa corrente: %d \n", taskAtual->tid);
	#endif
	return taskAtual->tid;
}

// Função do despachante

void dispatcher_body()
{
	while( queue_size((queue_t*) fila_prontas) > 0)
	{
		task_t *tarefa_escolhida = escalonador();	// Pega a tarefa do escalonador
		if(tarefa_escolhida)
		{
			timer_arm();
			task_switch(tarefa_escolhida);
			// Ações depois de lançar a próxima tarefa.
		}
	}
	task_exit(0);
}

// Retira a execũção da tarefa atual e por ela na fila de prontas novamente.

void task_yield()
{
	// Checamos a tarefa atual. Se não for a main, prossegue.
	if(taskAtual != PingPongMain)
	{
		// Se a tarefa não for a Main, coloca a task atual no fim da fila de\
		 tarefas prontas. Atualizamos o estado para prontas.
		queue_append( (queue_t**)&fila_prontas, (queue_t*)taskAtual);
		taskAtual->status = pronta;
	}
	// Deve trocar da tarefa atual para a tarefa Dispatcher.
	task_switch(despachante);
}

// Suspende a tarefa atual, colocando-a na fila de suspensas.

void task_suspend(task_t* task, task_t** fila_suspensas)
{
	task_t* tarefa_suspensa;
	if(fila_suspensas !=  NULL)
	{
		// Se a tarefa for nulo, consideramos a task atual.
		if(task == NULL)
		{
			// Não "removemos" ela da fila normal, pois se ela é a atual, e já foi\
			 	removida pelo escalonador.
			tarefa_suspensa = taskAtual;
			// Penduramos ela na fila de suspensas.
			queue_append( (queue_t**)&fila_suspensas, (queue_t*)tarefa_suspensa);
			// Atualizamos o estado.
			tarefa_suspensa->status = suspensa;
		}
		else
		{
			// Remove Task da fila de prontas e põe ela em tarefa_suspensa
			tarefa_suspensa = (task_t*)queue_remove((queue_t**)&fila_prontas,\
				(queue_t*)task);
			// Penduramos na fila de suspensas.
			queue_append( (queue_t**)&fila_suspensas, (queue_t*)tarefa_suspensa);
			// Atualizamos o estado.
			tarefa_suspensa->status = suspensa;
		}
		// Se fila_suspensas for nula, não faz a retirada.
	}
}

// Acorda uma tarefa da fila de suspensas, colocando-a na fila de tarefas prontas.

void task_resume(task_t* task)
{
	task_t* tarefa_acordada;
	// Remover da fila de tarefas suspensas
	tarefa_acordada = (task_t*)queue_remove((queue_t**)&fila_suspensas,\
		(queue_t*)task);
	// Adiciona na fila de tarefas prontas
	queue_append((queue_t**)&fila_prontas,(queue_t*)tarefa_acordada);
	// Atualiza o estado
	tarefa_acordada->status = pronta;
}

// Seta uma prioridade para task

void task_setprio(task_t* task, int prio)
{
	if(task == NULL)	// Seta a prioridade para a task atual
	{
		taskAtual->prio_est = prio;
	}
	else
	{
		task->prio_dim = prio;
		task->prio_est = prio;
	}
}

// Retorna a prioridade dinamica da task

int task_getprio(task_t* task)
{
	int prio;
	if(task == NULL)	// Retorna a prioridade da task atual
	{
		prio = taskAtual->prio_est;
	}
	else
	{
		prio = task->prio_est;
	}
	return prio;
}

// Inicializa o temporizador do sistema

void timer_inicio().
{
	// Registra a a��o para o sinal de timer SIGALRM
	action.sa_handler = decrem_ticks;
	sigemptyset (&action.sa_mask) ;
	action.sa_flags = 0 ;
	if (sigaction (SIGALRM, &action, 0) < 0)
	{
		perror ("Erro em sigaction: ") ;
		exit (1) ;
	}

	// ajusta valores do temporizador REAL
	timer.it_value.tv_usec = 1000;	// Primeiro disparo, em micro-segundos.\
	 	1000 micro = 1 mili
	timer.it_value.tv_sec  = 0;	// Primeiro disparo, em segundos. Não dispara em\
	 	"segundos"
	timer.it_interval.tv_usec = 1000;	// Disparos subsequentes, em\
	 	micro-segundos, 1000 micro = 1 mili
	timer.it_interval.tv_sec  = 0;	// Disparos subsequentes, em segundos

}

// Decrementa quantidade de ticks de uma tarefa.

void decrem_ticks()
{
	total_time++;	// Cada 1 aqui é um milisegundo.
	taskAtual->quantum--;	// Decrementa 1 tick da tarefa atual
	if(taskAtual->quantum <= 0 /*&& taskAtual->tipo == usuario*/)	// Se os\
	 	quantuns acabaram e o a tarefa é do tipo de usuario, retorna para a fila.
	{
		taskAtual->quantum = quantum;	// Recarrega quantum da tarefa
		task_yield();	// Retorna
	}
}

// Arma o temporizador ITIMER_REAL

void timer_arm()
{
	if(setitimer(ITIMER_REAL, &timer, 0) <0)
	{
		perror("Erro em setitimer: ");
		exit(1);
	}
}

unsigned int systime()
{
	return total_time;	// Retorna o tempo total do sistema.
}
