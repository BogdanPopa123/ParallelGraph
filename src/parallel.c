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
static int nodes_unprocessed;
/* TODO: Define graph synchronization mechanisms. */
//mutex[0] - mutexul pentru marcarea nodurilor
//mutex[1] - mutexul pentru suma
//mutex[2] - mutexul pentru variabila nodes_unprocessed
pthread_mutex_t mutexes[3];

/* TODO: Define graph task argument. */
//argumentul lui process neighbours este id ul vecinului, nu mai e nevoie sa fac struct doar pentru un int



int visited[1000];

void dfs(os_graph_t *graph, unsigned int node) {
    visited[node] = 1;
    for (unsigned int i = 0; i < graph->nodes[node]->num_neighbours; i++) {
        unsigned int neighbor = graph->nodes[node]->neighbours[i];
        if (!visited[neighbor]) {
            dfs(graph, neighbor);
        }
    }
}

int connected_component_size(os_graph_t *graph) {
    for (unsigned int i = 0; i < 1000; i++) {
        visited[i] = 0;
    }

    dfs(graph, 0);

    int component_size = 0;
    for (unsigned int i = 0; i < graph->num_nodes; i++) {
        if (visited[i]) {
            component_size++;
        }
    }

    return component_size;
}

int stop_signal(os_threadpool_t *tp)
{
	if (nodes_unprocessed == 0)
		return 1;

	return 0;
}



void process_neighbour(void *arg)
{
	int *index = (int *)(arg);
	int idx = *index;

	pthread_mutex_lock(&mutexes[0]);
	if (graph->visited[idx] == 0) {
		graph->visited[idx] = 1; //schimbam statusul nodului curent in processing
	}
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
			// os_task_t *task = create_task(process_neighbour, graph->nodes[idx]->neighbours[i], free);
			os_task_t *task = create_task(process_neighbour, (void *)(node_index), free);
			enqueue_task(tp, task);
			graph->visited[graph->nodes[idx]->neighbours[i]] = 1; //schimbam statusul nodului vecin in processing
		}
		pthread_mutex_unlock(&mutexes[0]);
	}

	graph->visited[idx] = 2; //schimbam statusul nodului curent in done

	// pthread_mutex_lock(&mutexes[2]);
	// nodes_unprocessed--;
	// pthread_mutex_unlock(&mutexes[2]);

}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	pthread_mutex_lock(&mutexes[0]);
	if (graph->visited[idx] == 0) {
		graph->visited[idx] = 1; //schimbam statusul nodului curent in processing
	}
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
			// os_task_t *task = create_task(process_neighbour, graph->nodes[idx]->neighbours[i], free);
			os_task_t *task = create_task(process_neighbour, (void *)(node_index), free);
			enqueue_task(tp, task);
			graph->visited[graph->nodes[idx]->neighbours[i]] = 1; //schimbam statusul nodului vecin in processing
		}
		pthread_mutex_unlock(&mutexes[0]);
	}

	graph->visited[idx] = 2; //schimbam statusul nodului curent in done
	// pthread_mutex_lock(&mutexes[2]);
	// nodes_unprocessed--;
	// pthread_mutex_unlock(&mutexes[2]);

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
	pthread_mutex_init(&mutexes[2], NULL);

	tp = create_threadpool(NUM_THREADS);
	
	// pthread_mutex_lock(&mutexes[2]);
	// nodes_unprocessed = connected_component_size(graph);
	// pthread_mutex_unlock(&mutexes[2]);

	process_node(0);
	// wait_for_completion(tp, stop_signal);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	pthread_mutex_destroy(&mutexes[0]);
	pthread_mutex_destroy(&mutexes[1]);
	pthread_mutex_destroy(&mutexes[2]);

	return 0;
}
