/* Chaque thread represente un usager qui formule un appel d'ascenseur. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "utilisateur.h"
#include "communication.h"
#include "immeuble.h"

void *generer_utilisateur(void *argument)
{
    ParametreUtilisateur *parametre = (ParametreUtilisateur *)argument;
    Demande demande;
    int tirage = 0;

    demande.id = parametre->id;
    demande.tentatives = 0;
    demande.heure_creation = time(NULL);

    if (parametre->a_demande_preparee)
    {
        demande = parametre->demande_preparee;
        demande.id = parametre->id;
        demande.heure_creation = time(NULL);
    }
    else
    {

    /* rand() est partage entre threads : le mutex protege la generation aleatoire. */
    pthread_mutex_lock(&parametre->statistiques->mutex_aleatoire);
    tirage = rand() % 100;
    if (parametre->id % 3 == 1)
    {
        demande.etage_depart = 0; /* Flux du matin : entree vers les bureaux. */
        demande.etage_arrivee = 1 + rand() % NB_ETAGES;
    }
    else if (parametre->id % 3 == 2)
    {
        demande.etage_depart = 1 + rand() % NB_ETAGES;
        demande.etage_arrivee = 0; /* Retour vers l'accueil. */
    }
    else
    {
        demande.etage_depart = rand() % (NB_ETAGES + 1);
        do
        {
            demande.etage_arrivee = rand() % (NB_ETAGES + 1);
        } while (demande.etage_arrivee == demande.etage_depart);
    }
    pthread_mutex_unlock(&parametre->statistiques->mutex_aleatoire);
    }

    if (!parametre->a_demande_preparee)
    {
        if (tirage < 10)
            demande.priorite = PRIORITE_URGENTE;
        else if (tirage < 30)
            demande.priorite = PRIORITE_ACCES;
        else
            demande.priorite = PRIORITE_NORMALE;
    }

    if (!parametre->partage->mode_interactif)
        printf("[Usager %02d] Appel %d (%s) -> %d (%s), priorite %d.\n",
               demande.id, demande.etage_depart, nom_etage(demande.etage_depart),
               demande.etage_arrivee, nom_etage(demande.etage_arrivee), demande.priorite);

    if (envoyer_demande(parametre->id_file, &demande) == 0)
    {
        pthread_mutex_lock(&parametre->statistiques->mutex_compteurs);
        parametre->statistiques->demandes_generees++;
        pthread_mutex_unlock(&parametre->statistiques->mutex_compteurs);
        sem_post(&parametre->partage->semaphore_evenements);

        /* L'usager reste actif et lit les notifications portant son type de message. */
        int trajet_termine = 0;
        while (!trajet_termine)
        {
            MessageNotification notification;
            if (recevoir_notification(parametre->id_file, demande.id,
                                      &notification) < 0)
            {
                if (errno == EINTR)
                    continue;
                perror("msgrcv notification usager");
                break;
            }

            switch (notification.statut)
            {
            case NOTIFICATION_ENREGISTREE:
                if (parametre->partage->affichage_detaille)
                    printf("[Usager %02d] Notification : appel enregistre.\n", demande.id);
                break;
            case NOTIFICATION_AFFECTEE:
                if (parametre->partage->affichage_detaille)
                    printf("[Usager %02d] Notification : ascenseur %d affecte a l'appel.\n",
                           demande.id, notification.numero_ascenseur + 1);
                break;
            case NOTIFICATION_REAFFECTEE:
                printf("[Usager %02d] Notification : appel remis en attente apres une panne.\n",
                       demande.id);
                break;
            case NOTIFICATION_TERMINEE:
                printf("[Usager %02d] Notification : trajet termine par l'ascenseur %d.\n",
                       demande.id, notification.numero_ascenseur + 1);
                if (parametre->partage->mode_interactif)
                {
                    puts("\n[TERMINE] Le trajet de l'usager est termine.");
                    puts("\n=== CONSOLE ASCENSEURS ===");
                    puts("appel | etat | panne | reparer | aide | quitter");
                    printf("> ");
                }
                trajet_termine = 1;
                break;
            case NOTIFICATION_ARRET:
                printf("[Usager %02d] Notification : simulation interrompue.\n", demande.id);
                trajet_termine = 1;
                break;
            default:
                break;
            }
            fflush(stdout);
        }
    }
    else
    {
        perror("msgsnd demande usager");
    }

    free(parametre);
    return NULL;
}
