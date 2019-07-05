/*
Equipe:
Amanda S. RA: 1658921
Matheus F. RA: 1794027
Sistemas Operacionais 2019/1
LAB 08.
*/

// *********** Bibliotecas **********
#include <stdio.h>
#include "datatypes.h"	// Inclusão das estruturas de dados necessárias
#include "pingpong.h"	// Declarações das funções.
#include "queue.h"	// Biblioteca de filas
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

// *********** Variaveis Globais e Defines **********
//#define DEBUG	// Utilizada para ativar mensagens de DEBUG na execução\
do código.
#define STACKSIZE 32768
#define PAUSE 60
#define TASK_QUANTUM 20
#define ALFA 1
struct sigaction action ;	// Variável que define a ação a cada interrupção\
do timer
struct itimerval timer;	// Variável do timer
struct itimerval pause;	// Variável da pausa
int task_counter;	// Variável contadora de tarefas. Serve para gerar ID\
para as tarefas no momento da criação.
task_t* ping_pong_main_task;	// Tarefa que representa a main.
task_t* current_task;	// Tarefa que está sendo executada no momento.
task_t* dispatcher;	// Ponteiro para o despachante.
int total_sys_time;	// Conta o tempo total do "sistema". Aqui, sistema se\
refere ao Sistema Operacional.
// Filas
task_t* ready_queue;	// Ponteiro para fila de tarefas prontas.
task_t* suspended_queue; // Ponteiro para a fila de tarefas suspensas.
task_t* sleeping_queue; // Ponteiro para a fila de tarefas dormindo.
// *********** Declarações de Funções **********
// Funções Gerais
task_t*  task_scheduler();
void dispatcher_body(void* arg);
void pingpong_init();
int task_create (task_t *task, void (*start_func)(void *), void *arg);
int task_switch (task_t *task);
int task_id ();
void task_exit (int exitCode);
void task_suspend (task_t *task);
void task_resume (task_t *task);
void task_yield ();
void task_setprio (task_t *task, int prio);
int task_getprio (task_t *task);
// Funções de Timer
void timer_init();
void pause_timer();
void resume_timer();
void decrease_quantum();

// *********** Implementação das Funções **********
// *********** Funções referentes ao timer **********

// Função: Decrease Quantum.
// Tratador de sinal do timer. Ativado toda vez que o timer geral um sinal\
Ou, em outras palavras, ativado toda vez que um quantum acaba.
// Ele decrementa o quantum da tarefa, e verifica se a mesma tem\
quantidade de quantum suficiente para continuar executando, ou se deve\
voltar para a fila de prontas.
void decrease_quantum()
{
	pause_timer(); // Só para garantir.
	(current_task->remanining_quantum)--;	// Retiramos um quantum da tarefa\
	em execução.
	total_sys_time++;	// Atualizamos o contador de tempo do sistema.
	// Verificamos se a tarefa atual ainda tem quantum para continuar\
	executando, ao mesmo tempo que verificamos se ela não é o dispatcher.
    if(current_task->remanining_quantum <= 0 && current_task != dispatcher)
	{
		// Recarrega o quantum da tarefa de volta para o valor definido.
		current_task->remanining_quantum = TASK_QUANTUM;
		// Coloca a tarefa de volta na fila de prontas.
        task_yield();
	}
	// reativa o timer.
    resume_timer();
}

// Função: Timer Init
// Ajustas os valores do temporizador. Feita baseada no LAB 05.
void timer_init()
{
	// registra a ação para o sinal de timer SIGALRM
	action.sa_handler = decrease_quantum;
	sigemptyset (&action.sa_mask);
	action.sa_flags = 0;

	if (sigaction (SIGALRM, &action, 0) < 0)
	{
	    perror ("Erro em sigaction: ");
		exit (1) ;
	}

	// ajusta valores do temporizador
	timer.it_value.tv_sec=0;	// Primeiro disparo, em segundos
	timer.it_value.tv_usec=1000;	// Primeiro disparo, em microssegundos
	timer.it_interval.tv_sec=0;	// Disparos subsequentes, em segundos
	timer.it_interval.tv_usec=1000;	// Disparos subsequentes, em\
	microssegundos
	pause.it_value.tv_usec=0;	// Primeiro disparo, em microssegundos
	pause.it_value.tv_sec=PAUSE;	// Primeiro disparo, em segundos
	pause.it_interval.tv_usec=0;	// Disparos subsequentes, em\
	microssegundos
	pause.it_interval.tv_sec=PAUSE;	// Disparos subsequentes, em segundos

	// arma o temporizador ITIMER_REAL (vide man setitimer)
	if (setitimer (ITIMER_REAL, &timer, 0) < 0)
	{
	    perror ("Erro em setitimer: ");
		exit (1);
	}

	total_sys_time = 0;
	pause_timer();
}

