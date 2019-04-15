/*
Equipe: Amanda S. e Matheus F.
Sistemas Operacionais 2019/1
LAB 02.
*/
#include "pingpong.h"
#include "datatypes.h"
#include <stdio.h>
#include <stdlib.h>

//Definição de variáveis globais.
#define DEBUG
#define STACKSIZE 32768		/* tamanho de pilha das threads */

int contador_tarefa;
//Inicializa as estruturas internas do Ping Pong OS.
void pingpong_init()
{
  //Função que desativa o buffer do printf.
  setvbuf(stdout, 0, _IONBF, 0);
  
  #ifdef DEBUG
    printf("PingPong iniciado.\n");
  #endif
}
// *task                          descritor da nova tarefa
// void (*start_func)(void *)     funcao corpo da tarefa
// void *arg                      argumentos para a tarefa
int task_create (task_t *task, void (*start_func)(void *),	void *arg)
{
  getcontext(&(task->context)); //Pega o contexto da tarefa/task recebida (no parametro;)
 
  //Seção igual ao context.c
  //Com adição da linha que cria a pilha
  char* stack; //cria a pilha
  stack = malloc (STACKSIZE); //aloca a pilha
   if (stack)
   {
      ContextPing.uc_stack.ss_sp = stack ;
      ContextPing.uc_stack.ss_size = STACKSIZE;
      ContextPing.uc_stack.ss_flags = 0;
      ContextPing.uc_link = 0;
   }
   else
   {
     printf("Erro ao inicializar a pilha.");
     return (-1);
   }
  
  int id; //id para a tarefa do parâmetro
  int id_posterior; //id da última tarefa.
  /*Como achar o prev se meu "task" veio vazio?
  if(task->prev == NULL)
  {
    //Não existe nenhuma tarefa anterior. Nesse caso, é nossa primeira tarefa do sistema.
    id = 1;
    task.tid = id;
    return id;
  }
  else
  {
    //Já existe ao menos alguma tarefa anterior.
    id_posterior = task->prev.tid;
    id_posterior++;
    return id_posterior;
  }
  */
  //Como checar se tem erros?
  //O que seriam esses erros?
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
  return;
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id ()
{
  return;
}
