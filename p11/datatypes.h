// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>

enum task_status{executing, ready, finished, suspended, sleeping, semaphore, barrier};
enum task_type{system_task, user_task};

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next ; // para usar com a biblioteca de filas (cast)
    int task_id ;   // ID da tarefa
	ucontext_t context;    // variável que guarda o contexto atual da tarefa
	enum task_status task_status;
    enum task_type task_type;
    int estatic_priority;  // Prioridade estática
	int dinamic_priority;  // Prioridade dinâmica
	int remanining_quantum;   // Quantum restante da tarefa.
	int execution_time;    // Variável que conta o tempo total de execução
	int cpu_time; // Variável que conta o tempo que a tarefa ficou no\
     processador
	int number_of_activations; // Variável que conta o número de ativações
	int creation_time; // Guarda o número do tick de quando a tarefa foi criada
	int last_call;   // Guarda o número do tick de quando a tarefa foi\
     executada pela última vez
	int task_father;   // Guarda o id tarefa pai (em caso de Join). Recebe -1\
    Se a tarefa não tiver pai.
    int sleeping_time;  // Guarda o tempo de sono que a tarefa precisa dormir\
    caso ela seja executada com task_sleep.
    int barrier_linked; // Guarda o id da barreira, caso a tarefa esteja\
    esperando a liberação de uma. Deve ser distinto de task_father pois JOIN\
    de barreiras e tarefas são coisas diferentes.
	int exit_code;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
    int slots;    // Número de vagas do semáforo.
    task_t* task_queue; // Fila de tarefas que aguardam esse semáforo ficar
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  int max_slots;
  int slots_used;
  int barrier_id;
  task_t* task_queue;
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
