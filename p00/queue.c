#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
// Função para achar um elemento da fila.

int search (queue_t **queue, queue_t *elem)
{
	queue_t **aux;
	for (aux = queue; *aux != NULL; *aux = (*aux)->next)
	{
		if ((*aux)->next == elem->next && (*aux)->prev == elem->prev)
		{
			return 1;
		}
	}
	return 0;
}

// Função para adicionar um elemento na fila.
void queue_append (queue_t **queue, queue_t *elem)
{
	// A fila existe? (não é nula)
	// o elemento existe? (não é nulo)
	// ele pertence alguma outra fila?
	if( queue != NULL && elem != NULL && (elem->next == NULL && elem->prev == NULL))
	{
		// Tudo pronto para adicionar ele no "FINAL" da fila.
		elem->next = (*queue)->prev->next;
		(*queue)->next->prev = elem;
		elem->prev = (*queue)->prev;
		(*queue)->prev = elem;
	}
	else
	{
		printf("O elemento está em alguma outra fila");
		printf(" ou o elemento não existe");
		printf("ou a fila especificada não existe.");
	}
}

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
	if( queue != NULL && ((*queue)->next != NULL && (*queue)->prev != NULL))
	{
		if(search(queue,elem))//pesquisar se o elemento ta nessa fila (função search?)
		{
			elem->prev->next = elem->next;
			elem->next->prev = elem->prev;
			elem->next = NULL;
			elem->prev = NULL;
			return elem;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}
int queue_size (queue_t *queue)
{
	int size = 1;
	queue_t *aux = queue;
	while(aux->next != queue->prev->next)
	{
		aux->next = aux->next->next;
		size++;
	}
	return size;
}
void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{

}
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
