/*
Equipe:
Amanda S. RA: 1658921
Matheus F. RA: 1794027
Sistemas Operacionais 2019/1
LAB 05.
*/
#include "pingpong.h"
#include "datatypes.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

//Definição de variáveis globais.
//#define DEBUG
#define STACKSIZE 32768		/* tamanho de pilha das threads */
struct itimerval timer;			// Estrutura do timer REAL
struct itimerval virtual;		// Estrutura do timer VIRTUAL (para pausas)
struct sigaction action ;		// Estrutura do tratador de sinais


int contador_tarefa;       // Para gerar os IDs das tarefas.
task_t *PingPongMain;   // A main do Ping Pong tem que ser uma tarefa também.
task_t *taskAtual;         // Para saber qual é a tarefa sendo executada no momento.
task_t* despachante;    // Para ter sempre uma referência para o despachante.
task_t* fila_prontas;
task_t* fila_suspensas;
int quantum;		// Define o quantum geral do sistema.

void dispatcher_body();
void decrem_ticks();
void timer_inicio();
void timer_arm();

// Escalonador do Ping Pong OS.
// Inicialmente funciona com a política FCFS (FIFO) para uma fila tarefas PRONTAS.
task_t *escalonador()
{
	// Removo o elemento da fila, e retorno ele pro despachante.
	task_t* tarefa_escolhida;
	tarefa_escolhida = (task_t*)queue_remove ( (queue_t**) &fila_prontas, (queue_t*) fila_prontas);

	return tarefa_escolhida;
}

// ******************************
//Inicializa as estruturas internas do Ping Pong OS.
void pingpong_init()
{
	setvbuf(stdout, 0, _IONBF, 0); 	// Desativa o buffer da saida padrao (stdout), usado pela função printf */
	PingPongMain = malloc(sizeof(task_t));  // Inicializamos a Ping Pong Main com a estrutura de tarefas
	PingPongMain->tid = 0;                         // O ID da Ping Pong Main será 0, o primeiro de todos.
	PingPongMain->tipo = usuario;
	taskAtual = PingPongMain;                    // Nossa tarefa atual passa a ser nossa main.
	contador_tarefa = 1;                             // As próximas tarefas terão ID de 1 em diante.

	quantum = 20;				// Define um quantum geral de 20 ticks
	timer_inicio();
	// Cria o despachante.
	despachante = malloc(sizeof(task_t));
	// Cria a tarefa do despachante
	task_create(despachante, dispatcher_body, "");
	despachante->tipo = sistema;
	// Cria a fila de tarefas prontas vazia (inicialmente)
	fila_prontas = NULL;
	fila_suspensas = NULL;

	#ifdef DEBUG
		printf("PingPong iniciado com sucesso.\n");
	#endif
}


int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
	getcontext(&(task->context));          //Pega o contexto da tarefa/task recebida (no parametro;)

	// Seção igual ao context.c
	// Com adição da linha que cria a pilha
	char* stack;                                               // Cria a pilha.
	stack = malloc (STACKSIZE);                      // Aloca a pilha
	if (stack)
	{
		task->context.uc_stack.ss_sp = stack ;                // Ponteiro para a pilha.
		task->context.uc_stack.ss_size = STACKSIZE;     // Tamanho da pilha. Definido nas variáveis globais.
		task->context.uc_stack.ss_flags = 0;                    // Recebe flags = 0 (não ativa nenhuma das flags)
		task->context.uc_link = 0;                                     // Para quando realizar troca de contextos.
	}
	else
	{
		printf("Erro ao inicializar a pilha.");
		return (-1); // Retorna valor negativo para erros.
	}
	// Agora que as informações foram devidamente preenchidas, associamos a função corpo a tarefa ao nosso descritor.
	// Coloquei paranteses em volta do void pois o compilador dizia que tinha few arguments.
	makecontext(&(task->context), (void*) (*start_func), 1, arg);

	// Por último, preenchemos as informações restantes do descritor da tarefa
	// o ID é igual a contagem de tarefas atuais.
	// Mesmo se futuramente a tarefa for concluída, a contagem não vai diminuir. Assim garantimos que nenhuma tarefa vai receber o mesmo ID.
	task->tid = contador_tarefa;
	contador_tarefa++;


	// Checamos se não é o próprio despachante que foi criado.
	if(task != despachante)
	{
		// Se não for, entra numa fila de tarefas
		queue_append((queue_t**) &fila_prontas, (queue_t*) task);
		// Atualiza o status dela para "pronta"
		task->status = pronta;

	}
	// Se for, não coloca o despachante na fila.

	//  Imprime a mensagem de depuração.
	#ifdef DEBUG
		printf("Tarefa %d criada com sucesso. \n", task->tid);
	#endif
	return(task->tid);
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
	// Atualiza o estado da tarefa atual para terminada.
	taskAtual->status = terminada;
	// Imprime a mensagem de depuração para a tarefa atual.
	#ifdef DEBUG
		printf("Iniciando o encerramento da tarefa %d \n", taskAtual->tid);
		printf("Status de Encerramento: %d \n", exitCode);
	#endif
	// Imprime o status de encerramento.
	// Retorna para a função Despachante se for uma tarefa qualquer
	// Ou retorna para main do Ping Pong se for o Despachante
	if( taskAtual == despachante)
	{
		// Se estamos no despachante, é porque não teve nenhuma tarefa mais no escalonador.
		// Retornamos para a main.
		task_switch(PingPongMain);
	}
	else
	{
	//  Caso contrário, mudamos para o despachante.
	task_switch(despachante);
	}
}

