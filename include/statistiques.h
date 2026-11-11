#ifndef STATISTIQUES_H
#define STATISTIQUES_H
#include "types.h"

typedef struct
{
    Demande demande;
    int ascenseur;
    long attente;
    long duree;
} EnregistrementStatistique;

typedef struct
{
    int demandes_terminees;
    int demandes_reaffectees;
    long attente_totale;
    long duree_totale;
    unsigned long trajets[NB_ASCENSEURS];
    EnregistrementStatistique enregistrements[CAPACITE_FILE];
    int nombre_enregistrements;
} BilanSimulation;

void ajouter_statistique(BilanSimulation *bilan, const Demande *demande,
                         int ascenseur, long attente, long duree);
int sauvegarder_statistiques(const char *chemin, const BilanSimulation *bilan);
void afficher_bilan(const BilanSimulation *bilan, EtatPartage *partage,
                    const char *chemin);

#endif
