#include "ordonnancement.h"
#include <stdio.h>
#include <string.h>

/* Helper fourni : affiche la liste des tâches. Ne pas modifier. */
static void afficher_taches(const char *nom, Tache *t, int n)
{
    printf("=== %s ===\n", nom);
    for (int i = 0; i < n; ++i)
    {
        printf("T%d: duree=%d, type=%d, periode=%d\n",
               t[i].id, t[i].duree, t[i].type, t[i].periode);
    }
}

/* TODO */
void fcfs(Tache *t, int n)
{
    (void)t;
    (void)n;
}

/* TODO */
void sjf(Tache *t, int n)
{
    (void)t;
    (void)n;
}

/* TODO*/
void round_robin(Tache *t, int n, int q)
{
    (void)t;
    (void)n;
    (void)q;
}

/* TODO */
void round_robin_periodique(Tache *t, int n, int quantum, int duree_max)
{
    (void)t;
    (void)n;
    (void)quantum;
    (void)duree_max;
}

/* ---------------- main de démonstration (fourni, NE PAS MODIFIER) ---------------- */

int main(void)
{
    Tache t[4] = {
        {0, 5, 1, 0, TACHE_INTERACTIVE},
        {1, 12, 2, 0, TACHE_CPU},
        {2, 3, 1, 10, TACHE_PERIODIQUE},
        {3, 7, 3, 0, TACHE_CPU}};

    afficher_taches("Tâches initiales", t, 4);

    fcfs(t, 4);
    sjf(t, 4);
    round_robin(t, 4, 4);
    round_robin_periodique(t, 4, 4, 40);

    return 0;
}
