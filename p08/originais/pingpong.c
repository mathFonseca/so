/*
Equipe:
Amanda S. RA: 1658921
Matheus F. RA: 1794027
Sistemas Operacionais 2019/1
LAB 08.
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
#define QUANTUM 20
#define ALFA 1

struct itimerval timer;	// Estrutura do timer REAL
struct itimerval virtual;	// Estrutura do timer VIRTUAL (para pausas)
struct sigaction action;	// Estrutura do tratador de sinais

int task_count;	// Para gerar os IDs das tarefas.
int total_sys_time;	// Relógio universal do sistema.

task_t *PingPongMain;	// A main do Ping Pong tem que ser uma tarefa também.
task_t *current_Task;	// Para saber qual é a tarefa sendo executada no momento.
task_t* dispatcher;	// Para ter sempre uma referência para o dispatcher.
task_t* ready_queue;	// Fila de tarefas prontas
task_t* suspend_queue;	// Fila de tarefas suspensas.

void dispatcher_body();	// Declaração da função do dispatcher.
void decrem_ticks();	// Declaração da função que contabiliza o quantum
void timer_init();	// Declaração da função que inicializa o timer do sistema.
void timer_arm();	// Declaração da função que "arma" o timer.

// scheduler por Prioridades.

task_t *scheduler()
{
	task_t *aux; // Auxiliar
	task_t *priority; // Tarefa prioritária
	task_t *current = current_Task; // Tarefa atual
	int value; // Valor da prioridade

	if(current_Task != dispatcher)
	{
		// Se a tarefa atual não for o dispatcher
		// (o dispatcher não pode entrar em filas)
		value = task_getprio(current_Task);
		// Value passa a ser o valor da prioridade da tarefa atual
		aux = current_Task;
		// A auxiliar se torna a tarefa atual.
	}
	else
	{
		// Se a tarefa atual for o dispatcher
		// Coloca o valor minímo possível (já que o dispatcher não tem prioridade)
		value = 21;
		// A auxiliar passa a ser a primeira tarefa da fila de prontas.
		aux = ready_queue;
	}

	// Atualizamos a tarefa prioritária para ser a Auxiliar
	// (a primeira da fila ou a atual)
	priority = aux;
	// Tarefa auxiliar passa a ser a primeira da fila de prontas
	// (para o caso da auxiliar ser a tarefa atual)
	aux = ready_queue;

	// Checamos se a prioridade da auxiliar é menor que a prioridade
	// de priority.
	if(task_getprio(aux) < value)
	{
		 // Se tiverem duas tarefas com mesma prioridade, dá prioridade à primeira que encontrou ou à atual
		value = task_getprio(aux);
		priority = aux;
	}
	aux = aux->next;

	// Percorrer a fila de tarefas prontas e verificar qual tem a maior prioridade
	while(aux != ready_queue)
	{
		if(task_getprio(aux) < value)
		{ // Se tiverem duas tarefas com mesma prioridade, dá prioridade à primeira que encontrou ou à atual
			value = task_getprio(aux);
			priority = aux;
		}
		aux = aux->next;
	}

	// Auxiliar se torna a próxima tarefa da fila de prontas
	// depois da tarefa mais prioritária
	aux = priority->next;

	// Envelhece todas as outras tarefas da fila
	while(aux != priority)
	{
		// Envelhece a prioridade dinamica em ALFA
		aux->prio_dim = aux->prio_dim - ALFA;
		// Move para pŕoxima tarefa.
		aux = aux->next;
	}

	// Reseta a prioridade estatica da tarefa escolhida
	task_setprio(priority, priority->prio_est);
	// Remove a tarefa mais prioritária da fila
	queue_remove ((queue_t**) &ready_queue, (queue_t *)priority);

	return priority;
}

void pingpong_init()	// Inicializa as estruturas internas do Ping Pong OS.
{
	setvbuf(stdout, 0, _IONBF, 0);	// Desativa o buffer da saida padrao (stdout),\
	 usado pela função printf
	PingPongMain = malloc(sizeof(task_t));	// Inicializamos a Ping Pong Main com a\
	 estrutura de tarefas
	PingPongMain->task_id = 0;	// O ID da Ping Pong Main será 0, o primeiro de todos.

	PingPongMain->prio_dim = 0;	// Prioridade Dinamica
    PingPongMain->prio_est = 0;	// Prioridade Estatica
	PingPongMain->quantum = QUANTUM;	// Quantum restante de uma tarefa.

	PingPongMain->status = pronta;
	PingPongMain->time_exec = 0;	// Tempo total de execução.
    PingPongMain->time_proc = 0;	// Tempo total dentro do processador.
    PingPongMain->n_ativ = 0;	// Número de ativações da tarefa até concluir.
    PingPongMain->created_time = total_sys_time;	// Quantos ticks tinha o\
		sistema quando a tarefa executou pela primeira vez.
	PingPongMain->task_wait_id = -1; // A princípio, a Main não espera por ninguém\
	para executar.
	current_Task = PingPongMain;	// Nossa tarefa atual passa a ser nossa main.
	task_count = 1;	// As próximas tarefas terão ID de 1 em diante.

	timer_init();
	dispatcher = malloc(sizeof(task_t));	// Cria o dispatcher.
	task_create(dispatcher, dispatcher_body, "");	// Cria a tarefa do dispatcher
	ready_queue = NULL;	// Inicializa a fila de tarefas prontas vazia
	suspend_queue = NULL;	// Inicializa a fila de tarefas suspensas vazia

	task_yield();	// Coloca a Main na fila.

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
	task->task_id = task_count;	// O ID é igual o da contagem de tarefas atuais.
	task_count++;	// Atualiza o ID. (Nunca vai repetir um número).
	if(task != dispatcher)	// Se a tarefa criada não for o dispatcher, põe\
	 ela na fila.
	{
		queue_append((queue_t**) &ready_queue, (queue_t*) task);
		task->status = pronta;
		task->prio_dim = 0;	// Seta a prioridade dinamica para default (0)
		task->prio_est = 0;	// Seta a prioridade estatica para default (0)
		task->quantum = QUANTUM;	// Seta o quantum da tarefa pro definido;
	}

	// Inicializa as variáveis em relação aos tempos.
	task->n_ativ = 0;
	task->final_time = total_sys_time;

	task->task_wait_id = -1; // A princípio, uma tarefa criada não depende de\
	ninguém. Assim sendo, colocamos -1.
	#ifdef DEBUG
		printf("Tarefa %d criada com sucesso. \n", task->task_id);
	#endif

	return(task->task_id);
}

void task_exit (int exitCode)	// Termina a tarefa corrente, indicando um\
 valor de status encerramento
{
	current_Task->status = terminada;	// Atualiza o estado da tarefa atual para\
	 terminada.
	current_Task->exitCode = exitCode; // Recebe o exit Code e armazena. É utilizado\
	no caso de tarefas que estão esperando através do task_join.
	#ifdef DEBUG	// Imprime a mensagem de depuração para a tarefa atual.
		printf("Iniciando o encerramento da tarefa %d \n", current_Task->task_id);
		printf("Status de Encerramento: %d \n", exitCode);	// Imprime o status de\
		 encerramento.
	#endif
	// Atualiza variáveis de tempo
	// É o tempo que levou desde que ela foi criada até o tempo atual
	current_Task->time_exec = total_sys_time - current_Task->created_time;
	/* É uma soma contínua do tempo que ele ficou executando até receber um
	switch. Ele pega o tempo que estava quando ele executou pela última vez e
	subtrai do tempo do sistema. Aqui, ele soma essa última etapa até o fim
	da sua execução.*/
	current_Task->time_proc += total_sys_time - current_Task->final_time;
	// Imprime informações de tempo.
	printf("Task %d exit: Execution time %dms. CPU time: %dms, %d Activations. \n",\
	current_Task->task_id, current_Task->time_exec, \
		current_Task->time_proc, current_Task->n_ativ);

	if(suspend_queue != NULL)	// Antes de encerrar, verificamos se existem\
	tarefas que estão esperando pela Tarefa Atual
	{
		task_t * aux = suspend_queue; // Auxiliar para percorrer a filas

		if(aux->task_wait_id == current_Task->task_id) // Caso especial onde a
		{
			task_resume(aux);
		}
		aux = aux->next;

		while(aux != suspend_queue && suspend_queue != NULL && aux != NULL)
		{
			// Percorre a fila buscando as tarefas que estão esperando a atual acabar.
			if(aux->task_wait_id == current_Task->task_id) // Se a tarefa estiver esperando
			{
				task_resume(aux); // Retira a tarefa da fila de suspensas.
			}
			aux = aux->next; // pula pra pŕoxima tarefa da fila
		}
	}
	// Retorna para o despachante.
	task_switch(dispatcher);
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

	task->status = executando;	// Atualizamos o estado da tarefa que vai entrar\
	 	para executando
	// Atualizamos as variáveis de tempo.
	// É uma soma contínua do tempo que ele ficou executando até receber um\
	 	switch. Ele pega o tempo que estava quando ele executou pela última vez\
		e subtrai do tempo do sistema.
	current_Task->time_proc += total_sys_time - current_Task->final_time;
	// Incrementa a cada switch de entrada.
	task->n_ativ++;
	// É o exato tempo do sistema quando esse switch foi executado.
	task->final_time = total_sys_time;

	// Nossa "current_Task" vai mudar para a tarefa que iremos entrar.
	// Por isso já se altera a informação antes da troca de contexto.
	task_t* aux;
	aux = current_Task;
	current_Task = task;

	if(current_Task != dispatcher)	// Pois o dispatcher não tem quantum.
	{
		current_Task->quantum = QUANTUM;	// Recarrega o quantum da tarefa.
	}

	#ifdef DEBUG
		printf("Iniciando a troca de contexto %d para %d. \n", current_Task->task_id,\
		 	task->task_id);
	#endif

	swapcontext(&(aux->context),&(current_Task->context));	// Realizamos a troca de\
	 contexto
	return 0;
}

