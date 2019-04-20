/*
Equipe: Amanda S. e Matheus F.
Sistemas Operacionais 2019/1
LAB 02.
*/
#include "pingpong.h"
#include "datatypes.h"
#include <stdio.h>
#include <stdlib.h>

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

//Definição de variáveis globais.
#define DEBUG
#define STACKSIZE 32768		/* tamanho de pilha das threads */


int contador_tarefa;       // Para gerar os IDs das tarefas.
task_t *PingPongMain;   // A main do Ping Pong tem que ser uma tarefa também.
task_t *taskAtual;         // Para saber qual é a tarefa sendo executada no momento
// ******************************
//Inicializa as estruturas internas do Ping Pong OS.
void pingpong_init()
{
    setvbuf(stdout, 0, _IONBF, 0); /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    PingPongMain = malloc(sizeof(task_t));  // Inicializamos a Ping Pong Main com a estrutura de tarefas
    PingPongMain->tid = 0;                         // O ID da Ping Pong Main será 0, o primeiro de todos.
    //Função que desativa o buffer do printf.
    taskAtual = PingPongMain;                    // Nossa tarefa atual passa a ser nossa main.
    contador_tarefa = 1;                             // As próximas tarefas terão ID de 1 em diante.


  #ifdef DEBUG
    printf("PingPong iniciado com sucesso.\n");
  #endif
}


// *task                          descritor da nova tarefa
// void (*start_func)(void *)     funcao corpo da tarefa
// void *arg                      argumentos para a tarefa
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

   //  Imprime a mensagem de depuração.
    #ifdef DEBUG
        printf("Tarefa %d criada com sucesso. \n", task->tid);
    #endif
    return(task->tid);
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
    // Imprime a mensagem de depuração para a tarefa atual.
    #ifdef DEBUG
        printf("Iniciando o encerramento da tarefa %d \n", taskAtual->tid);
    #endif
    // Imprime o status de encerramento.
        printf("Status de Encerramento: %d \n", exitCode);
    // Retorna para a função main do Ping Pong.
        task_switch(PingPongMain);
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