// Função: Pause Timer
// Arma o temporizador para parar o tempo. (Amanda, se ler isso aqui), pede\
ajua pra Koda, pq eu mesmo não sei.
void pause_timer()
{
	if (setitimer(ITIMER_VIRTUAL, &pause, 0) < 0)
	{
      	perror ("Erro em setitimer: ");
        exit (1);
	}
}

// Função: Resume Timer
// Re-ajustas os valores do temporizador.
// Responsável por fazer o timer voltar a contar.
void resume_timer()
{
	if (setitimer(ITIMER_REAL, &timer, 0) < 0)
	{
		perror ("Erro em setitimer: ");
		exit (1);
   	}
}

// Função: systime.
// Retorna o tempo atual do sistema.
unsigned int systime ()
{
	return total_sys_time;
}

// *********** Funções referentes ao sistema operacional **********

// Função: task_scheduler
// Escalonador do Sistema Operacional.
// Responsável por decidir qual tarefa deve ser escolhida da fila de\
tarefas prontas.
task_t* task_scheduler()
{
	pause_timer();	// Pausamos o timer, para que não haja preempção durante\
	a execução do escalonador.
   	task_t *task_aux;	// Tarefa auxiliar para manipulação de ponteiros.
	task_t *high_priority_task;	// Tarefa mais prioritária

	// Supostamente, esse IF_ELSE é desnecessário. Afinal, o escalonador só\
	é chamado através do despachante. Mas mantive mesmo assim, \
	só para garantir.
	// Checamos se a tarefa atual não é despachante.
	// O despachante não pode ser escalonado.
	if(current_task != dispatcher && current_task->sleeping_time < total_sys_time)
	{
		// Se não for o despachante, então deve ser uma tarefa qualquer.
		// Utilizamos a tarefa atual como a, até então, mais prioritária.
		task_aux = current_task;
	}
	else
	{
		// Caso seja o despachante, então devemos pegar uma tarefa da fila de\
		prontas. No caso, pegamos a primeira (a cabeça da fila).
		// Tarefa auxiliar se torna a primeira tarefa da fila de prontas.
		task_aux = ready_queue;
	}

	// Atualizamos a tarefa prioritária para ser a tarefa que a taxk_auxiliar\
	recebeu. Em seguinda, usamos a task_auxiliar para ser a cabeça da fila.
	high_priority_task = task_aux;
	task_aux = ready_queue;

	// A fila é circular, mas antes de percorrer, checamos a cabeça da fila.
	if(task_getprio(task_aux) < high_priority_task->dinamic_priority\
	 && current_task->sleeping_time < total_sys_time)
	{
		// Se encontrar uma tarefa com mais prioridade, atualiza o valor do\
		ponteiro. Lembrando que prioridade negativa é mais prioritário que\
		as prioridades positivas.
		high_priority_task = task_aux;
	}
	task_aux = task_aux->next;

	// Percorrer a fila de tarefas prontas e verificar qual tem a maior\
	prioridade. Só percorremos a fila até voltarmos para a cabeça novamente\
	e com a certeza que tanto a fila quanto nossa tarefa auxilizar não estão\
	vazias.
	while(task_aux != NULL && ready_queue!= NULL && task_aux != ready_queue\
	&& task_aux->sleeping_time < total_sys_time)
	{
		if(task_getprio(task_aux) < high_priority_task->dinamic_priority)
		{
			// Se encontrar uma tarefa com mais prioridade, atualiza o valor do\
			ponteiro. Lembrando que prioridade negativa é mais prioritário que\
			as prioridades positivas.
			high_priority_task = task_aux;
		}
		task_aux = task_aux->next;
	}

	// Após o IF e o WHILE, high_priority_task é a nossa tarefa mais\
	prioritária.
	// Agora, antes de encerrar o escalonador, envelhecemos todas as demais\
	tarefas (é um escalonador de prioridades com preempção por tempo).
	task_aux = high_priority_task->prev;

	// Envelhece todas as outras tarefas da fila de prontas.
	// Percorremos a fila a partir da tarefa mais prioritária encontrada.
	while(task_aux != high_priority_task && ready_queue != NULL)
	{
		// Como as tarefas mais prioritárias são as negativas, realizamos\
		um decréssimo de ALFA, definido globalmente.
		task_aux->dinamic_priority -= ALFA;
		task_aux = task_aux->prev;
	}

	// Remove a tarefa da fila de prontas.
	high_priority_task = (task_t*) queue_remove ((queue_t**) &ready_queue,\
	(queue_t *)high_priority_task);

	// Reseta a prioridade da tarefa escolhida de volta para a estática.
	task_setprio(high_priority_task, high_priority_task->estatic_priority);

	return high_priority_task;
}

