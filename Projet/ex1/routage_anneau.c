#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define M 64
#define TAG_INIT 0
#define TAG_ASK 1
#define TAG_ANSWER 2
#define TAG_DISCONNECT 3
#define TAG_DISCONNECT_ACK 4
#define TAG_CONNECTED_NOTIFY_SUCCESSOR 5
#define TAG_CONNECTED_NOTIFY_PREDESSESSOR 6
#define TAG_CONNECTED_ACK 7
#define TAG_STOP 8

#define PRECEDENT(i, count) ((i+(count)-1)%(count))
#define SUIVANT(i, count) ((i+(count)+1)%(count))


struct nodeIds {
	int *mpi_ids;
	int *chord_ids;
	int nb_node;
};

/* CODE DU SIMULATEUR */

/*
	Fonction pour quicksort sur un tableau d'entier
*/
static int cmpint(const void *a, const void *b)
{
	if(*(int*)a > *(int*)b)
		return 1;
	if(*(int*)a < *(int*)b)
		return -1;
	return 0;
}


/*
	Vérifie si un entier to_ckeck est présent dans un tableau d'entier
	return: 1 s'il est présent, 0 sinon
*/
int array_contains(const int *tab, const int tab_size, const int to_ckeck)
{
	for (int i = 0; i < tab_size; ++i)
	{
		if(tab[i] == to_ckeck)
			return 1;
	}
	return 0;
}

int getUniqueId(int *tab, int size)
{
	int random;
	do
	{
		random = rand()%M;
	}
	while(array_contains(tab, size, random));
	return random;
}

/*
	Génération des clés CHORD dans l'ensemble [0;M[, 
	les clés sont tirées aléatoirement et de manière unique 
*/
void generate_node_ids(int *tab, int nb_node)
{
	srand(time(NULL));
	for (int i = 0; i<nb_node; ++i)
	{
		tab[i] = getUniqueId(tab, nb_node);
	}

	qsort(tab, nb_node, sizeof(int),cmpint);
	printf("[%d |", tab[0]);
	for(int i = 1; i < nb_node-1; i++)
	{
		printf(" %d |", tab[i]);
	}
	printf(" %d]", tab[nb_node-1]);
	printf("\n\n");
}


void printRing(struct nodeIds *correspond)
{
	printf("[ | ");
	for(int i = 0; i < correspond->nb_node; i++)
	{
		if(correspond->chord_ids[i] != -1)
			printf("%d | ", correspond->chord_ids[i]);
	}
	printf("]\n");
}

/*
	Correspondance entre une clé CHORD (chord_ids) et un identifiant MPI
	return: l'identifiant MPI de la clé CHORD, 
			ou -1 si la clé n'existe pas
*/
int chordToMpi(struct nodeIds *correspond, int chord_ids)
{
	for (int i = 0; i < correspond->nb_node; ++i)
	{
		if(correspond->chord_ids[i] == chord_ids)
			return correspond->mpi_ids[i];
	}
	return -1;
}

/*
	Retire correspondance entre une clé CHORD (chord_ids) et un identifiant MPI
	return: l'identifiant MPI de la clé CHORD retirée, 
			ou -1 si la clé n'existe pas
*/
int removeChordID(struct nodeIds *correspond, int chord_ids)
{
	for (int i = 0; i < correspond->nb_node; ++i)
	{
		if(correspond->chord_ids[i] == chord_ids)
		{
			correspond->chord_ids[i] = -1;
			return correspond->mpi_ids[i];
		}
	}
	return -1;
}

int addChordID(struct nodeIds *correspond, int mpi_id, int new_chord_id)
{
	for (int i = 0; i < correspond->nb_node; ++i)
	{
		if(correspond->mpi_ids[i] == mpi_id)
		{
			if(correspond->chord_ids[i] != -1)
				return -1;

			correspond->chord_ids[i] = new_chord_id;
			return 0;
		}
	}
	return -1;
}

