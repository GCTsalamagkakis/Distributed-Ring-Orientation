#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>

typedef struct Node{

    int succ;
    int pred;
    int has_token;
    int is_sending;
    int is_receiving;
    char state;
}node;

void initialize_mpi(){

    MPI_Init(NULL, NULL);
}

int get_world_size(){

	printf("[INFO] Main - Main: Retrieving world size\n");
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    return world_size;
}

int get_world_rank(){

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    return world_rank;
}

int get_random_neighbor(int world_size, int world_rank){

    int neighbor = world_rank;
    while(neighbor == world_rank) neighbor = rand() % world_size;
    return neighbor;
}

char get_random_state(int world_rank){

    char states[3] = {'S', 'R', 'I'};
    char state = rand() % 3;
    return states[state];
}

int get_token_if_send_state(char state){

    if (state == 'S') return 1;
    return 0;
}

void get_random_orientation(node* node, int world_size, int world_rank){

    int clockwise = rand() % 2;
    if(clockwise){
        if(world_rank == world_size - 1){
            node->succ = 1;
            node->pred = world_rank - 1;
        }else if(world_rank == 1){
            node->succ = world_rank + 1;
            node->pred = world_size - 1;
        }else{
            node->succ = world_rank + 1;
            node->pred = world_rank - 1;
        }
    }else{
        if(world_rank == world_size - 1){
            node->succ = world_rank - 1;
            node->pred = 1;
        }else if(world_rank == 1){
            node->succ = world_size - 1;
            node->pred = world_rank + 1;
        }else{
            node->succ = world_rank - 1;
            node->pred = world_rank + 1;
        }
    }
}

void initialize_node(node* node, int world_size, int world_rank){

	printf("[INFO] Main - node%d: Initializing with random values\n", world_rank);
    int i;
    srand(time(NULL) + world_rank * world_rank);
    get_random_orientation(node, world_size, world_rank);
    node->state = get_random_state(world_rank);
    node->has_token = get_token_if_send_state(node->state);
    node->is_sending = 0;
    node->is_receiving = 0;
}

void check_for_size_underflows(int world_size, int world_rank){

    if (world_size < 3){
        printf("[ERROR] Main - node %d: single node can not create a ring\n", world_rank);
        exit(0);
    }else if(world_size == 3){
        printf("[INFO] Main - node %d: with 2 nodes, the graph is automatically a ring\n", world_rank);
        exit(0);
    }
}

void copy_from_dto_to_node(node* node_in_ring, int* dto, char* states){

	node_in_ring->succ = dto[1];
    node_in_ring->pred = dto[2];
    node_in_ring->state = states[dto[3]];
    node_in_ring->has_token = dto[4];
    node_in_ring->is_sending = dto[5];
    node_in_ring->is_receiving = dto[6];
}

int is_repository(int world_rank){

	return world_rank == 0;
}

void inform_repo_about_exchange(node* ring, int initiator, char exchange_type){

	if (exchange_type == 'S'){
		int next = ring[initiator].succ;
		printf("[INFO] Main - node%d: Sending message to node %d\n", initiator+1, next);
		ring[initiator].has_token = 0;
		ring[initiator].is_sending = 0;
		ring[next].has_token = 1;
	}else if(exchange_type == 'R'){
		int prev = ring[initiator].pred;
		printf("[INFO] Main - node%d: Receiving message from node %d\n", initiator+1, prev);
		ring[initiator].has_token = 1;
		ring[initiator].is_receiving = 0;
		ring[prev].has_token = 0;
	}
}

void turn_node_to_dto(node* node, int* dto, int rank, char* states){

	int i;
	
	dto[0] = rank;
	dto[1] = node->succ;
	dto[2] = node->pred;
	for (i=0; i<3; i++)
		if(node->state == states[i])
			dto[3] = i;		
	dto[4] = node->has_token;
	dto[5] = node->is_sending;
	dto[6] = node->is_receiving;
}

