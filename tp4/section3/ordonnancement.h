#ifndef ORDONNANCEMENT_H
#define ORDONNANCEMENT_H

#define MAX_TACHES 32
#define MAX_INSTANCES 512

typedef enum
{
    TACHE_INTERACTIVE,
    TACHE_CPU,
    TACHE_PERIODIQUE
} TypeTache;

typedef struct
{
    int id;
    int duree;
    int priorite;
    int periode; /* utilisé seulement si type == TACHE_PERIODIQUE */
    TypeTache type;
} Tache;

void fcfs(Tache *taches, int n);
void sjf(Tache *taches, int n);
void round_robin(Tache *taches, int n, int quantum);

/* Round Robin qui relance automatiquement chaque tâche périodique à chacune
 * de ses échéances (t = periode, 2*periode, ...) jusqu'à duree_max, et
 * rapporte le nombre d'échéances ratées. */
void round_robin_periodique(Tache *taches, int n, int quantum, int duree_max);

#endif
