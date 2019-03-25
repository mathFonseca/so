#include "queue.h"

/* */
queue* create (void) {
  return NULL;
}

/* */
void delete (queue *queue) {
  while (queue != NULL) {
    queue *aux = queue->next;
    free (queue);
    queue = aux;
  }
}

/* */
queue* append (queue* queue, int elem) {
  queue* new =(queue*)malloc(sizeof(queue));
  new->info = elem;
  new->next = queue; /*A lista anterior é anexada a novo.*/
  new->prev = NULL;
  /* Verifica se lista não está vazia. */
  if (queue != NULL)
    queue->prev = new; /*A ponteiro anterior da lista agora é novo.*/
  return new;
}

/* */
queue* remove (queue *queue, int elem) {
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

/* */
queue* search (queue* queue, int elem) {
  queue *v; /*var. para percorrer a lista*/
  for (v = lista; v != NULL; v = v->next) {
    if (v->info == elem) {
       return v;
    }
  }
  return NULL;
}

/* */
queue* last (queue* queue) {
  queue *v, *prv = NULL; /*var. para percorrer a lista*/
  for (v = queue; v != NULL; v = v->next) {
    prv = v;
  }
  return prv;
}

/* */
void print_previous (queue *queue) {
  queue *v; /*var. para percorrer a lista*/
  for (v = queue; v != NULL; v = v->prev) {
    printf(" %d ", v->info);
  }
  printf("\n");
}

/* */
void imprimir_posteriores (queue *queue) {
  queue *v; /*var. para percorrer a lista*/
  for (v = lista; v != NULL; v = v->next) {
    printf(" %d ", v->info);
  }
  printf("\n");
}
