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
	// A fila existe
	if(queue != NULL)
	{
		// Elemento existe / não pertence a mais nenhuma fila.
		if(elem != NULL && (elem->next == NULL && elem->prev == NULL))
		{
			// Insere um elemento em uma fila que já tem algo nela.
			elem->next = (*queue)->prev->next;
			elem->prev = (*queue)->prev;
			// elem agora é o elemento anterior de queue.
			// agora atualizamos o elemento anterior de queue para apontar para elem.
			(*queue)->prev->next = elem;
			(*queue)->prev = elem;
		}
		else
		{
			// mata a função?
			printf("Elemento não existe ou pertence a outra fila.");
		}
	}
	else
	// Uma fila = NULL é nova, acabou de ser criada.
	{
		// Inserindo o primeiro elemento de todos.
		// Ele aponta para si mesmo, nos dois sentidos.
		*queue = elem;
		(*queue)->next = elem;
		(*queue)->prev = elem;
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
	if(queue != NULL)
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
	else
	{
		return 0;
	}
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
