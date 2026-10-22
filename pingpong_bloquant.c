/*
TP MPI Suite - Question 2 : ping-pong, version bloquante

Le processus 0 envoie "ping" au processus 1 (MPI_Send), puis attend la reponse
(MPI_Recv). Le processus 1 attend un certain temps avant de repondre "pong".
Chaque affichage porte le rang du processus et un horodatage (secondes depuis
le debut du programme).

Argument (facultatif) : delai du processus 1 avant de repondre, en secondes
(5 par defaut).

Compilation : mpicc pingpong_bloquant.c -o runnable/pingpong_bloquant
Execution   : mpirun -np 2 ./runnable/pingpong_bloquant [delai]
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

static double t_debut;

// affichage horodate avec vidage immediat du tampon
#define LOG(rang, ...) do { \
    printf("[t=%6.3f s] processus %d : ", MPI_Wtime() - t_debut, rang); \
    printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while (0)

int main(int argc, char *argv[]) {
    int rang, p;
    int tag = 0;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int delai = (argc > 1) ? atoi(argv[1]) : 5;

    if (p != 2) {
        if (rang == 0) printf("Ce programme necessite exactement 2 processus.\n");
        MPI_Finalize();
        return 0;
    }

    char ping[5] = "ping";
    char pong[5] = "pong";
    char recu[10];

    MPI_Barrier(MPI_COMM_WORLD);       /* origine commune des temps */
    t_debut = MPI_Wtime();

    if (rang == 0) {
        LOG(rang, "envoi du message \"%s\"", ping);
        MPI_Send(ping, 5, MPI_CHAR, 1, tag, MPI_COMM_WORLD);
        LOG(rang, "ping envoye, attente du pong (MPI_Recv bloquant)");
        MPI_Recv(recu, 10, MPI_CHAR, 1, tag, MPI_COMM_WORLD, &status);
        LOG(rang, "message \"%s\" recu", recu);
    } else {
        MPI_Recv(recu, 10, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
        LOG(rang, "message \"%s\" recu", recu);
        LOG(rang, "attente de %d seconde(s) avant de repondre", delai);
        sleep(delai);
        LOG(rang, "envoi du message \"%s\"", pong);
        MPI_Send(pong, 5, MPI_CHAR, 0, tag, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
