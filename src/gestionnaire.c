
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "communication.h"
#include "gestionnaire.h"

#define CAPACITE_FILE 100

void gerer_demandes(int file_id, int nombre_utilisateurs)
{

    Demande attente[CAPACITE_FILE];

    int debut = 0;
    int fin = 0;
    int nombre = 0;

    EtatAscenseur ascenseurs[NB_ASCENSEURS] = {
        {0, 0},
        {0, 0}};

    int demandes_recues = 0;
    int demandes_terminees = 0;

    while (demandes_terminees < nombre_utilisateurs)
    {

        Demande nouvelle;

        while (demandes_recues < nombre_utilisateurs &&
               nombre < CAPACITE_FILE &&
               recevoir_nouvelle_demande(file_id, &nouvelle) >= 0)
        {

            attente[fin] = nouvelle;

            fin = (fin + 1) % CAPACITE_FILE;
            nombre++;

            demandes_recues++;

            printf("[Gestionnaire] Nouvelle demande %d recue\n",
                   nouvelle.id);

            fflush(stdout);
        }

        for (int i = 0; i < NB_ASCENSEURS; i++)
        {

            MessageAscenseur retour;

            if (recevoir_fin_trajet(file_id, i, &retour) >= 0)
            {

                ascenseurs[i].etage = retour.etage_actuel;
                ascenseurs[i].occupe = 0;

                demandes_terminees++;

                printf("[Gestionnaire] Ascenseur %d disponible a l'etage %d\n",
                       i + 1, ascenseurs[i].etage);

                fflush(stdout);
            }
        }

        while (nombre > 0)
        {

            int meilleur = -1;
            int meilleure_distance = NB_ETAGES + 1;

            for (int i = 0; i < NB_ASCENSEURS; i++)
            {

                if (!ascenseurs[i].occupe)
                {

                    int distance = abs(
                        ascenseurs[i].etage -
                        attente[debut].etage_depart);

                    if (distance < meilleure_distance)
                    {

                        meilleure_distance = distance;
                        meilleur = i;
                    }
                }
            }

            if (meilleur == -1)
            {
                break;
            }

            Demande demande = attente[debut];

            debut = (debut + 1) % CAPACITE_FILE;
            nombre--;

            ascenseurs[meilleur].occupe = 1;

            printf("[Gestionnaire] Demande %d attribuee a l'ascenseur %d (distance : %d etages)\n",
                   demande.id,
                   meilleur + 1,
                   meilleure_distance);

            fflush(stdout);

            if (envoyer_commande(file_id, meilleur, demande) < 0)
            {

                perror("Erreur envoi commande");
                exit(EXIT_FAILURE);
            }
        }

        usleep(100000);
    }

    printf("[Gestionnaire] Toutes les demandes ont ete traitees.\n");
}
