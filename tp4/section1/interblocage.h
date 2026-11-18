#ifndef INTERBLOCAGE_H
#define INTERBLOCAGE_H

#include <pthread.h>

typedef struct
{
    pthread_mutex_t A;
    pthread_mutex_t B;
    pthread_mutex_t C;
} Ressources;

/* Scénario 1 : deux threads, deux verrous, ordre d'acquisition inversé
 * (A puis B / B puis A) -> interblocage possible. Fourni tel quel : ce
 * scénario sert d'exemple à analyser, pas à corriger sur place. */
void scenario1(Ressources *r);

/* Scénario 2 : trois threads, trois verrous, cycle A->B, B->C, C->A ->
 * interblocage possible. Fourni tel quel, à analyser. */
void scenario2(Ressources *r);

/* Scénario corrigé : à vous de l'implémenter (voir l'énoncé, Section 1) de
 * façon à éliminer l'interblocage observé dans scenario1/scenario2, par
 * exemple en imposant un ordre total sur les verrous. */
void scenario_corrige(Ressources *r);

#endif