// Função: dispatcher_body
// Despachante do Sistema Operacional.
// É chamado todo fim de quantum de alguma tarefa, ou quando a mesma terminou\
de ser executada.
void dispatcher_body (void* arg)
{
	//pause_timer(); // Pausamos o timer
	// Primeiro, verificamos se existem tarefas dormindo que já podem acordar
	task_t* task_aux = sleeping_queue;
	if(sleeping_queue != NULL)
	{
		// Se a fila de tarefas dormindo não está vazia, percorremos para\
		acordar todas as tarefas QUE JÁ PODEM ACORDAR.
		if(task_aux->sleeping_time <= total_sys_time)
		{
			// Remove da fila de adormecidas, e adiciona na fila de tarefas\
			prontas.
			queue_remove((queue_t**)&sleeping_queue,(queue_t*)task_aux);
			queue_append((queue_t**)&ready_queue,(queue_t*)task_aux);
		}
		task_aux = task_aux->next;
		// Percorremos o resto da fila
		while(task_aux != sleeping_queue && sleeping_queue != NULL &&\
			 task_aux != NULL)
		{
			if(task_aux->sleeping_time <= total_sys_time)
			{
				queue_remove((queue_t**)&sleeping_queue, (queue_t*)task_aux);
				queue_append((queue_t**)&ready_queue,(queue_t*)task_aux);
			}
			task_aux = task_aux->next;
		}
	}
	// Procedimento normal daqui para baixo.
    while(queue_size((queue_t*) ready_queue) > 0 ||\
		 queue_size((queue_t*)sleeping_queue) > 0)
	{
		task_aux = sleeping_queue;
		//printf("chegou aqui");
		while(ready_queue == NULL && sleeping_queue != NULL)
		{
			//printf("almost certain that this while doesn't need to exist");
			if(task_aux->sleeping_time <= total_sys_time)
			{
				queue_remove((queue_t**)&sleeping_queue, (queue_t*)task_aux);
				queue_append((queue_t**)&ready_queue,(queue_t*)task_aux);
			}
			task_aux = task_aux->next;
		}
    	task_t* next_task = task_scheduler(); // next_task recebe o resultado\
		da tarefa escolhida pelo escalonador.
		task_switch(next_task);	// Após o retorno, realizamos o switch para\
		iniciar a execução dessa tarefa.
    }

    task_exit(0); // Se eventualmente não houver tarefas na fila de prontas\
	retorna zero.
}


// Função: PingPong Init
// A função inicializadora do SO.
// Cria a tarefa ping pong main, inicializa as filas, o despachante e o timer.
void pingpong_init ()
{
	setvbuf (stdout, 0, _IONBF, 0) ; 		// Desativa o buffer da saida\
	padrão (stdout), usado pela função printf
	// Processo de Inicialização (igual no task_create)
	ping_pong_main_task = malloc(sizeof(task_t));	// Inicializamos a\
	 Ping Pong Main Task com a estrutura de tarefas
	ping_pong_main_task->task_id = 0;	// ID da main = 0

	ping_pong_main_task->estatic_priority = 0;
	ping_pong_main_task->dinamic_priority = 0;
	ping_pong_main_task->remanining_quantum = TASK_QUANTUM;

	ping_pong_main_task->task_status = ready;
	ping_pong_main_task->task_type = system_task;

	ping_pong_main_task->number_of_activations = 0;
	ping_pong_main_task->creation_time = total_sys_time;

	ping_pong_main_task->task_father = -1;
	ping_pong_main_task->sleeping_time = 0; // não precisa dormir.

	current_task = ping_pong_main_task;	// A Ping Pong Main Task vai ser nossa\
	primeira tarefa do sistema, por isso iniciará como a atual.
	task_counter = 1; // Inicializa o contador do id de tarefas. Como a Ping\
	Pont Main Task é a 0, todas as outras são de 1 em diante.

	// Cria o despachante.
   	dispatcher = malloc(sizeof(task_t));
   	task_create(dispatcher, dispatcher_body, "");
	dispatcher->task_type = system_task;

	ready_queue = NULL;	// Inicializa a fila de tarefas prontas.
	suspended_queue = NULL;	// Inicializa a fila de tarefas suspensas.
	sleeping_queue = NULL; // Inicializa a fila de tarefas dormindo.

	// Uma vez feito tudo que deveria ser feito, inicializamos o timer.
	timer_init();
	task_yield(); // Coloca a tarefa main na fila.

	#ifdef DEBUG
		printf("PingPong iniciado com sucesso.\n");
	#endif

}

