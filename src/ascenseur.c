/* Chaque ascenseur fonctionne dans un processus fils independant. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/msg.h>
#include "ascenseur.h"
#include "communication.h"
#include "synchronisation.h"
#include "alertes.h"
#include "immeuble.h"

static volatile sig_atomic_t arret_local = 0;

static void gerer_signal(int numero_signal)
{
    (void)numero_signal;
    arret_local = 1;
}

typedef struct
{
    Demande demandes[CAPACITE_ATTENTE_ASCENSEUR];
    int nombre;
} FileLocale;

static void mettre_a_jour(EtatPartage *partage, int numero, int etage,
                          int direction, int destination, int occupe, int panne)
{
    verrouiller_etat(partage);
    EtatAscenseur *etat = &partage->ascenseurs[numero];
    etat->etage = etage;
    etat->direction = direction;
    etat->destination = destination;
    etat->occupe = occupe;
    etat->hors_service = panne;
    deverrouiller_etat(partage);
}

static int ajouter_locale(FileLocale *file, const Demande *demande)
{
    if (file->nombre >= CAPACITE_ATTENTE_ASCENSEUR)
        return -1;
    file->demandes[file->nombre++] = *demande;
    return 0;
}

static Demande retirer_locale(FileLocale *file)
{
    Demande demande = file->demandes[0];
    for (int i = 0; i < file->nombre - 1; i++)
        file->demandes[i] = file->demandes[i + 1];
    file->nombre--;
    return demande;
}

/* Recupere les ordres arrivant pendant un deplacement sans bloquer l'ascenseur. */
static int lire_ordres_en_attente(int id_file, int numero, EtatPartage *partage,
                                  FileLocale *file)
{
    MessageCommande message;
    while (recevoir_commande(id_file, numero, &message, IPC_NOWAIT) == 0)
    {
        if (message.action == COMMANDE_ATTRIBUER)
        {
            if (ajouter_locale(file, &message.demande) < 0)
            {
                fprintf(stderr, "[Ascenseur %d] File locale pleine.\n", numero + 1);
                envoyer_fin(id_file, numero, &message.demande, FIN_PANNE, 0, time(NULL));
                sem_post(&partage->semaphore_evenements);
            }
        }
        else if (message.action == COMMANDE_PANNE)
        {
            return 1;
        }
        else if (message.action == COMMANDE_ARRET)
        {
            arret_local = 1;
            return -1;
        }
        /* Une commande de reparation n'est traitee que dans l'etat hors service. */
    }
    return 0;
}

static int deplacer(int id_file, int numero, EtatPartage *partage,
                    FileLocale *file, int *position,
                    int destination)
{
    int direction = destination > *position ? DIRECTION_MONTEE : (destination < *position ? DIRECTION_DESCENTE : DIRECTION_IMMOBILE);
    mettre_a_jour(partage, numero, *position, direction, destination, 1, 0);

    while (*position != destination && !arret_local)
    {
        sleep(1); /* Un niveau parcouru represente une seconde de simulation. */
        int ordre = lire_ordres_en_attente(id_file, numero, partage, file);
        if (ordre != 0)
            return ordre;

        *position += direction;
        mettre_a_jour(partage, numero, *position, direction, destination, 1, 0);
        if (partage->affichage_detaille)
        {
            printf("[Ascenseur %d] Niveau %d - %s\n",
                   numero + 1, *position, nom_etage(*position));
            fflush(stdout);
        }
    }
    return arret_local ? -1 : 0;
}

/* L'ascenseur reste immobilise jusqu'a la reception de l'ordre de reparation. */
static int attendre_reparation(int id_file, int numero, EtatPartage *partage,
                               int tube_ecriture, int position,
                               FileLocale *file, const Demande *active)
{
    mettre_a_jour(partage, numero, position, DIRECTION_IMMOBILE, position, 0, 1);

    if (active != NULL)
    {
        printf("[Ascenseur %d] Interruption de la demande %d.\n",
               numero + 1, active->id);
        envoyer_fin(id_file, numero, active, FIN_PANNE, 0, time(NULL));
        sem_post(&partage->semaphore_evenements);
    }
    envoyer_alerte(tube_ecriture, partage, ALERTE_PANNE, numero, position,
                   active != NULL ? active->id : -1);
    sem_post(&partage->semaphore_evenements);

    while (!arret_local)
    {
        MessageCommande message;
        if (recevoir_commande(id_file, numero, &message, 0) < 0)
        {
            if (errno == EINTR)
                continue;
            perror("msgrcv pendant panne");
            return -1;
        }
        if (message.action == COMMANDE_ATTRIBUER)
        {
            /* Une commande deja affectee est conservee pour apres la reparation. */
            if (ajouter_locale(file, &message.demande) < 0)
                fprintf(stderr, "[Ascenseur %d] Impossible de conserver une demande en attente.\n", numero + 1);
        }
        else if (message.action == COMMANDE_REPARER)
        {
            mettre_a_jour(partage, numero, position, DIRECTION_IMMOBILE, position, 0, 0);
            envoyer_fin(id_file, numero, active != NULL ? active : &(Demande){.id = -1},
                        FIN_REPARATION, 0, time(NULL));
            sem_post(&partage->semaphore_evenements);
            envoyer_alerte(tube_ecriture, partage, ALERTE_REPARATION, numero, position, -1);
            return 0;
        }
        else if (message.action == COMMANDE_ARRET)
        {
            arret_local = 1;
            return -1;
        }
    }
    return -1;
}

