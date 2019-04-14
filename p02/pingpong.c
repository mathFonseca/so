/*
*/
#include "pingpong.h"
#include "datatypes.h"


//Inicializa as estruturas internas do Ping Pong OS.
void pingpong_init()
{
  //Função que desativa o buffer do printf.
  setvbuf(stdout, 0, _IONBF, 0);
}
// *task                          descritor da nova tarefa
// void (*start_func)(void *)     funcao corpo da tarefa
// void *arg                      argumentos para a tarefa
int task_create (task_t *task, void (*start_func)(void *),	void *arg)
{
  return;
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
