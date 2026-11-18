/* memoire.h - Structures et constantes pour le simulateur de mémoire virtuelle */

#ifndef MEMOIRE_H
#define MEMOIRE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------------- Paramètres du système ---------------- */

/* Taille d'une page : 2^10 = 1024 octets */
#define TAILLE_PAGE (1 << 10)

/* Adresses virtuelles codées sur 16 bits pour simplifier */
#define NB_BITS_ADR 16

/* Nombre de pages virtuelles */
#define NB_PAGES (1 << (NB_BITS_ADR - 10)) /* 2^(16-10) = 64 */

/* Nombre de cadres physiques */
#define NB_CADRES 8

/* Taille du TLB */
#define TAILLE_TLB 4

/* Nombre max de requêtes à simuler */
#define NB_REQ_MAX 1000

/* ---------------- Structures ---------------- */

typedef struct
{
    uint16_t adresse_virtuelle;
    unsigned long instant;
} RequeteMemoire;

typedef struct
{
    int page;
    int cadre;
    int valide;             /* 0 = invalide, 1 = valide */
    unsigned long last_use; /* pour FIFO approx ou debug */
} TLBEntry;

typedef struct
{
    int cadre;
    int valide;
    unsigned long last_use;
} PageTableEntry;

typedef struct
{
    TLBEntry tlb[TAILLE_TLB];
    PageTableEntry tp[NB_PAGES];

    int mem_physique[NB_CADRES][TAILLE_PAGE];

    int nb_cadres_utilises;
    unsigned long temps;

    unsigned long nb_acces;
    unsigned long nb_tlb_hits;
    unsigned long nb_defauts_page;
} SystemeMemoire;

/* ---------------- Fonctions à implémenter dans le TP ---------------- */

/* 1) Numéro de page à partir de l’adresse virtuelle */
int calculer_numero_page(uint16_t adr_virt);

/* 2) Offset dans la page */
int calculer_offset(uint16_t adr_virt);

/* 3) Construction de l’adresse physique à partir du cadre et de l’offset */
uint16_t construire_adresse_physique(int cadre, int offset);

/* 4) Recherche dans le TLB : renvoie 1 si hit (cadre dans *cadre_out), 0 sinon */
int rechercher_tlb(SystemeMemoire *sys, int page, int *cadre_out);

/* 5) Recherche dans la table des pages : 1 si présente (cadre dans *cadre_out), 0 sinon */
int rechercher_table_pages(SystemeMemoire *sys, int page, int *cadre_out);

/* 6) Charge une page dans le premier cadre libre, renvoie le numéro de cadre */
int ajouter_page_en_memoire(SystemeMemoire *sys, int page);

/* 7) Mise à jour du TLB avec politique FIFO approximative */
void maj_tlb_fifo(SystemeMemoire *sys, int page, int cadre);

/* Outils */
void initialiser_systeme(SystemeMemoire *sys);
void afficher_stats(SystemeMemoire *sys);

#endif /* MEMOIRE_H */