void processus_ascenseur(int id_file, int numero_ascenseur,
                         EtatPartage *partage, int tube_ecriture)
{
    signal(SIGTERM, gerer_signal);
    signal(SIGINT, gerer_signal);

    FileLocale attente = {.nombre = 0};
    int position = 0;
    if (!partage->mode_interactif)
    {
        printf("[Ascenseur %d / PID %ld] Pret au niveau 0.\n",
               numero_ascenseur + 1, (long)getpid());
        fflush(stdout);
    }

    while (!arret_local)
    {
        MessageCommande message;
        Demande demande;
        if (attente.nombre > 0)
        {
            demande = retirer_locale(&attente);
            message.action = COMMANDE_ATTRIBUER;
        }
        else
        {
            if (recevoir_commande(id_file, numero_ascenseur, &message, 0) < 0)
            {
                if (errno == EINTR && arret_local)
                    break;
                if (errno == EINTR)
                    continue;
                perror("msgrcv commande ascenseur");
                break;
            }
            demande = message.demande;
        }

        if (message.action == COMMANDE_ARRET)
            break;
        if (message.action == COMMANDE_PANNE)
        {
            if (attendre_reparation(id_file, numero_ascenseur, partage,
                                    tube_ecriture, position, &attente, NULL) < 0)
                break;
            continue;
        }
        if (message.action == COMMANDE_REPARER)
            continue;
        if (message.action != COMMANDE_ATTRIBUER)
            continue;

        printf("[Ascenseur %d] Demande %d : %d (%s) -> %d (%s).\n",
               numero_ascenseur + 1, demande.id,
               demande.etage_depart, nom_etage(demande.etage_depart),
               demande.etage_arrivee, nom_etage(demande.etage_arrivee));
        fflush(stdout);

        int resultat = deplacer(id_file, numero_ascenseur, partage,
                                &attente, &position, demande.etage_depart);
        if (resultat == 1)
        {
            if (attendre_reparation(id_file, numero_ascenseur, partage,
                                    tube_ecriture, position, &attente, &demande) < 0)
                break;
            continue;
        }
        if (resultat < 0)
            break;

        time_t prise = time(NULL);
        printf("[Ascenseur %d] Usager %d pris en charge au niveau %d.\n",
               numero_ascenseur + 1, demande.id, position);
        fflush(stdout);

        resultat = deplacer(id_file, numero_ascenseur, partage,
                            &attente, &position, demande.etage_arrivee);
        if (resultat == 1)
        {
            if (attendre_reparation(id_file, numero_ascenseur, partage,
                                    tube_ecriture, position, &attente, &demande) < 0)
                break;
            continue;
        }
        if (resultat < 0)
            break;

        verrouiller_etat(partage);
        partage->ascenseurs[numero_ascenseur].etage = position;
        partage->ascenseurs[numero_ascenseur].direction = DIRECTION_IMMOBILE;
        partage->ascenseurs[numero_ascenseur].destination = position;
        partage->ascenseurs[numero_ascenseur].occupe = 0;
        partage->ascenseurs[numero_ascenseur].trajets_termines++;
        deverrouiller_etat(partage);

        time_t fin = time(NULL);
        printf("[Ascenseur %d] Demande %d terminee au niveau %d.\n",
               numero_ascenseur + 1, demande.id, position);
        fflush(stdout);
        if (envoyer_fin(id_file, numero_ascenseur, &demande,
                        FIN_TRAJET, prise, fin) < 0)
            perror("envoi fin de trajet");
        sem_post(&partage->semaphore_evenements);
    }

    mettre_a_jour(partage, numero_ascenseur, position,
                  DIRECTION_IMMOBILE, position, 0, 0);
    printf("[Ascenseur %d] Arret propre.\n", numero_ascenseur + 1);
    fflush(stdout);
    _exit(EXIT_SUCCESS);
}
