#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utilisateur.h"
#include "communication.h"

void *generer_utilisateur(void *argument) {
    ParametreUtilisateur *parametre = argument;
    Demande demande;

    demande.id = parametre->id;
    demande.etage_depart = rand() % (NB_ETAGES + 1);
    do {
        demande.etage_arrivee = rand() % (NB_ETAGES + 1);
    } while (demande.etage_arrivee == demande.etage_depart);

    printf("[Utilisateur %d] Demande : etage %d -> etage %d\n",
           demande.id, demande.etage_depart, demande.etage_arrivee);
    fflush(stdout);

    if (envoyer_demande(parametre->file_id, demande) < 0) {
        perror("msgsnd demande");
    }

    free(parametre);
    return NULL;
}
