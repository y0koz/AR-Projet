#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

#define M 64
#define K 6
#define TAG_INIT 0
#define TAG_INIT_FINGER 1
#define TAG_ASK 2
#define TAG_LAST_CHANCE 3
#define TAG_ANSWER 4
#define TAG_STOP 5


struct finger {
	int value;
	int chord_id;
	int mpi_id;
};

//Retourne le respondable de la ressource tofind parmis les chords_ids
int chord_find_supervisor(int *chord_ids, int nb_nodes, int tofind)
{
	for(int i = 0; i < nb_nodes; ++i)
	{
		if( (chord_ids[i] >= tofind))
			return chord_ids[i];
	}
	return chord_ids[0];
}

//Pre condition: L'indice des chord_ids correspond au futur rang MPI - 1 (chord_ids[0] ==> mpi_rank 1)
void generate_finger_table(int *fingerTable, int K_value, int *chord_ids, int nb_nodes, int id)
{
	int value = 0;
	int node_value = 0; 

	for(int i =0; i < K_value; ++i)
	{
		value = ((chord_ids[id] + (1 << i)) % ( 1 << K_value ));
		fingerTable[3*i] = value;
		node_value = chord_find_supervisor(chord_ids, nb_nodes, value);
		fingerTable[3*i+1] = node_value;
		if((node_value = find_in_array(chord_ids, nb_nodes, node_value)) != -1)
			fingerTable[3*i+2] = (node_value + 1); //cf pre-condition
		else
			printf("Error, chord_id %d not found\n", node_value);

	}
}

void display_finger_table(int *fingerTable, int K_value, int chord_id)
{
	printf("Finger Table of %d\n", chord_id);
	printf("+------------------------+\n");
	printf("| i| value | chord | mpi |\n");
	printf("|--|-------|-------|-----|\n");
	for(int i =0; i < K_value; ++i)
	{
		printf("|%2d|%7d|%7d|%5d|\n", i, fingerTable[i*3], fingerTable[i*3+1], fingerTable[i*3+2]);
	}
	printf("+------------------------+\n\n");
}

void display_help()
{
	printf("\n");
	printf("+---------------------------------H-E-L-P----------------------------------+\n");
	printf("|     Command     |          Fonction         |            Args            |\n");
	printf("|--------------------------------------------------------------------------|\n");
	printf("| s toSearch from | Search a data from a node |toSearch: Chord id to search|\n");
	printf("|                 |                           |from: Chord id searching    |\n");
	printf("|--------------------------------------------------------------------------|\n");
	printf("| h               | Diplay this help          | None                       |\n");
	printf("|--------------------------------------------------------------------------|\n");
	printf("| q               | Quit, kill everyone       | None                       |\n");
	printf("+--------------------------------------------------------------------------+\n");
	printf("\n");
}