// Retorna o identificador da tarefa corrente (main eh 0)

int task_id ()
{
	#ifdef DEBUG
		printf("Identificador da tarefa corrente: %d \n", current_Task->task_id);
	#endif
	return current_Task->task_id;
}

// Função do dispatcher

void dispatcher_body()
{
	while( queue_size((queue_t*) ready_queue) > 0)
	{
		task_t *tarefa_escolhida = scheduler();	// Pega a tarefa do scheduler
		if(tarefa_escolhida)
		{
			timer_arm();
			task_switch(tarefa_escolhida);
		}
	}
	task_exit(0);
}

// Retira a execũção da tarefa atual e por ela na fila de prontas novamente.

void task_yield()
{
	// Checamos se a tarefa atual não é o dispatcher e se ela pode ir pra filas\
	de prontas.
	if(current_Task != dispatcher && current_Task->task_wait_id == -1)
	{
		// Se a tarefa não foro dispatcher, coloca a task atual no fim da fila de\
		 tarefas prontas. Atualizamos o estado para prontas.
		queue_append( (queue_t**)&ready_queue, (queue_t*)current_Task);
		current_Task->status = pronta;
	}
	// Deve trocar da tarefa atual para a tarefa Dispatcher.
	task_switch(dispatcher);
}

// Suspende a tarefa atual, colocando-a na fila de suspensas.

