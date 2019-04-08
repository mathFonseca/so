#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
//Equipe:
// Amanda Schmidt
// Matheus Fonseca
// Sistemas Operacionais. Ping Pong OS. 2019/1
//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem)
{
    //Checar se a fila existe (não é null)
    if(queue != NULL) //queue sem asterico é a estrutura em si.
    {
        //Checar se o elemento existe (não é null)
        if(elem != NULL)
        {
            //Checar se o elemento está em outra fila (seus ponteiros apontam para algo)
            if( elem->next == NULL && elem->prev == NULL)
            {
                //Tudo OK, pode inserir.
                //Caso 1 = Fila vazia.
                if( (*queue) == NULL) //queue* com asterico é uma estrutura que existe
                                       //mas não aponta para nada (vazia).
                {
                    //Por estar vazia, podemos inserir o elemento diretamente.
                    (*queue) = elem;
                    elem->next = elem;
                    elem->prev = elem;
                    //Por ser duplamente encadeada, tem que apontar para si mesmo.
                }
                //Caso 2 = Fila com algum elemento dentro
                else
                {
                    queue_t* aux; //Cria um auxiliar.
                    aux = (*queue)->prev;   //Inserimos no fim da fila.
                    (*queue)->prev = elem;  //A fila (prev) aponta pro novo elemento.
                    elem->next = (*queue);  //O elem (next) aponta pra fila.
                    aux->next = elem;       //O antigo último elemento aponta para elem.
                    elem->prev = aux;       //O elem (prev) aponta para esse cara.
                    
                }                
            }
            else
            {
                printf("O elemento está em outra fila. (Seus ponteiros não são NULL)");
            }
        }
        else
        {
            printf("O elemento não existe");
        }
    }
    else
    {
        printf("A fila não existe"); //Não existe return em função void.
    }

}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
    //Verifica se a fila existe.
    if( queue == NULL) 
    {
        printf("Fila não existe. Não foi possível excluir");  
        
    }
    else if( (*queue) == NULL)  //Verifica se a fila está vazia.
    {
        printf("A fila está vazia. Não foi possível excluir");
    }     
    else if(elem==NULL)  //O elemento deve existir
    {
        printf("O elemento não existe. Não foi possível excluir");
    }
    else 
    {
         //Para pertencer na fila, precisamos primeiro encontrar ele na fila.
         //Alguns casos especiais
         //Caso 1. É o primeiro elemento.
         if(elem == (*queue))
         {
            //Caso 2. Ele é primeiro E o único da fila.
            if( elem->next == elem && elem->prev == elem)
            {
               //(elem) deve receber NULL em seus ponteiros, pois não pertence mais a fila.
               elem->next = NULL;
               elem->prev = NULL;
               //Como a fila só tinha elem, ela se torna vazia.
               (*queue) = NULL;
               return elem;
            }
            //Não é o único da fila, mas ainda era o primeiro.
            //Este caso especial existe, pois por mais que a fila seja duplamente encadeada, ela ainda tem um "Início".
            //Se o elemento a ser removido é o nosso início, temos que modificar este ponteiro para o próximo.
            queue_t* aux = (*queue)->next; //O nosso novo início será o próximo após queue.
            elem->prev->next = aux; //O último elemento da fila aponta para o novo início.
            aux->prev = elem->prev; //O novo início (prev) aponta para o fim da fila.
            //(elem) deve receber NULL em seus ponteiros, pois não pertence mais a fila.
            elem->next = NULL;
            elem->prev = NULL;
            //Nossa (*queue) deve apontar pro novo início, salvo em aux.
            *queue = aux;
            return elem;
          }
          //Caso não é o primeiro elemento, devemos percorrer até encontrar.
          //Não sabemos o tamanho da fila, então vamos percorrer até chegarmos na head novamente.
          queue_t* aux = (*queue)->next;
          while(aux != *queue)
          {
            //Iremos percorrer a fila até chegarmos na head. Se isso acontecer, o elemento não pertence a essa fila.
            //Como inicializamos aux como sendo o (*queue)->next, iremos primeiro checar e depois percorrer.
            if(aux == elem) //A comparação já checa os ponteiros. Se encontramos o elemento, preparamos para remover.
            {
               aux = elem->next;
               aux->prev = elem->prev; //Passa a apontar para o elemento após (elem).
               elem->prev->next = aux; //Este elemento (prev) aponta para o elemento antes de (elem).
               //(elem) deve receber NULL em seus ponteiros, pois não pertence mais a fila.
               elem->next = NULL;
               elem->prev = NULL;
               return elem;
               //Retornamos o elem;
             }
             aux = aux->next;
             //Porém, pode ser que nunca encontremos elem (nunca pertenceu a fila).
             if(aux == (*queue)) //Chegou na head novamente. 
             //Como anteriormente já checamos no caso de elem == *queue, não há a possibilidade dele ser a head nesse momento
             {
               return NULL;//Não foi possível remover, o elemento nunca existiu nesta fila.                        
             }
         }
      }
      return NULL;
}
//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue)
{
    if(queue==NULL) //Se a fila não existe, ela tem tamanho 0.
    {
        return 0;
    }
    else
    {   queue_t* aux = queue->next;
        int tamanho = 1;
        //Iremos rodar a fila até chegarmos na head novamente. Por isso, já contamos a head "previamente", e assim nosso tamanho começa em 1.
        while( aux != queue)
        {   //Enquanto não chegar na head da fila.
            aux = aux->next; //Vamos para o próximo elemento.
            tamanho++;       //Aumentamos a contagem em 1.
        }
        return tamanho;
    }
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    int tamanho, i;
    //Precisamos saber o tamanho da fila para saber quando parar de imprimir.
    tamanho = queue_size(queue);
    queue_t* aux = queue;
    //Acredito que char* name seja o "nome" da fila. Por isso, imprimo na linha debaixo.
    printf("%c\n", *name);  //Porque que é assim? Pesquisar.
    //Como sabemos o tamanho da fila, podemos (agora sim), percorrer usando FOR.
    for(i=0; i < tamanho; i++)
    {   
        //Usamos a função externa para imprimir o conteúdo da fila.
        print_elem(aux);
        //O aux passa a ser o próximo elemento.
        aux = aux->next;

    }
}

