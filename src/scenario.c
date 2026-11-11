/* Scenario de maintenance : panne programmee, puis reparation apres quelques secondes. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "scenario.h"
#include "communication.h"

void *scenario_panne_reparation(void *argument)
{
    ParametreScenario *parametre = (ParametreScenario *)argument;
    Demande aucune_demande;
    aucune_demande.id = -1;
    aucune_demande.etage_depart = 0;
    aucune_demande.etage_arrivee = 0;
    aucune_demande.priorite = PRIORITE_NORMALE;
    aucune_demande.tentatives = 0;
    aucune_demande.heure_creation = time(NULL);

    int attente = 4;
    if (parametre->configuration->utilisateurs < 4)
        attente = 1;
    sleep((unsigned int)attente);
    int ascenseur = rand() % NB_ASCENSEURS;
    int tirage = rand() % 100;
    if (tirage >= parametre->configuration->panne_probabilite)
        return NULL;
    printf("[SCENARIO] Panne aleatoire de l'ascenseur %d.\n", ascenseur + 1);
    if (envoyer_commande(parametre->id_file, ascenseur, COMMANDE_PANNE, &aucune_demande) < 0)
        perror("envoi ordre panne");

    sleep((unsigned int)parametre->configuration->duree_reparation);
    printf("[SCENARIO] Fin de l'intervention de maintenance.\n");
    if (envoyer_commande(parametre->id_file, ascenseur, COMMANDE_REPARER, &aucune_demande) < 0)
        perror("envoi ordre reparation");

    return NULL;
}
