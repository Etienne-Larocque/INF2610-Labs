/* memoire.c
 *
 * Gabarit pour la section 2 : gestion de mémoire virtuelle.
 * Complète les 7 fonctions marquées TODO ci-dessous. Le reste (structures,
 * initialisation, démonstration dans main()) est fourni et ne doit pas être
 * modifié.
 */

#include "memoire.h"

/* ---------------- Fonctions à compléter ---------------- */

/* TODO  */
int calculer_numero_page(uint16_t adr_virt)
{
    (void)adr_virt;
    return 0;
}

/* TODO*/
int calculer_offset(uint16_t adr_virt)
{
    (void)adr_virt;
    return 0;
}

/* TODO  */
uint16_t construire_adresse_physique(int cadre, int offset)
{
    (void)cadre;
    (void)offset;
    return 0;
}

/* TODO  */
int rechercher_tlb(SystemeMemoire *sys, int page, int *cadre_out)
{
    (void)sys;
    (void)page;
    (void)cadre_out;
    return 0;
}

/* TODO */
int rechercher_table_pages(SystemeMemoire *sys, int page, int *cadre_out)
{
    (void)sys;
    (void)page;
    (void)cadre_out;
    return 0;
}

/* TODO */
int ajouter_page_en_memoire(SystemeMemoire *sys, int page)
{
    (void)sys;
    (void)page;
    return 0;
}

/* TODO  */
void maj_tlb_fifo(SystemeMemoire *sys, int page, int cadre)
{
    (void)sys;
    (void)page;
    (void)cadre;
}

/* ---------------- Fonctions utilitaires (fournies, NE PAS MODIFIER) ---------------- */

void initialiser_systeme(SystemeMemoire *sys)
{
    sys->nb_cadres_utilises = 0;
    sys->temps = 0;
    sys->nb_acces = 0;
    sys->nb_tlb_hits = 0;
    sys->nb_defauts_page = 0;

    for (int i = 0; i < TAILLE_TLB; ++i)
    {
        sys->tlb[i].page = -1;
        sys->tlb[i].cadre = -1;
        sys->tlb[i].valide = 0;
        sys->tlb[i].last_use = 0;
    }

    for (int p = 0; p < NB_PAGES; ++p)
    {
        sys->tp[p].cadre = -1;
        sys->tp[p].valide = 0;
        sys->tp[p].last_use = 0;
    }

    for (int c = 0; c < NB_CADRES; ++c)
    {
        for (int i = 0; i < TAILLE_PAGE; ++i)
        {
            sys->mem_physique[c][i] = 0;
        }
    }
}

void afficher_stats(SystemeMemoire *sys)
{
    printf("\n===== Statistiques mémoire =====\n");
    printf("Nombre total d'accès    : %lu\n", sys->nb_acces);
    printf("TLB hits                 : %lu\n", sys->nb_tlb_hits);
    printf("Défauts de page          : %lu\n", sys->nb_defauts_page);

    if (sys->nb_acces > 0)
    {
        double taux_tlb = (double)sys->nb_tlb_hits / sys->nb_acces * 100.0;
        double taux_def = (double)sys->nb_defauts_page / sys->nb_acces * 100.0;
        printf("Taux de succès TLB       : %.2f %%\n", taux_tlb);
        printf("Taux de défauts de page  : %.2f %%\n", taux_def);
    }
}

/* ---------------- main de démonstration (fourni, NE PAS MODIFIER) ---------------- */

int main(void)
{
    SystemeMemoire sys;
    initialiser_systeme(&sys);

    const int nb_requetes = 20;
    uint16_t adresses[nb_requetes];

    for (int i = 0; i < nb_requetes; ++i)
    {
        if (i < 6)
        {
            adresses[i] = (uint16_t)(0x0123 + (i % 3) * TAILLE_PAGE);
        }
        else
        {
            int page_aleatoire = rand() % NB_CADRES;
            int offset_aleatoire = rand() % TAILLE_PAGE;
            adresses[i] = (uint16_t)(page_aleatoire * TAILLE_PAGE + offset_aleatoire);
        }
    }

    for (int i = 0; i < nb_requetes; ++i)
    {
        sys.temps++;
        sys.nb_acces++;

        uint16_t adr_virt = adresses[i];
        int page = calculer_numero_page(adr_virt);
        int offset = calculer_offset(adr_virt);
        int cadre = -1;
        int tlb_hit = 0;

        /* 1) TLB */
        if (rechercher_tlb(&sys, page, &cadre))
        {
            tlb_hit = 1;
            sys.nb_tlb_hits++;
        }
        else
        {
            /* 2) Table des pages */
            if (!rechercher_table_pages(&sys, page, &cadre))
            {
                /* 3) Défaut de page : chargement en mémoire */
                sys.nb_defauts_page++;
                cadre = ajouter_page_en_memoire(&sys, page);
            }
            /* 4) Mettre à jour le TLB */
            maj_tlb_fifo(&sys, page, cadre);
        }

        uint16_t adr_phys = construire_adresse_physique(cadre, offset);

        printf("Accès %2d : VA = 0x%04x (page=%2d, offset=%4d) -> PA = 0x%04x%s\n",
               i, adr_virt, page, offset, adr_phys,
               tlb_hit ? " [TLB]" : "");
    }

    afficher_stats(&sys);
    return 0;
}
