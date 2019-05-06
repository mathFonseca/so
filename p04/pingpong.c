/*
Equipe: Amanda S. e Matheus F.
Sistemas Operacionais 2019/1
LAB 04.
*/
#include "pingpong.h"
#include "datatypes.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define STACKSIZE 32768		/* tamanho de pilha das threads */


int contador_tarefa;			// Para gerar os IDs das tarefas.
task_t *PingPongMain;			// A main do Ping Pong tem que ser uma tarefa também.
task_t *taskAtual;			// Para saber qual é a tarefa sendo executada no momento.
task_t* despachante;			// Para ter sempre uma referência para o despachante.
task_t* fila_prontas;			// Fila de tarefas prontas
task_t* fila_suspensas;		// Fila de tarefas suspensas.
int alfa;				// Constante de envelhecimento.

void dispatcher_body();		// Declaração da função do despachante.

task_t *escalonador()			// Escalonador por Prioridades.
{
	task_t *prioritaria = fila_prontas;		// Iniciamos com a head sendo a prioritária
	task_t *aux_comp = fila_prontas->next;	// Percorremos do elemento seguinte até o fim da fila (circular)
	while(aux_comp != fila_prontas)
	{
		if(prioritaria->prio_dim >= aux_comp->prio_dim)	// Se aux_comp for mais prioritária
		{
			if(prioritaria->prio_dim > -20)			// Envelhecemos a prioritaria
			{
				prioritaria->prio_dim = prioritaria->prio_dim + alfa;
			}
			prioritaria = aux_comp;			// Atualizamos a tarefa mais prioritária encontrada até então
		}
		else
		{
			if(aux_comp->prio_dim > -20)			// Envelhecemos a aux_comp
			{									aux_comp->prio_dim = aux_comp->prio_dim + alfa;
			}
		}
		aux_comp = aux_comp->next;			// Movemos para o próximo elemento da fila
	}
	task_t *tarefa_escolhida = (task_t*)queue_remove((queue_t**)&fila_prontas, (queue_t*)prioritaria);
	return tarefa_escolhida;	// retorna a task mais prioritária

}

void pingpong_init()			// Inicializa as estruturas internas do Ping Pong OS.
{
	setvbuf(stdout, 0, _IONBF, 0); 		// desativa o buffer da saida padrao (stdout), usado pela função printf */
	PingPongMain = malloc(sizeof(task_t));  	// Inicializamos a Ping Pong Main com a estrutura de tarefas
	PingPongMain->tid = 0;                         	// O ID da Ping Pong Main será 0, o primeiro de todos.
	taskAtual = PingPongMain;			// Nossa tarefa atual passa a ser nossa main.
	contador_tarefa = 1;				// As próximas tarefas terão ID de 1 em diante.
	alfa = -1;					// Seta a constante de envelhecimento para -1.

	despachante = malloc(sizeof(task_t));		// Cria o despachante.
	task_create(despachante, dispatcher_body, "");	// Cria a tarefa do despachante
	fila_prontas = NULL;					// Inicializa a fila de tarefas prontas vazia
	fila_suspensas = NULL;				// Inicializa a fila de tarefas suspensas vazia

	#ifdef DEBUG
		printf("PingPong iniciado com sucesso.\n");
	#endif
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
	getcontext(&(task->context));					//Pega o contexto da tarefa/task recebida (no parametro;)

	// Seção igual ao context.c
	// Com adição da linha que cria a pilha
	char* stack;								// Cria a pilha.
	stack = malloc (STACKSIZE);						// Aloca a pilha
	if (stack)
	{
		task->context.uc_stack.ss_sp = stack ;			// Ponteiro para a pilha.
		task->context.uc_stack.ss_size = STACKSIZE;		// Tamanho da pilha. Definido nas variáveis globais.
		task->context.uc_stack.ss_flags = 0;				// Recebe flags = 0 (não ativa nenhuma das flags)
		task->context.uc_link = 0;					// Para quando realizar troca de contextos.
	}
	else
	{
		printf("Erro ao inicializar a pilha.");
		return (-1);							// Retorna valor negativo para erros.
	}

	makecontext(&(task->context), (void*) (*start_func), 1, arg);	// Associamos a função corpo a tarefa ao nosso descritor.
	task->tid = contador_tarefa;						// O ID é igual o da contagem de tarefas atuais.
	contador_tarefa++;							// Atualiza o ID. (Nunca vai repetir um número).
	if(task != despachante)						// Se a tarefa criada não for o despachante, põe ela na fila.
	{
		queue_append((queue_t**) &fila_prontas, (queue_t*) task);
		task->status = pronta;
		task->prio_dim = 0;		// Seta a prioridade dinamica para default (0)
		task->prio_est = 0;		// Seta a prioridade estatica para default (0)
	}

	#ifdef DEBUG
		printf("Tarefa %d criada com sucesso. \n", task->tid);
	#endif

	return(task->tid);
}