// alterna a execução para a tarefa indicada
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
		printf("Iniciando a troca de contexto %d para %d. \n", taskAtual->tid, task->tid);
	#endif
	// Atualizamos o estado (status) da tarefa que vai sair.
	// Se o status for terminada, não faz nada. Se for diferente, mandamos para a fila de prontas, no estado "pronta"
	if(taskAtual->status != terminada)
	{
		taskAtual->status = pronta;
	}
	// Atualizamos o estado da tarefa que vai entrar para executando
	task->status = executando;
	// Nossa "TaskAtual" vai mudar para a tarefa que iremos entrar.
	// Por isso já se altera a informação antes da troca de contexto.
	task_t* aux;
	aux = taskAtual;
	taskAtual = task;
	// Realizamos a troca de contexto
	swapcontext(&(aux->context),&(taskAtual->context));
	return 0;
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id ()
{
	// Como a tarefa atual está salvo em TaskAtual, retornamos o id dela.
	#ifdef DEBUG
		printf("Identificador da tarefa corrente: %d \n", taskAtual->tid);
	#endif
	return taskAtual->tid;
}

// Função despachante.
// É usado para ser uma interface entre as trocas de contexto das tarefas
void dispatcher_body()
{
	while( queue_size((queue_t*) fila_prontas) > 0)
	{
		// Pega a tarefa do escalonador
		task_t *tarefa_escolhida = escalonador();
		if(tarefa_escolhida)
		{
			timer_arm();	// Inicia o relógio antes de entrar na Tarefa Escolhida
			task_switch(tarefa_escolhida);
			// Ações depois de lançar a próxima tarefa.
		}
	}
	task_exit(0);
}

// Retirar a execũção da tarefa atual e por ela na fila de prontas novamente.
void task_yield()
{
	// Checamos a tarefa atual. Se não for a main, prossegue.
	if(taskAtual != PingPongMain)
	{
		// Se a tarefa não for a Main, coloca a task atual no fim da fila de tarefas prontas. Atualizamos o estado para prontas.
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
			// Não "removemos" ela da fila normal, pois se ela é a atual, e já foi removida pelo escalonador.
			tarefa_suspensa = taskAtual;
			// Penduramos ela na fila de suspensas.
			queue_append( (queue_t**)&fila_suspensas, (queue_t*)tarefa_suspensa);
			// Atualizamos o estado.
			tarefa_suspensa->status = suspensa;
		}
		else
		{
			// Remove Task da fila de prontas e põe ela em tarefa_suspensa
			tarefa_suspensa = (task_t*)queue_remove((queue_t**)&fila_prontas,(queue_t*)task);
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
	// Verifica se pertence a alguma fila
	//if()
	//{
		// Remover da fila de tarefas suspensas
		tarefa_acordada = (task_t*)queue_remove((queue_t**)&fila_suspensas,(queue_t*)task);
	//}
	// Adiciona na fila de tarefas prontas
	queue_append((queue_t**)&fila_prontas,(queue_t*)tarefa_acordada);
	// Atualiza o estado
	tarefa_acordada->status = pronta;
}

void timer_inicio()	// Inicializa o temporizador do sistema.
{
	// registra a a��o para o sinal de timer SIGALRM
	action.sa_handler = decrem_ticks;
	sigemptyset (&action.sa_mask) ;
	action.sa_flags = 0 ;
	if (sigaction (SIGALRM, &action, 0) < 0)
	{
		perror ("Erro em sigaction: ") ;
		exit (1) ;
	}

	// ajusta valores do temporizador REAL
	timer.it_value.tv_usec = 1000 ;	// primeiro disparo, em micro-segundos. 1000 micro = 1 mili
	timer.it_value.tv_sec  = 0 ;		// primeiro disparo, em segundos. Não dispara em "segundos"
	timer.it_interval.tv_usec = 1000 ;	// disparos subsequentes, em micro-segundos. 1000 micro = 1 mili
	timer.it_interval.tv_sec  = 0 ;		// disparos subsequentes, em segundos

}

void decrem_ticks()	// Decrementa quantidade de ticks de uma tarefa.
{
	taskAtual->quantum--;		// Decrementa 1 tick da tarefa atual
	if(taskAtual->quantum <= 0 /*&& taskAtual->tipo == usuario*/)	// Se os quantuns acabaram e o a tarefa é do tipo de usuario, retorna para a fila.
	{
		taskAtual->quantum = quantum;	// Recarrega quantum da tarefa
		task_yield();				// Retorna
	}
}

// * * * * * * * * * * *
void timer_arm()	// Arma o temporizador ITIMER_REAL
{
	if(setitimer(ITIMER_REAL, &timer, 0) <0)
	{
		perror("Erro em setitimer: ");
		exit(1);
	}
}