// Função: Task Create
// A função que cria tarefas,
// Inicializa a estrutura das tarefas, e as coloca na fila de prontas.
int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
	pause_timer();
	getcontext(&(task->context)); //Pega o contexto da tarefa/task recebida\
	 (no parametro;)

	// Estrutura de acordo com o LAB 01.
	char *stack; // Cria a pilha
	stack = malloc (STACKSIZE);	// Aloca a pilha
	if (stack)
	{
		task->context.uc_stack.ss_sp = stack;	// Ponteiro para a pilha.
		task->context.uc_stack.ss_size = STACKSIZE;	// Tamanho da pilha\
		definido pela variável global.
		task->context.uc_stack.ss_flags = 0;	// Recebe as flags. 0 não\
		ativa nenhuma delas.
		task->context.uc_link = 0;	// Usado em troca de contextos.
	}
	else
	{
		printf("Erro ao inicializar a pilha \n");
		return (-1);	// // Retorna valor negativo para erros.
	}

	makecontext (&(task->context), (void*) *start_func, 1, arg);// Associamos\
	 a função corpo a tarefa ao nosso descritor.
	task->task_id = task_counter; // O ID é igual o da contagem de tarefas\
	atuais.
	task_counter++;	// Atualiza o ID. (Nunca vai repetir um número).

	if(task != dispatcher)
	{
		queue_append((queue_t**) &ready_queue,(queue_t*) task);
		task->task_status = ready;
		task_setprio (task, 0);// Seta a prioridade estatica e dinamica\
		 para default (0)
		task->remanining_quantum = TASK_QUANTUM; // Recarrega o quantum da\
		 tarefa.
		task->task_type = user_task; // Como a tarefa não é a main e nem o\
		despachante, ela é uma tarefa de usuário.
	}
	//  Inicializa as variáveis em relação ao timer.
	task->number_of_activations = 0; // Ela acabou de ser criada, não foi\
	ativada nenhuma vez.
	task->creation_time = total_sys_time; // O tempo de criação dela é o tempo\
	do sistema.
	task->sleeping_time = 0;	// não precisa dormir.

	// A tarefa inicialmente não é dependente de nenhuma outra tarefa. Para\
	isso setamos a variável de join como -1, um código que não representa\
	nenhuma tarefa.
	task->task_father = -1;
	#ifdef DEBUG
		printf("Tarefa %d  criada com sucesso.\n", task->task_id);
	#endif

	return (task->task_id);
}


// Função: Task switch
// A função que troca de tarefas,
// Coloca a tarefa atual em execução na fila, e troca para a tarefa recebida\
no parametro..
int task_switch (task_t *task)
{
	pause_timer();

	// A task recebida pelo parâmetro é a tarefa na qual iremos entrar.
	// Precisamos saber se a tarefa não é nula antes de trocar de contexto
    if(task == NULL)
	{
		printf("Erro durante a troca de tarefas. A tarefa não existe. (NULL)\
		\n");
        return (-1);
   	}

	// Verificamos o estado da tarefa (status).
	if(current_task->task_status != finished &&\
	   current_task->task_status != suspended)
	{
		// Verificamos primeiro se a tarefa que está saindo (a atual), era\
		uma tarefa em execução. Se ela fosse encerrada ou suspensa, não\
		alteramos seu valor.
		current_task->task_status = ready;
	}
	// Modifica a tarefa que vai entrar em execução para o status de execução;
	task->task_status = executing;

	// Atualiza os tempos e a quantidade de ativações da tarefa atual.
	current_task->cpu_time += total_sys_time - current_task->last_call;
	task->last_call = total_sys_time;
	task->number_of_activations++;

	// Como vamos perder a referencia da tarefa atual, usamos um auxiliar.
	task_t* task_aux;
	task_aux = current_task;
	current_task = task;

	// Verificamos se a tarefa atual não é o despachante.
	// Se não for, recarregamos o seu quantum.
	if(current_task != dispatcher)
	{
		current_task->remanining_quantum = TASK_QUANTUM;
		resume_timer();
		// Ativa o timer antes da troca de contexto.
	}

	#ifdef DEBUG
		printf("Troca de contexto %d -> %d feita com sucesso \n",\
		 	   current_task->context, task->context);
	#endif

	swapcontext(&(task_aux->context),&(current_task->context));
	// Realizamos a troca de contexto
	return 0;
}