void task_exit (int exitCode)	// Termina a tarefa corrente, indicando um valor de status encerramento
{
	taskAtual->status = terminada;						// Atualiza o estado da tarefa atual para terminada.
	#ifdef DEBUG									// Imprime a mensagem de depuração para a tarefa atual.
		printf("Iniciando o encerramento da tarefa %d \n", taskAtual->tid);
		printf("Status de Encerramento: %d \n", exitCode);			// Imprime o status de encerramento.
	#endif
	// Retorna para a função Despachante se for uma tarefa qualquer
	// Ou retorna para main do Ping Pong se for o Despachante
	if( taskAtual == despachante)
	{
		// Se estamos no despachante, é porque não teve nenhuma tarefa mais no escalonador.
		task_switch(PingPongMain);		// Retornamos para a main.
	}
	else
	{
		task_switch(despachante);		//  Caso contrário, mudamos para o despachante.
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
	task->status = executando;		// Atualizamos o estado da tarefa que vai entrar para executando
	// Nossa "TaskAtual" vai mudar para a tarefa que iremos entrar.
	// Por isso já se altera a informação antes da troca de contexto.
	task_t* aux;
	aux = taskAtual;
	taskAtual = task;
	swapcontext(&(aux->context),&(taskAtual->context));	// Realizamos a troca de contexto
	return 0;
}

int task_id ()	// retorna o identificador da tarefa corrente (main eh 0)
{
	#ifdef DEBUG
		printf("Identificador da tarefa corrente: %d \n", taskAtual->tid);
	#endif
	return taskAtual->tid;
}

void dispatcher_body()	// Função do despachante
{
	while( queue_size((queue_t*) fila_prontas) > 0)
	{
		task_t *tarefa_escolhida = escalonador();	// Pega a tarefa do escalonador
		if(tarefa_escolhida)
		{
			// Ações antes de lançar a próxima tarefa.
			task_switch(tarefa_escolhida);
			// Ações depois de lançar a próxima tarefa.
		}
	}
	task_exit(0);
}

void task_yield()	//Retirar a execũção da tarefa atual e por ela na fila de prontas novamente.
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

void task_suspend(task_t* task, task_t** fila_suspensas)	// Suspende a tarefa atual, colocando-a na fila de suspensas.
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

void task_resume(task_t* task)	// Acorda uma tarefa da fila de suspensas, colocando-a na fila de tarefas prontas.
{
	task_t* tarefa_acordada;
	// Remover da fila de tarefas suspensas
	tarefa_acordada = (task_t*)queue_remove((queue_t**)&fila_suspensas,(queue_t*)task);
	// Adiciona na fila de tarefas prontas
	queue_append((queue_t**)&fila_prontas,(queue_t*)tarefa_acordada);
	// Atualiza o estado
	tarefa_acordada->status = pronta;
}

void task_setprio(task_t* task, int prio)	// Seta uma prioridade para task
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

int task_getprio(task_t* task)		// Retorna a prioridade dinamica da task
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
