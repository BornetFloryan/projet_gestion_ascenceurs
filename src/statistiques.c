/* Conservation des statistiques en memoire puis ecriture du CSV en fin de simulation. */
#include <stdio.h>
#include "statistiques.h"
#include "synchronisation.h"
#include "immeuble.h"

void ajouter_statistique(BilanSimulation *bilan, const Demande *demande,
                         int ascenseur, long attente, long duree)
{
    if (bilan->nombre_enregistrements >= CAPACITE_FILE)
        return;
    EnregistrementStatistique *enregistrement =
        &bilan->enregistrements[bilan->nombre_enregistrements++];
    enregistrement->demande = *demande;
    enregistrement->ascenseur = ascenseur;
    enregistrement->attente = attente;
    enregistrement->duree = duree;
}

int sauvegarder_statistiques(const char *chemin, const BilanSimulation *bilan)
{
    FILE *fichier = fopen(chemin, "w");
    if (fichier == NULL)
        return -1;
    fprintf(fichier, "demande,priorite,ascenseur,depart,destination,attente_s,duree_s,tentatives\n");
    for (int i = 0; i < bilan->nombre_enregistrements; i++)
    {
        const EnregistrementStatistique *e = &bilan->enregistrements[i];
        fprintf(fichier, "%d,%d,%d,%d,%d,%ld,%ld,%d\n",
                e->demande.id, e->demande.priorite, e->ascenseur + 1,
                e->demande.etage_depart, e->demande.etage_arrivee,
                e->attente, e->duree, e->demande.tentatives);
    }
    int resultat = ferror(fichier) ? -1 : 0;
    if (fclose(fichier) != 0)
        resultat = -1;
    return resultat;
}

void afficher_bilan(const BilanSimulation *bilan, EtatPartage *partage,
                    const char *chemin)
{
    printf("\n=============== BILAN DE SIMULATION ===============\n");
    printf("Demandes terminees : %d\n", bilan->demandes_terminees);
    printf("Demandes reaffectees apres panne : %d\n", bilan->demandes_reaffectees);
    if (bilan->demandes_terminees > 0)
    {
        printf("Attente moyenne avant prise en charge : %.2f seconde(s)\n",
               (double)bilan->attente_totale / bilan->demandes_terminees);
        printf("Duree moyenne totale : %.2f seconde(s)\n",
               (double)bilan->duree_totale / bilan->demandes_terminees);
    }
    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        int etage;
        unsigned long trajets;
        verrouiller_etat(partage);
        etage = partage->ascenseurs[i].etage;
        trajets = partage->ascenseurs[i].trajets_termines;
        deverrouiller_etat(partage);
        printf("Ascenseur %d : %lu trajet(s), position finale %d (%s)\n",
               i + 1, trajets, etage, nom_etage(etage));
    }
    printf("Statistiques detaillees : %s\n", chemin);
    printf("===================================================\n");
}