void update_repo(node* ring, node ring_node, int world_size, int world_rank){

	int i, j;
	int current;
	char states[3] = {'I', 'S', 'R'};
	int node_info[7];
	if(is_repository(world_rank)){
		printf("[INFO] Main - repo: Updating repo with latest values\n");
    	for(i=1; i<world_size; i++){
    		current = i-1;
    		MPI_Recv(&node_info[0], 7, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    		copy_from_dto_to_node(&ring[current], node_info, states);
    		if(ring[current].is_sending){
    			inform_repo_about_exchange(ring, current, 'S');
    		}else if(ring[current].is_receiving){
    			inform_repo_about_exchange(ring, current, 'R');
    		}
    	}
    }else{
    	printf("[INFO] Main - node%d: Pushing updated values to repo\n", world_rank);
    	turn_node_to_dto(&ring_node, node_info, world_rank, states);
    	MPI_Send(&node_info[0], 7, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
}



void pull_neighbor_changes_from_repo(node* ring, int* neighborDTO1, int* neighborDTO2, int j, int world_rank, char* states){
	
	if(world_rank == j){
		printf("[INFO] Main - node%d: Pulling updated neighbor info from repo\n", world_rank);
		MPI_Recv(&neighborDTO1[0], 7, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&neighborDTO2[0], 7, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
	}
	else if (is_repository(world_rank)){
	
		printf("[INFO] Main - repo: Sending updated neighbors to node %d\n", j);
		int k;
		
		int next_neighbor = ring[j-1].succ;
		int prev_neighbor = ring[j-1].pred;
		turn_node_to_dto(&ring[next_neighbor-1], neighborDTO1, next_neighbor, states);
		turn_node_to_dto(&ring[prev_neighbor-1], neighborDTO2, prev_neighbor, states);
		
		MPI_Send(&neighborDTO1[0], 7, MPI_INT, j, 0, MPI_COMM_WORLD);
		MPI_Send(&neighborDTO2[0], 7, MPI_INT, j, 0, MPI_COMM_WORLD);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
}

int check_if_oriented(node* ring, int world_size, int world_rank){

	if (is_repository(world_rank)){
    	int is_in_chain[world_size - 1];
		int i;
		int pos = 0;
		int counter = 0;
		for(i=0; i<world_size-1;i++) is_in_chain[i] = 0;
		while(counter < world_size - 1){
			counter++;
			is_in_chain[pos] = 1;
			pos =  ring[pos].succ - 1;
		}
		for(i=0; i<world_size-1; i++) if(is_in_chain[i] == 0) return 0;
		return 1;
	}
}

void flip(node* ring_node){

	ring_node->succ += ring_node->pred;
    ring_node->pred = ring_node->succ - ring_node->pred;
    ring_node->succ -= ring_node->pred;
}



void execute_step1(node* current_node, node* neighbor, int neighbor_rank, int world_rank){

	printf("[INFO] Main - node%d: Checking condition 1 for neighbor %d\n", world_rank, neighbor_rank);
	if(!is_repository(world_rank)){
	if(current_node->state == 'I' 
	&& neighbor->state == 'S' 
	&& neighbor->succ == world_rank){
		printf("[INFO] Main - node%d: Changing State to R\n", world_rank);
        current_node->state = 'R';
        if(current_node->succ == neighbor_rank){
            printf("[INFO] Main - node%d: Flipping\n", world_rank);
            flip(current_node);
        }    
    }
    }
}

void execute_step3(node* current_node, node* neighbor, int neighbor_rank, int world_rank){

	printf("[INFO] Main - node%d: Checking condition 3 for neighbor %d\n", world_rank, neighbor_rank);
	if (current_node->state == 'R' && current_node->pred == neighbor_rank 
	&& !(neighbor->state == 'S' && neighbor->succ == world_rank)){
		printf("[INFO] Main - node%d: Changing State to S\n", world_rank);
        current_node->state = 'S';   
	}
}

void execute_step2(node* current_node, node* neighbor_node, int neighbor_rank, int world_rank){

	printf("[INFO] Main - node%d: Checking condition 2 for neighbor %d\n", world_rank, neighbor_rank);
	if (current_node-> state == 'S' 
	&& neighbor_node->state == 'R' 
	&& neighbor_node->pred == world_rank 
	&& current_node->succ == neighbor_rank){
		printf("[INFO] Main - node%d: Initializing message transfer\n", world_rank);
		printf("[INFO] Main - node%d: Changing State to I\n", world_rank);
	    current_node->state = 'I';
	    current_node->is_sending = 1;
	}
}

void execute_step4(node* current_node, node* neighbor_node, int neighbor_rank, int world_rank){

	printf("[INFO] Main - node%d: Checking condition 4 for neighbor %d\n", world_rank, neighbor_rank);
	if (current_node->state == 'S' 
	&& neighbor_node->state == 'S' 
	&& neighbor_node->succ == world_rank 
	&& current_node->succ == neighbor_rank){
        printf("[INFO] Main - node%d: Changing State to R\n", world_rank);
        printf("[INFO] Main - node%d: flipping\n", world_rank);
        printf("[INFO] Main - node%d: Initializing message transfer\n", world_rank);
        current_node->state = 'R';
        current_node->is_receiving = 1;
        flip(current_node);
	}
}

void execute_step5(node* current_node, node* neighbor_node, int neighbor_rank, int world_rank){
	printf("[INFO] Main - node%d: Checking condition 5 for neighbor %d\n", world_rank, neighbor_rank);
	if (current_node->state == 'I' 
	&& neighbor_node->state == 'I' 
	&& neighbor_node->succ == world_rank 
	&& current_node->succ == neighbor_rank){
    	printf("[INFO] Main - node%d: Changing state to S\n", world_rank);
    	printf("[INFO] Main - node%d: Spawning token\n", world_rank);
        current_node->state = 'S';
        current_node->has_token = 1;
    }
}


void execute_step1_for_all_neighbors(node* ring, node* current_node, node* neighbor_node1, int neighbor_rank1, node* neighbor_node2, int neighbor_rank2, int world_size, int world_rank){

	execute_step1(current_node, neighbor_node1, neighbor_rank1, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
	execute_step1(current_node, neighbor_node2, neighbor_rank2, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
}

void execute_step2_for_all_neighbors(node* ring, node* current_node, node* neighbor_node1, int neighbor_rank1, node* neighbor_node2, int neighbor_rank2, int world_size, int world_rank){

	execute_step2(current_node, neighbor_node1, neighbor_rank1, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
	execute_step2(current_node, neighbor_node2, neighbor_rank2, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
}

void execute_step3_for_all_neighbors(node* ring, node* current_node, node* neighbor_node1, int neighbor_rank1, node* neighbor_node2, int neighbor_rank2, int world_size, int world_rank){

	execute_step3(current_node, neighbor_node1, neighbor_rank1, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
	execute_step3(current_node, neighbor_node2, neighbor_rank2, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
}

void execute_step4_for_all_neighbors(node* ring, node* current_node, node* neighbor_node1, int neighbor_rank1, node* neighbor_node2, int neighbor_rank2, int world_size, int world_rank){

	execute_step4(current_node, neighbor_node1, neighbor_rank1, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
	execute_step4(current_node, neighbor_node2, neighbor_rank2, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
}

void execute_step5_for_all_neighbors(node* ring, node* current_node, node* neighbor_node1, int neighbor_rank1, node* neighbor_node2, int neighbor_rank2, int world_size, int world_rank){

	execute_step5(current_node, neighbor_node1, neighbor_rank1, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
	execute_step5(current_node, neighbor_node2, neighbor_rank2, world_rank);
	update_repo(ring, *current_node, world_size, world_rank);
}

void get_rank_from_dto(int* dto, int* rank){
	*rank = dto[0];
}

int main(int argc, char** argv){
    initialize_mpi();
    int i, j;
    int world_size = get_world_size();
    int world_rank = get_world_rank();
    char states[3] = {'I', 'S', 'R'};
    int oriented;
    int neighborDTO1[7] = {0, 0, 0, 0, 0, 0, 0};
    int neighborDTO2[7] = {0, 0, 0, 0, 0, 0, 0};
    int neighbor1_rank;
    int neighbor2_rank;
    node neighbor_node_1;
    node neighbor_node_2;
    node ring[world_size-1];
    node ring_node;
    
    check_for_size_underflows(world_size, world_rank);

    
    
    if(!is_repository(world_rank)) initialize_node(&ring_node, world_size, world_rank);
    update_repo(ring, ring_node, world_size, world_rank);
    
    MPI_Barrier(MPI_COMM_WORLD);
    if(is_repository(world_rank)){
    	printf("[INFO] Main - repo: Initial State Of Ring\n");
    	for(i=0; i<world_size-1; i++){
    		printf("node: %d\tsuccesor: %d\tpredecesor: %d\tstate: %c\thas_token: %d\n", i+1, ring[i].succ, ring[i].pred, ring[i].state, ring[i].has_token);
    	}
    }
   
    oriented= check_if_oriented(ring, world_size, world_rank);
    MPI_Barrier(MPI_COMM_WORLD);
    
    while(!oriented){
    
    	oriented = check_if_oriented(ring, world_size, world_rank);
    	MPI_Barrier(MPI_COMM_WORLD);
    	for(j=1; j<world_size; j++){
    		
    		pull_neighbor_changes_from_repo(ring, neighborDTO1, neighborDTO2, j, world_rank, states);
			copy_from_dto_to_node(&neighbor_node_1, neighborDTO1, states);
			copy_from_dto_to_node(&neighbor_node_2, neighborDTO2, states);
			
			get_rank_from_dto(neighborDTO1, &neighbor1_rank);
			get_rank_from_dto(neighborDTO2, &neighbor2_rank);

			execute_step1_for_all_neighbors(ring, &ring_node, &neighbor_node_1, neighbor1_rank, &neighbor_node_2, neighbor2_rank, world_size, world_rank);
    		
		    pull_neighbor_changes_from_repo(ring, neighborDTO1, neighborDTO2, j, world_rank, states);
		    execute_step3_for_all_neighbors(ring, &ring_node, &neighbor_node_1, neighbor1_rank, &neighbor_node_2, neighbor2_rank, world_size, world_rank);
		    
    		pull_neighbor_changes_from_repo(ring, neighborDTO1, neighborDTO2, j, world_rank, states);
		    execute_step2_for_all_neighbors(ring, &ring_node, &neighbor_node_1, neighbor1_rank, &neighbor_node_2, neighbor2_rank, world_size, world_rank);
		    
		    pull_neighbor_changes_from_repo(ring, neighborDTO1, neighborDTO2, j, world_rank, states);
		    execute_step4_for_all_neighbors(ring, &ring_node, &neighbor_node_1, neighbor1_rank, &neighbor_node_2, neighbor2_rank, world_size, world_rank);
			
		    pull_neighbor_changes_from_repo(ring, neighborDTO1, neighborDTO2, j, world_rank, states);
			execute_step5_for_all_neighbors(ring, &ring_node, &neighbor_node_1, neighbor1_rank, &neighbor_node_2, neighbor2_rank, world_size, world_rank);
    	}
   	}
    if (is_repository(world_rank)){
    	printf("[INFO] Main - repo: Final State Of Ring\n");
    	for(i=0; i<world_size-1; i++){
    		printf("node: %d\tsucc: %d\tpred: %d\tstate: %c\thas_token: %d\n", i+1, ring[i].succ, ring[i].pred, ring[i].state, ring[i].has_token);
    	}
    }
}