int MPInotConnected(struct nodeIds *correspond, int mpi_ids)
{
	for (int i = 0; i < correspond->nb_node; ++i)
	{
		if(correspond->mpi_ids[i] == mpi_ids)
		{	
			if(correspond->chord_ids[i] == -1)
				return 1;
			else
				return 0;
		}
	}
	return 0;
}



int getRandomMPIOfConnectedPeer(struct nodeIds *correspond)
{
	int random;
	//On cherche un processus MPI deja connecté en tant que pair
	do
	{
		random = rand()%correspond->nb_node;
	}
	while(correspond->chord_ids[random] == -1);

	return correspond->mpi_ids[random]; 
}

void simulateur(int nb_node) 
{
	/*Generation des CHORD id */
	//int *tab = malloc(sizeof(int)*(nb_node));
	int mpi_next, first_data, data[2];
	struct nodeIds correspond;
	char input[16];
	char command;
	int identificator, origin_chord_ids;

	correspond.nb_node = nb_node;
	correspond.mpi_ids = malloc(sizeof(int)*(nb_node));
	correspond.chord_ids = malloc(sizeof(int)*(nb_node));
	
	memset(correspond.mpi_ids, -1, sizeof(int)*(nb_node));
	memset(correspond.chord_ids, -1, sizeof(int)*(nb_node));

	generate_node_ids(correspond.chord_ids, nb_node);

	for (int i = 0; i < nb_node; ++i)
	{
		correspond.mpi_ids[i] = i+1;
		MPI_Send(&correspond.chord_ids[i], 1, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
		first_data = correspond.chord_ids[PRECEDENT(i, nb_node)] + 1;
		MPI_Send(&first_data, 1, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
		MPI_Send(&correspond.chord_ids[SUIVANT(i, nb_node)], 1, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
		mpi_next = SUIVANT(i, nb_node) + 1;
		MPI_Send(&mpi_next, 1, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
	}

	while(1)
	{
		fgets(input, 16, stdin);
		if(input[0] == 'q')
		{
			for (int i = 1; i <= nb_node; ++i)
				MPI_Send(&data, 1, MPI_INT, i, TAG_STOP, MPI_COMM_WORLD);
			break;
		}
		else if(input[0] == 's')
		{
			sscanf(input, "%c %d %d", &command, &identificator, &origin_chord_ids);
			data[0] = identificator;
			data[1] = chordToMpi(&correspond, origin_chord_ids);
			if(data[1] == -1)
				puts("Chord ID not found");
			else
				MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);
		}
		else if(input[0] == 'd') {
			sscanf(input, "%c %d", &command, &identificator);
			if( (identificator = removeChordID(&correspond, identificator)) != -1)
				MPI_Send(&data, 1, MPI_INT, identificator, TAG_DISCONNECT, MPI_COMM_WORLD);
			else
				puts("Chord ID not found");

		}
		else if(input[0] == 'c') 
		{
			sscanf(input, "%c %d", &command, &identificator);
			data[0] = getUniqueId(correspond.chord_ids, nb_node); // Nouveau CHORD ID du nouveau pair
			data[1] = identificator; // Le Rank MPI du pair se connectant

			if(addChordID(&correspond, data[1], data[0]) == 0)
			{
				printf("New chord_id[%d] mpi_rank[%d] connecting ...\n", data[0], data[1]);
				MPI_Send(&data, 2, MPI_INT, getRandomMPIOfConnectedPeer(&correspond), TAG_CONNECTED_NOTIFY_SUCCESSOR, MPI_COMM_WORLD);
			}
			else
				puts("Wrong MPI_rank given (already connected or non-existing");
		}
		else if(input[0] == 'p')
		{
			printRing(&correspond);
		}
		else
			puts("Wrong input");
		
	}
	/*
	data[1] = 2;
	data[0] = 45;
	MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);
	data[0] = 10;
	MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);
	data[0] = 64;
	MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);
	sleep(2);

	MPI_Send(&data, 1, MPI_INT, 1, TAG_DISCONNECT, MPI_COMM_WORLD);

	sleep(2);
	data[0] = 45;
	MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);
	data[0] = 10;
	MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);
	data[0] = 64;
	MPI_Send(&data, 2, MPI_INT, data[1], TAG_ASK, MPI_COMM_WORLD);

	sleep(2);
	for (int i = 1; i <= nb_node; ++i)
		MPI_Send(&data, 1, MPI_INT, i, TAG_STOP, MPI_COMM_WORLD);
	*/

	free(correspond.mpi_ids);
	free(correspond.chord_ids);
}

/* FIN CODE DU SIMULATEUR */

/* CODE DES NOEUDS */
int checkData(const int chord_ids, const int first_data, const int to_check)
{
	if (chord_ids >= first_data)
		return (to_check <= chord_ids && to_check >= first_data);
	else
		return (to_check >= first_data || to_check <= chord_ids);
}

void disconnect(MPI_Status status, int *chord_ids, int *first_data, int *chord_next_node, int *mpi_next_node)
{
	int to_recv[4], to_send[4];
	if(status.MPI_SOURCE == 0) //Si c'est proc maitre qui nous demande de nous deconnecter
	{
		MPI_Recv(&to_recv, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
		to_send[0] = *chord_ids;
		to_send[1] = *first_data;
		to_send[2] = *chord_next_node;
		to_send[3] = *mpi_next_node;
		MPI_Send(&to_send, 4, MPI_INT, *mpi_next_node, TAG_DISCONNECT, MPI_COMM_WORLD);
	}
	else
	{
		MPI_Recv(&to_recv, 4, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
		//Si c'est le pair qui me precede qui se deco
		if(to_recv[0] == (*first_data-1))
			*first_data = to_recv[1];
		//Si c'est le pair qui me devance qui se deco
		if(to_recv[0] == *chord_next_node)
		{
			int save_old_mpi_node = *mpi_next_node; 
			*chord_next_node = to_recv[2];
			*mpi_next_node = to_recv[3];
			MPI_Send(&to_send, 1, MPI_INT, save_old_mpi_node, TAG_DISCONNECT_ACK, MPI_COMM_WORLD);
		}
		else
		{
			MPI_Send(&to_recv, 4, MPI_INT, *mpi_next_node, TAG_DISCONNECT, MPI_COMM_WORLD);
		}
	}
}

void connect_notify_predessessor(MPI_Status status, int *chord_id, int *chord_next_node, int *mpi_next_node)
{
	int to_recv[4], to_send[4];
	MPI_Recv(&to_recv, 4, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
	if(*chord_next_node == to_recv[2])
	{
		*mpi_next_node = to_recv[1];
		*chord_next_node = to_recv[0];

		printf("chord_id[%d]: new chord_next_node:[%d], mpi_next_node[%d]\n", *chord_id, *chord_next_node, *mpi_next_node);

		//Envoie des inforation au nouveau pair
		to_send[0] = to_recv[0];	  // Nouveau CHORD ID du nouveau pair
		to_send[1] = to_recv[3];	  // mpi_next_node
		to_send[2] = to_recv[2]; 	  // chord_next_node
		to_send[3] = (*chord_id) + 1; // first_data
		MPI_Send(&to_send, 4, MPI_INT, to_recv[1], TAG_CONNECTED_ACK, MPI_COMM_WORLD);		
	}
	else
		MPI_Send(&to_recv, 4, MPI_INT, *mpi_next_node, TAG_CONNECTED_NOTIFY_PREDESSESSOR, MPI_COMM_WORLD);
}

void connect_notify_successor(MPI_Status status, int *chord_id, int rank, int *first_data, int *mpi_next_node)
{
	int to_recv[2], to_send[4];
	MPI_Recv(&to_recv, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
	if(checkData(*chord_id, *first_data, to_recv[0]))
	{
		*first_data = SUIVANT(to_recv[0],M);
		printf("chord_id[%d]: rangeDown because of new_chord_id[%d], new first_data: %d\n", *chord_id, to_recv[0], *first_data);


		//Envoie des inforation a l'anneau à la recherche du predecesseur
		to_send[0] = to_recv[0]; // Nouveau CHORD ID du nouveau pair
		to_send[1] = to_recv[1]; // Le Rank MPI du pair se connectant
		to_send[2] = *chord_id;	 // Le CHORD ID du successeur du nouveau pair
		to_send[3] = rank; 		 // Le MPI ID du sucesseur du nouveau pair
		MPI_Send(&to_send, 4, MPI_INT, *mpi_next_node, TAG_CONNECTED_NOTIFY_PREDESSESSOR, MPI_COMM_WORLD);		
	}
	else
		MPI_Send(&to_recv, 2, MPI_INT, *mpi_next_node, TAG_CONNECTED_NOTIFY_SUCCESSOR, MPI_COMM_WORLD);

}

void node(int rank) 
{
	int chord_ids, first_data, chord_next_node, mpi_next_node;
	int to_recv[4], to_send[4];
	int run = 1;
	MPI_Status status;

	MPI_Recv(&chord_ids, 1, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&first_data, 1, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&chord_next_node, 1, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&mpi_next_node, 1, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);

	printf("chord_ids[%d] mpi_rank[%d] first_data: %d chord_next_node: %d mpi_next_node: %d\n", chord_ids, rank, first_data, chord_next_node, mpi_next_node);

	while(run)
	{
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG)
		{
			case TAG_ASK:
				MPI_Recv(&to_recv, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
				if(status.MPI_SOURCE == 0)
					printf("Searching node %d from %d\n", to_recv[0], chord_ids);

				if(checkData(chord_ids, first_data, to_recv[0]))
				{
					to_send[0] = to_recv[0];
					to_send[1] = chord_ids;
					MPI_Send(&to_send, 2, MPI_INT, to_recv[1], TAG_ANSWER, MPI_COMM_WORLD);
				}
				else
					MPI_Send(&to_recv, 2, MPI_INT, mpi_next_node, TAG_ASK, MPI_COMM_WORLD);
				break;

			case TAG_ANSWER:
					MPI_Recv(&to_recv, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
					printf("chord_ids[%d]: requested data %d is handled by %d\n", chord_ids, to_recv[0], to_recv[1]);
				break;

			case TAG_DISCONNECT:
				disconnect(status, &chord_ids, &first_data, &chord_next_node, &mpi_next_node);
				break;

			case TAG_DISCONNECT_ACK:
					MPI_Recv(&to_recv, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
					printf("old_chord_ids[%d] mpi_rank[%d] properly disconnected\n", chord_ids, rank);
					chord_ids = -1;
				break;
			case TAG_CONNECTED_NOTIFY_SUCCESSOR:	
				connect_notify_successor(status, &chord_ids, rank, &first_data, &mpi_next_node);
				break;

			case TAG_CONNECTED_NOTIFY_PREDESSESSOR:	
				connect_notify_predessessor(status, &chord_ids, &chord_next_node, &mpi_next_node);
				break;

			case TAG_CONNECTED_ACK:	
				MPI_Recv(&to_recv, 4, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
				chord_ids = to_recv[0]; // Nouveau CHORD ID du nouveau pair
				mpi_next_node = to_recv[1]; // mpi_next_node
				chord_next_node = to_recv[2]; // chord_next_node
				first_data = to_recv[3]; 	 // first_data
				printf("chord_id[%d] mpi_rank[%d] properly connected ! mpi_next_node = %d, chord_next_node = %d, first_data = %d\n", chord_ids, rank, mpi_next_node, chord_next_node, first_data);
				break;

			case TAG_STOP:
				run = 0;
				break;

			default:
				printf("Error, tag unknown: %d\n", status.MPI_TAG);
		}
	}
	printf("chord_ids[%d]/mpi_rank[%d] Killed\n", chord_ids, rank);
}

/* FIN CODE DES NOEUDS */



int main (int argc, char* argv[]) {

   int nb_proc,rang;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

   if (nb_proc < 3 || nb_proc > M) {
      printf("Nombre de processus incorrect !\n");
      MPI_Finalize();
      exit(2);
   }
  
   MPI_Comm_rank(MPI_COMM_WORLD, &rang);
  
   if (rang == 0) {
      simulateur(nb_proc-1);
   } else {
      node(rang);
   }
  
   MPI_Finalize();
   return 0;
}