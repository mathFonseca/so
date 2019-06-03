/*
Equipe:
Amanda S. RA: 1658921
Matheus F. RA: 1794027
Sistemas Operacionais 2019/1
LAB 06.
*/
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__
#include <ucontext.h> //Incluir a biblioteca para contexto das tarefas.

enum status_t{pronta, executando, suspensa, terminada};
enum tipo_t{sistema, usuario};
// Estrutura que define uma tarefa
typedef struct task_t
{
  struct task_t *prev, *next;	// Para usar com a biblioteca de filas do lab 0.
  int tid;  // ID da tarefa
  ucontext_t context;   // Indica o contexto da tarefa
  enum status_t status; // Define o status da tarefa.
  enum tipo_t tipo; // Define o tipo da tarefa (Sistema ou Usuário)
  int prio_dim; // Prioridade Dinamica
  int prio_est; // Prioridade Estatica
  int quantum;  // Quantum restante de uma tarefa.
  int tempo_exec;   // Tempo total de execução.
  int tempo_proc;   // Tempo total dentro do processador.
  int num_ativ; // Número de ativações da tarefa até concluir.
  int ticks_inicial;    // Quantos ticks tinha o sistema quando a tarefa\
      executou pela primeira vez.
  int ticks_final;  // Quantos ticks tinha o sistema quando a tarefa terminou\
   de executar. (Ai dá pra ver o tempo total mais facilmente)
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