// Função: Task ID
// Retorna o ID da task atual.
int task_id ()
{
	#ifdef DEBUG
		printf("Identificador da tarefa corrente: %d", current_task->task_id);
	#endif

	return current_task->task_id;
}


// Função: Task Exit
// Encerra a execução da tarefa atual.
// Verificamos se a tarefa atual tem tarefas esperando por\
 ela (na fila de suspensas)
void task_exit (int exitCode)
{
	pause_timer(); // Pausamos o timer para garantir que essa função não seja\
	interrompida.

	// Atualizamos o tipo de tarefa e estado..
	current_task->task_status = finished;
	current_task->exit_code = exitCode;

	#ifdef DEBUG
		printf("Iniciando o encerramento da tarefa %d \n",\
		current_task->task_id);
		printf("Status de Encerramento: %d \n", exitCode);
	#endif

	// Atualizamos os tempos de execução, CPU e num de ativações.
	current_task->execution_time = total_sys_time\
	- current_task->creation_time;
	current_task->cpu_time += total_sys_time - current_task->last_call;

	// Printamos os tempos.
	printf("Task %d exit: Execution time %d ms, CPU time %d ms, %d activations.\
\n", current_task->task_id, current_task->execution_time,\
current_task->cpu_time, current_task->number_of_activations);

	// Verificamos a existencia de filhos (outras tarefas esperando nosso)\
	encerramento. Para isso, checamos se a fila de suspensas está nula ou não.
	if(suspended_queue != NULL)
	{
		// Se não está nula, então existe a chance de alguma tarefa dentro\
		da fila estar esperando pelo encerramento da nossa tarefa atual.
		task_t *task_aux = suspended_queue;

		// Verificamos se a cabeça da fila de suspensas é "filha" nossa.
		if(task_aux->task_father == current_task->task_id)
		{
			// Se sim, da resume nessa tarefa (coloca ela de volta na fila\
			de prontas).
			task_resume(task_aux);
		}
		task_aux = task_aux->next;

		// Verificamos o resto da fila de suspensas procurando por todos os\
		possiveis filhos da tarefa atual.
		while(task_aux != suspended_queue && task_aux != NULL &&\
			  suspended_queue != NULL)
		{
			// Se for filho, resume a tarefa.
			if(task_aux->task_father == current_task->task_id)
			{
				task_resume(task_aux);
			}
			task_aux = task_aux->next;
		}
	}

	// Por fim, ao acordar todos os possíveis filhos, retorna para o dispatcher.
	task_switch(dispatcher);
}

// Função: Task Suspend
// Suspende uma tarefa, colocando-a na fila de suspensas.
void task_suspend (task_t *task)
{
	pause_timer();
	task_t* suspended_task; // Ponteiro para segurar as informações referentes\
	a tarefa que será suspensa.

	// Checamos se a tarefa recebida no parametro é nula ou não.
	if(task != NULL)
	{
		// Se não for nula, a tarefa a ser suspensa é a recebida por parametro.
		suspended_task = task;
	}
	else
	{
		// Se for nula, suspendemos a tarefa atual.
		// Como a tarefa atual já está fora da fila, não precisamos retira-lá\
		de lugar algum.
		suspended_task = current_task;
	}

	// Atualizamos seu estado e recarregamos seu Quantum
	// O quantum é naturalmente carregado nos momentos de preempção, mas fora\
	disso, somente aqui.
	suspended_task->task_status = suspended;
	suspended_task->remanining_quantum = TASK_QUANTUM;

	// Adicionamos a tarefa na fila de suspensas.
	// Antes, checamos se a tarefa é a tarefa atual ou não. Pois se não for\
	temos que remover essa tarefa da fila de prontas.
	if(suspended_task != current_task)
	{
		suspended_task = (task_t*) queue_remove((queue_t**)&ready_queue,\
		(queue_t*)suspended_task);
	}
	queue_append ((queue_t**)&suspended_queue, (queue_t*)suspended_task);
}

