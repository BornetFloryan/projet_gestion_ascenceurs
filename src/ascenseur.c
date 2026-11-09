#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "types.h"
#include "communication.h"
#include "ascenseur.h"

void processus_ascenseur(int file_id, int numero_ascenseur, int etage_initial) {
    int position = etage_initial;
    MessageAscenseur message;

    printf("[Ascenseur %d] Demarre a l'etage %d\n", numero_ascenseur + 1, position);
    fflush(stdout);

    while (1) {
        if (recevoir_commande(file_id, numero_ascenseur, &message) < 0) {
            perror("msgrcv ascenseur");
            exit(EXIT_FAILURE);
        }

        Demande demande = message.demande;
        if (demande.etage_depart == -1) break;

        printf("[Ascenseur %d] Demande %d : etage %d -> etage %d\n",
               numero_ascenseur + 1, demande.id,
               demande.etage_depart, demande.etage_arrivee);
        fflush(stdout);

        /* Aller chercher l'utilisateur */
        while (position != demande.etage_depart) {
            position += (position < demande.etage_depart) ? 1 : -1;
            printf("[Ascenseur %d] Position : etage %d\n", numero_ascenseur + 1, position);
            fflush(stdout);
            sleep(1);
        }

        printf("[Ascenseur %d] Utilisateur %d pris en charge\n",
               numero_ascenseur + 1, demande.id);
        fflush(stdout);

        /* Transporter l'utilisateur */
        while (position != demande.etage_arrivee) {
            position += (position < demande.etage_arrivee) ? 1 : -1;
            printf("[Ascenseur %d] Position : etage %d\n", numero_ascenseur + 1, position);
            fflush(stdout);
            sleep(1);
        }

        printf("[Ascenseur %d] Demande %d terminee\n", numero_ascenseur + 1, demande.id);
        fflush(stdout);

        if (envoyer_fin_trajet(file_id, numero_ascenseur, demande, position) < 0) {
            perror("msgsnd fin trajet");
            exit(EXIT_FAILURE);
        }
    }

    printf("[Ascenseur %d] Arret\n", numero_ascenseur + 1);
    fflush(stdout);
    exit(EXIT_SUCCESS);
}
