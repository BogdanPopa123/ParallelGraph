// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
//mutex[0] - mutexul pentru marcarea nodurilor
//mutex[1] - mutexul pentru suma
pthread_mutex_t mutexes[2];

/* TODO: Define graph task argument. */
//argumentul lui process neighbours este id ul vecinului, nu mai e nevoie sa fac struct doar pentru un int

void process_neighbour(void *arg)
{
	int *index = (int *)(arg);
	int idx = *index;

	pthread_mutex_lock(&mutexes[0]);
	if (graph->visited[idx] == 0)
		graph->visited[idx] = 1; //schimbam statusul nodului curent in processing

	pthread_mutex_unlock(&mutexes[0]);

	pthread_mutex_lock(&mutexes[1]);
	sum = sum + graph->nodes[idx]->info;
	pthread_mutex_unlock(&mutexes[1]);


	unsigned int i;

	for (i = 0; i < graph->nodes[idx]->num_neighbours; i++) {
		int *node_index = malloc(sizeof(int));
		*node_index = graph->nodes[idx]->neighbours[i];


		pthread_mutex_lock(&mutexes[0]);
		if (graph->visited[graph->nodes[idx]->neighbours[i]] == 0) {
			os_task_t *task = create_task(process_neighbour, (void *)(node_index), free);

			graph->visited[graph->nodes[idx]->neighbours[i]] = 1; //schimbam statusul nodului vecin in processing
			enqueue_task(tp, task);
		}
		pthread_mutex_unlock(&mutexes[0]);
	}

	graph->visited[idx] = 2; //schimbam statusul nodului curent in done
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	pthread_mutex_lock(&mutexes[0]);
	if (graph->visited[idx] == 0)
		graph->visited[idx] = 1; //schimbam statusul nodului curent in processing

	pthread_mutex_unlock(&mutexes[0]);

	pthread_mutex_lock(&mutexes[1]);
	sum = sum + graph->nodes[idx]->info;
	pthread_mutex_unlock(&mutexes[1]);

	//cream task cu fiecare vecin si il bagam in queue
	unsigned int i;

	for (i = 0; i < graph->nodes[idx]->num_neighbours; i++) {
		int *node_index = malloc(sizeof(int));
		*node_index = graph->nodes[idx]->neighbours[i];


		pthread_mutex_lock(&mutexes[0]);
		if (graph->visited[graph->nodes[idx]->neighbours[i]] == 0) {
			os_task_t *task = create_task(process_neighbour, (void *)(node_index), free);

			graph->visited[graph->nodes[idx]->neighbours[i]] = 1; //schimbam statusul nodului vecin in processing
			enqueue_task(tp, task);
		}
		pthread_mutex_unlock(&mutexes[0]);
	}

	graph->visited[idx] = 2; //schimbam statusul nodului curent in done
}


int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&mutexes[0], NULL);
	pthread_mutex_init(&mutexes[1], NULL);

	tp = create_threadpool(NUM_THREADS);

	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	pthread_mutex_destroy(&mutexes[0]);
	pthread_mutex_destroy(&mutexes[1]);

	return 0;
}
