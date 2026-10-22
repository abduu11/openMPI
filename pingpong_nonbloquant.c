/*
TP MPI Suite - Question 2 : ping-pong, version non bloquante

Le processus 0 envoie "ping" au processus 1 avec MPI_Isend et lance la
reception du pong avec MPI_Irecv (les deux rendent la main immediatement).
Il verifie ensuite toutes les secondes, avec MPI_Test, si le pong est arrive, et
affiche le resultat du test. Le processus 1 attend un certain temps avant de
repondre "pong".

Variante demandee a la fin de la question : ne verifier que pendant les N
premieres secondes (2e argument), puis bloquer le processus 0 dans l'attente
du pong avec MPI_Wait.

Arguments (facultatifs) :
  delai      : delai du processus 1 avant de repondre, en secondes (5 par defaut)
  tests_max  : nombre maximal de tests, un par seconde (0 = tester jusqu'a
               l'arrivee du pong ; 2 = variante "2 secondes puis attente bloquante")

Compilation : mpicc pingpong_nonbloquant.c -o runnable/pingpong_nonbloquant
Execution   : mpirun -np 2 ./runnable/pingpong_nonbloquant           (tests sans limite)
              mpirun -np 2 ./runnable/pingpong_nonbloquant 5 2       (2 tests puis MPI_Wait)
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
    int tests_max = (argc > 2) ? atoi(argv[2]) : 0;      /* 0 = pas de limite */

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
        MPI_Request req_envoi, req_reception;
        int arrive = 0;
        int tests = 0;

        LOG(rang, "envoi du message \"%s\" (MPI_Isend)", ping);
        MPI_Isend(ping, 5, MPI_CHAR, 1, tag, MPI_COMM_WORLD, &req_envoi);
        MPI_Irecv(recu, 10, MPI_CHAR, 1, tag, MPI_COMM_WORLD, &req_reception);
        LOG(rang, "reception du pong lancee (MPI_Irecv), je continue sans attendre");

        // on verifie toutes les secondes si le pong est arrive
        while (!arrive && (tests_max == 0 || tests < tests_max)) {
            sleep(1);                                    /* travail simule */
            MPI_Test(&req_reception, &arrive, &status);
            tests++;
            LOG(rang, "pong arrive ? %s", arrive ? "oui" : "non");
        }

        // variante : apres tests_max secondes, on attend de facon bloquante
        if (!arrive) {
            LOG(rang, "fin des tests, attente bloquante du pong (MPI_Wait)");
            MPI_Wait(&req_reception, &status);
        }
        LOG(rang, "message \"%s\" recu", recu);
        MPI_Wait(&req_envoi, MPI_STATUS_IGNORE);
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
