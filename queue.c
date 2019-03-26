#include "queue.h"
#include <stdlib.h>

    /* Função para criar / iniciar uma fila
    queue_t* create (void) {
      return NULL;
    }
    */
    /* Função para deletar uma fila existente
    void delete (queue *queue) {
      while (queue != NULL) {
        queue *aux = queue->next;
            free (queue);
        queue = aux;
      }
    }
    */
    /* Função para adicionar um elemento na fila*/
    void queue_append (queue_t **queue, queue_t *elem)
    {
        // Condicoes a verificar, gerando msgs de erro:
        // - a fila deve existir
        // - o elemento deve existir
        // - o elemento nao deve estar em outra fila
        if( (elem->next != NULL) && (elem->prev != NULL) )
        {   //Se ele já aponta para algo, ele pertence em alguma fila.
            return printf("Elemento já pertence em alguma fila").
        }
      queue* new =(queue*)malloc(sizeof(queue));
      new->next = queue;  /*A lista anterior é anexada a novo.*/
      new->prev = NULL;
      /* Verifica se lista não está vazia. */
      if (queue != NULL)
        queue->prev = new; /*A ponteiro anterior da lista agora é novo.*/
    }

    /* */
    queue_t* queue_remove (queue_t **queue, queue_t *elem)
    {
      queue *aux = search (queue, elem);
      if (aux != NULL) {
        if (aux->prev == NULL) {
          /*Elemento na cabeça da lista!*/
          queue = aux->next;
          if (queue != NULL)
            queue->prev = NULL;
        }
        else if (aux->next == NULL) {
          /*Elemento na cauda da lista!*/
          aux->prev->next = NULL;
        }
        else {
          /*Elemento no meio da lista!*/
          aux->prev->next = aux->next;
          aux->next->prev = aux->prev;
        }
        free(aux);
        return queue;
      }
      else {
        return NULL;
      }
    }
int queue_size (queue_t *queue) ;
void queue_print (char *name, queue_t *queue, void print_elem (void*) ) ;
void print_elem (void *ptr)
    /*
    queue* search (queue* queue, int elem) {
      queue *v; //var. para percorrer a lista
      for (v = lista; v != NULL; v = v->next) {
        if (v->info == elem) {
           return v;
        }
      }
      return NULL;
    }
    */
    /*
    queue* last (queue* queue) {
      queue *v, *prv = NULL; /*var. para percorrer a lista
      for (v = queue; v != NULL; v = v->next) {
        prv = v;
      }
      return prv;
    }

    /*
    void print_previous (queue *queue) {
      queue *v; //var. para percorrer a lista
      for (v = queue; v != NULL; v = v->prev) {
        printf(" %d ", v->info);
      }
      printf("\n");
    }

    /*
    void imprimir_posteriores (queue *queue) {
      queue *v; //var. para percorrer a lista
      for (v = lista; v != NULL; v = v->next) {
        printf(" %d ", v->info);
      }
      printf("\n");
    }
*/