void routine(int nb_node)
{
	int to_send[2];

	printf("\n*** Random search from MPI 1***\n");
	to_send[0] = 0;
	to_send[1] = 1;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(1);
	to_send[0] = 25;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(1);
	to_send[0] = 44;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(1);
	to_send[0] = 62;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(2);

	printf("\n*** Random search **from MPI 2***\n");
	to_send[0] = 0;
	to_send[1] = 2;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(1);
	to_send[0] = 25;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(1);
	to_send[0] = 44;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(1);
	to_send[0] = 62;
	MPI_Send(&to_send, 2, MPI_INT, to_send[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(2);
	
	printf("\n*** Kill everyone *** \n");
	for (int i = 1; i <= nb_node; ++i)
		MPI_Send(&to_send, 1, MPI_INT, i, TAG_STOP, MPI_COMM_WORLD);
}

void prompt(int nb_nodes, int *chord_ids)
{
	char input[16];
	int to_send[2], identificator, origin_chord_id, mpi;
	char command;
	
	display_help();
	while(1)
	{
		fgets(input, 16, stdin);
		if(input[0] == 'q')
		{
			for (int i = 1; i <= nb_nodes; ++i)
				MPI_Send(&to_send, 1, MPI_INT, i, TAG_STOP, MPI_COMM_WORLD);
			break;
		}
		else if(input[0] == 's')
		{
			sscanf(input, "%c %d %d", &command, &identificator, &origin_chord_id);
			to_send[0] = identificator;
			mpi = find_in_array(chord_ids, nb_nodes, origin_chord_id);
			to_send[1] = mpi+1;
			if(mpi == -1)
				puts("Chord ID not found");
			else
				MPI_Send(&to_send, 2, MPI_INT, (mpi+1), TAG_ASK, MPI_COMM_WORLD);
		}
		else if(input[0] == 'h')
			display_help();
		else
			puts("Wrong input");
		
	}
}

/*
	Code du simulateur (MPI ID = 0)
*/

void simulateur(int nb_nodes)
{
	int to_send[2];
	//Chaque entrée de la fingerTable a 3 ints
	int *fingerTable = malloc(sizeof(int)*3*K);
	int *chord_ids = malloc(sizeof(int)*nb_nodes);

	memset(fingerTable, 0, sizeof(int)*3*K);

	generate_node_ids(chord_ids, nb_nodes);

	for (int i = 0; i < nb_nodes; ++i)
	{
		generate_finger_table(fingerTable, K ,chord_ids, nb_nodes, i);
		display_finger_table(fingerTable, K, chord_ids[i]);
		to_send[0] = chord_ids[i];
		to_send[1] = chord_ids[PRECEDENT(i, nb_nodes)] + 1;
		MPI_Send(&to_send, 2, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
		MPI_Send(fingerTable, 3*K, MPI_INT, i+1, TAG_INIT_FINGER, MPI_COMM_WORLD);
	}

	/*********
	 * Mettre en commentaire une des deux lignes suivantes  
	 * pour changer en mode interractif / automatique
	 ********/

	//prompt(nb_nodes, chord_ids);
	routine(nb_nodes);

	free(fingerTable);
	free(chord_ids);
}
/* FIN CODE DU SIMULATEUR */

/* CODE DES NOEUDS */
//tofind appatient a ]j;id] 
int belongDHT(int j, int id, int tofind)
{
	if(j < id)
		return ((tofind > j) && (tofind <= id));
	else
		return((tofind > j) || (tofind <= id));
}

int checkData(const int chord_ids, const int first_data, const int to_check)
{
	if (chord_ids >= first_data)
		return (to_check <= chord_ids && to_check >= first_data);
	else
		return (to_check >= first_data || to_check <= chord_ids);
}

void process_TAG_ASK(struct finger *fingerTable, int chord_id, MPI_Status status)
{
	int to_recv[2];
	int foundDHT = 0;
	MPI_Recv(&to_recv, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
	if(status.MPI_SOURCE == 0)
		printf("Searching node %d from %d\n", to_recv[0], chord_id);
	for(int i = K-1; i >= 0; --i)
	{
		if(belongDHT(fingerTable[i].chord_id, chord_id, to_recv[0]))
		{
			if(fingerTable[i].chord_id == chord_id)
				continue;
			
			foundDHT = 1;
			printf("chord_ids[%d]: %d in the DHT at %d, send TAG_ASK at chord[%d]\n",chord_id, to_recv[0], i, fingerTable[i].chord_id);
			MPI_Send(&to_recv, 2, MPI_INT, fingerTable[i].mpi_id, TAG_ASK, MPI_COMM_WORLD);
			break;
		}
	}
	if(!foundDHT)
	{
		printf("chord_ids[%d]: %d NOT in the DHT, send TAG_LAST_CHANCE at chord[%d]\n",chord_id, to_recv[0], fingerTable[0].chord_id);
		MPI_Send(&to_recv, 2, MPI_INT, fingerTable[0].mpi_id, TAG_LAST_CHANCE, MPI_COMM_WORLD);
	}
	foundDHT = 0;
}

void node(int rank)
{
	struct finger fingerTable[K];
	int buff[K*3], to_recv[2], to_send[2];
	int run = 1, chord_id, first_data;
	MPI_Status status;

	MPI_Recv(&to_recv, 2, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);
	chord_id = to_recv[0];
	first_data = to_recv[1];
	MPI_Recv(&buff, K*3, MPI_INT, 0, TAG_INIT_FINGER, MPI_COMM_WORLD, &status);

	//printf("chord_ids[%d] mpi_rank[%d] first_data: %d\n", chord_id, rank, first_data);

	for(int i = 0; i < K; ++i)
	{
		fingerTable[i].value = buff[i*3];
		fingerTable[i].chord_id = buff[i*3+1]; 
		fingerTable[i].mpi_id = buff[i*3+2]; 
	}
	while(run)
	{
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG)
		{
			case TAG_ASK:
				process_TAG_ASK(fingerTable, chord_id ,status);
				break;

			case TAG_LAST_CHANCE:
				MPI_Recv(&to_recv, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
				if(checkData(chord_id, first_data, to_recv[0]))
				{
					to_send[0] = to_recv[0];
					to_send[1] = chord_id;
					MPI_Send(&to_send, 2, MPI_INT, to_recv[1], TAG_ANSWER, MPI_COMM_WORLD);
				}
				else
					printf("chord_ids[%d]: requested data %d is unfound\n", chord_id, to_recv[0]);
				break;

			case TAG_ANSWER:
				MPI_Recv(&to_recv, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
				printf("chord_ids[%d]: requested data %d is handled by %d\n", chord_id, to_recv[0], to_recv[1]);
				break;

			case TAG_STOP:
				run = 0;
				break;

			default:
				printf("Error, tag unknown: %d\n", status.MPI_TAG);
		}
	}
	printf("chord_id[%d]/mpi_rank[%d] Killed\n", chord_id, rank);
}
/* FIN CODE DES NOEUDS */

int main (int argc, char* argv[]) {

   int nb_proc,rank;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (nb_proc < 3 || nb_proc > M) {
   	  printf("Le nombre de processus MPI doit être compris entre %d et %d\n", 3, M);
      MPI_Finalize();
      exit(2);
   }
  
   if (rank == 0) 
      simulateur(nb_proc-1);
   else 
      node(rank);
  
   MPI_Finalize();
   return 0;
}