void task_suspend(task_t* task)
{
	task_t* suspend;

	// Checamos se a tarefa recebida é nula ou não
	if(task == NULL)
	{
		// Se não for nula, ela será a tarefa a ser suspensa.
		suspend = current_Task;
	}
	else
	{
		// Se for nulo, suspendemos a tarefa atual no lugar.
		suspend = task;
	}

	// Atualiza as flags e demais valores.
	suspend->status = suspensa;
	suspend->quantum = QUANTUM;	// Recarrega o quantum da tarefa\
	 suspensa.

	if(suspend != current_Task)
	{
		// Se não for a tarefa atual, primeiro retiramos ela da fila de prontas.
		queue_remove ((queue_t**) &ready_queue, (queue_t *)suspend);
	}
	// Adicionamos a tarefa suspensa na fila de suspensas.

	queue_append( (queue_t**) &suspend_queue , (queue_t*)suspend);
}

// Acorda uma tarefa da fila de suspensas, colocando-a na fila de tarefas prontas.

void task_resume(task_t* task)
{
	// Remover da fila de tarefas suspensas
	queue_remove((queue_t**)&suspend_queue, (queue_t*)task);
	// Adiciona na fila de tarefas prontas
	queue_append((queue_t**)&ready_queue,(queue_t*)task);
	// Atualiza o estado
	task->task_wait_id = -1; // Depois de ser acordada, ela não espera\
	por mais ninguém.
	task->status = pronta;
}

// Seta uma prioridade para task

void task_setprio(task_t* task, int prio)
{
	if(task == NULL)	// Seta a prioridade para a task atual
	{
		current_Task->prio_est = prio;
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
		prio = current_Task->prio_est;
	}
	else
	{
		prio = task->prio_est;
	}
	return prio;
}

// Inicializa o temporizador do sistema

void timer_init()
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

// Decrementa quantask_idade de ticks de uma tarefa.

void decrem_ticks()
{
	total_sys_time++;	// Cada 1 aqui é um milisegundo.
	current_Task->quantum--;	// Decrementa 1 tick da tarefa atual
	if(current_Task->quantum <= 0 /*&& current_Task->tipo == usuario*/)	// Se os\
	 	quantuns acabaram e o a tarefa é do tipo de usuario, retorna para a fila.
	{
		current_Task->quantum = QUANTUM;	// Recarrega quantum da tarefa
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
	return total_sys_time;	// Retorna o tempo total do sistema.
}

int task_join (task_t *task)
{
	if(task == NULL)
	{
		// No caso da tarefa não existir, retorna -1
		return -1;
	}
	current_Task->task_wait_id = task->task_id; // A tarefa atual precisa esperar a "task"\
	terminar.
	task_suspend(current_Task);
	return task->exitCode;
}