// Função: Task Resume
// Acorda uma tarefa da fila de suspensas, colocando-a de volta na fila de\
tarefas prontas.
void task_resume (task_t *task)
{
	task->task_status = ready;
	task->task_father = -1; // Ao ir para fila de prontas, ela deixa de ter pai.
	// Remove da fila de suspensas,
	task = (task_t*) queue_remove((queue_t**)&suspended_queue, (queue_t*)task);
	// Coloca na fila de tarefas prontas.
	queue_append ((queue_t**)&ready_queue, (queue_t*)task);
}

// Função: Task Yield
// retira a tarefa atual do processador e coloca ela de novo na fila de prontas.
void task_yield ()
{
	pause_timer();
	// Se a tarefa atual não for o despachante e seu pai é -1, (não tem pai)
	if(current_task != dispatcher && current_task->task_father == -1)
	{
		// colocamos ela na fila de prontas.
		queue_append((queue_t**) &ready_queue,(queue_t*) current_task);
	}

	// trocamos para o Despachante
	task_switch(dispatcher);
}

// Função: Task Setprio
// Atualiza as informações de prioridade da tarefa recebida.
void task_setprio (task_t *task, int priority)
{
	// Verificamos se a tarefa é a atual (NULL como parametro) ou se é uma\
	tarefa válida
	if(task == NULL)
	{
		// AMBOS os valores devem ser atualizados. A dinamica APENAS É\
		MODIFICADA quando existe interrupções, ou recebe o processador pelo\
		escalonador.
		current_task->estatic_priority = priority;
		current_task->dinamic_priority = priority;
	}
	else
	{
		task->estatic_priority = priority;
		task->dinamic_priority = priority;
	}
}

// Função: Task getprio
// Retorna as informações da prioridade dinamica da tarefa.
// A dinamica é sempre igual ou maior que a estática, e se ela for maior, seu\
valor importa mais que a estatica.
int task_getprio (task_t *task)
{
	if(task == NULL)
	{
		// Se a tarefa recebida por parametro é nula, então retornamos o valor\
		da tarefa atual.
		return current_task->dinamic_priority;
	}
	else
	{
		// Se não for nula, então é da tarefa recebida por parametro.
		return task->dinamic_priority;
	}
}

// Função: Task Join
// Associa a tarefa corrente a tarefa recebida por parametro.
// Ao receber um JOIN, a tarefa atual imediantamente é suspensa.
int task_join (task_t *task)
{
	// Atualizamos o ponteiro de pai
	current_task->task_father = task->task_id;
	// Suspendemos nós mesmos.
	task_suspend(current_task);
	// Chamamos o Yield, que vai ir para o dispachante.
	task_yield();

	return task->exit_code;
}

// Função: Task Sleep
// Adormece a tarefa atual por T segundos.
// Tarefas adormecidas perdem o processador e vão para a fila de tarefas\
adormecidas.
void task_sleep (int t)
{
	pause_timer(); // não atrapalhar no processo.
	// Como é a tarefa atual que recebe sleep, (e a mesma não está na fila\
	de prontas), não precisamos removê-la de lá.

	// Recebe período de tempo para dormir.
	current_task->sleeping_time = t*1000 + total_sys_time;
	// Bota a tarefa para dormir.
	current_task->task_status = sleeping;
	current_task->remanining_quantum = TASK_QUANTUM;
	queue_append((queue_t**) &sleeping_queue,(queue_t*) current_task);

	// Trocamos para o despachante.
	task_switch(dispatcher);
}

// operações de IPC ============================================================

// semáforos

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value) ;

// requisita o semáforo
int sem_down (semaphore_t *s) ;

// libera o semáforo
int sem_up (semaphore_t *s) ;

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) ;

// mutexes

// Inicializa um mutex (sempre inicialmente livre)
int mutex_create (mutex_t *m) ;

// Solicita um mutex
int mutex_lock (mutex_t *m) ;

// Libera um mutex
int mutex_unlock (mutex_t *m) ;

// Destrói um mutex
int mutex_destroy (mutex_t *m) ;

// barreiras

// Inicializa uma barreira
int barrier_create (barrier_t *b, int N) ;

// Chega a uma barreira
int barrier_join (barrier_t *b) ;

// Destrói uma barreira
int barrier_destroy (barrier_t *b) ;

// filas de mensagens

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size) ;

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg) ;

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg) ;

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue) ;

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue) ;
