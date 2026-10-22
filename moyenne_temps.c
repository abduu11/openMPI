/*
TP MPI Suite - Question 1 : moyenne parallele chronometree

Le processus 0 genere un grand tableau d'entiers aleatoires et le distribue a
tous les processus (MPI_Scatterv : fonctionne quel que soit le nombre de
processus, y compris 1 seul, meme si N n'est pas divisible par p).
Chaque processus calcule la moyenne de son bloc, puis les moyennes locales sont
collectees par le processus 0 (MPI_Gather) qui calcule la moyenne globale
(moyenne ponderee par la taille des blocs).

Charge de calcul : la moyenne d'un tableau est tres rapide (O(N)). Pour que le
calcul sur un seul processus dure quelques secondes, chaque processus parcourt
son bloc "passes" fois (le resultat est inchange : somme totale / nombre
d'elements parcourus).

Pour aller plus loin : la somme du tableau est aussi calculee avec MPI_Reduce.

Arguments (facultatifs) : N (taille du tableau), passes (repetitions du calcul)
Compilation : mpicc -O2 moyenne_temps.c -o runnable/moyenne_temps
Execution   : mpirun -np 4 ./runnable/moyenne_temps 50000000 100
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rang, p;
    int *tableau = NULL;
    long n_long = (argc > 1) ? atol(argv[1]) : 50000000L;
    int passes = (argc > 2) ? atoi(argv[2]) : 100;
    int n = (int)n_long;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // taille du bloc de chaque processus : N / p, le reste est reparti sur les premiers
    int *comptes = malloc(p * sizeof(int));
    int *deplacements = malloc(p * sizeof(int));
    for (int i = 0, debut = 0; i < p; i++) {
        comptes[i] = n / p + (i < n % p ? 1 : 0);
        deplacements[i] = debut;
        debut += comptes[i];
    }
    int mon_compte = comptes[rang];
    int *bloc = malloc((size_t)mon_compte * sizeof(int));

    // generation du tableau par le processus 0 (hors mesure du temps de calcul)
    double temps_generation = 0;
    if (rang == 0) {
        double g0 = MPI_Wtime();
        tableau = malloc((size_t)n * sizeof(int));
        srand(12345);
        for (int i = 0; i < n; i++) tableau[i] = rand() % 1000;
        temps_generation = MPI_Wtime() - g0;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    // distribution du tableau
    MPI_Scatterv(tableau, comptes, deplacements, MPI_INT,
                 bloc, mon_compte, MPI_INT, 0, MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    // calcul de la moyenne locale (le barrier asm empeche le compilateur de fusionner les passes)
    long long somme_passes = 0;
    for (int k = 0; k < passes; k++) {
        long long s = 0;
        for (int i = 0; i < mon_compte; i++) s += bloc[i];
        somme_passes += s;
        __asm__ volatile("" ::: "memory");
    }
    double moyenne_locale = (mon_compte > 0)
        ? (double)somme_passes / ((double)passes * mon_compte) : 0.0;
    double t2 = MPI_Wtime();

    // collecte des moyennes locales
    double *moyennes = NULL;
    if (rang == 0) moyennes = malloc(p * sizeof(double));
    MPI_Gather(&moyenne_locale, 1, MPI_DOUBLE, moyennes, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double t3 = MPI_Wtime();

    // temps de distribution et de calcul : maximum sur tous les processus
    double distrib = t1 - t0, calcul = t2 - t1, distrib_max, calcul_max;
    MPI_Reduce(&distrib, &distrib_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&calcul, &calcul_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rang == 0) {
        // moyenne globale ponderee par la taille de chaque bloc
        double total = 0;
        for (int i = 0; i < p; i++) total += moyennes[i] * comptes[i];
        double moyenne_globale = total / n;
        printf("np=%d N=%d passes=%d | generation=%.3f s | distribution=%.3f s | "
               "calcul=%.3f s | total=%.3f s | moyenne=%.6f\n",
               p, n, passes, temps_generation, distrib_max, calcul_max, t3 - t0,
               moyenne_globale);
        fflush(stdout);
    }

    // ----- Pour aller plus loin : somme du tableau avec MPI_Reduce -----
    long long somme_locale = 0;
    for (int i = 0; i < mon_compte; i++) somme_locale += bloc[i];
    long long somme_totale = 0;
    MPI_Reduce(&somme_locale, &somme_totale, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rang == 0) {
        long long reference = 0;                       /* verification sequentielle */
        for (int i = 0; i < n; i++) reference += tableau[i];
        printf("MPI_Reduce : somme = %lld (reference sequentielle = %lld), moyenne = %.6f\n",
               somme_totale, reference, (double)somme_totale / n);
        free(tableau);
        free(moyennes);
    }

    free(comptes);
    free(deplacements);
    free(bloc);
    MPI_Finalize();
    return 0;
}
