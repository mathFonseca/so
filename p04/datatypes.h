// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__
#include <ucontext.h> //Incluir a biblioteca para contexto das tarefas.

enum status_t{pronta, executando, suspensa, terminada};
// Estrutura que define uma tarefa
typedef struct task_t
{
  struct task_t *prev, *next;     // Para usar com a biblioteca de filas do lab 0.
  int tid;                                  // ID da tarefa
  ucontext_t context;               // Indica o contexto da tarefa
  enum status_t status;
  int prio_dim;                         // Prioridade Dinamica
  int prio_est;                          // Prioridade Estatica
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
