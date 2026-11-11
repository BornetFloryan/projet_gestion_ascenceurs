/* Thread de gestion : reception, priorites et affectation des appels. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "gestionnaire.h"
#include "communication.h"
#include "ordonnancement.h"
#include "synchronisation.h"
#include "statistiques.h"

typedef struct
{
    Demande demandes[CAPACITE_FILE];
    int nombre;
} FileAttente;

static int console_interactive = 0;

static int ajouter_demande(FileAttente *file, const Demande *demande)
{
    if (file->nombre >= CAPACITE_FILE)
        return -1;
    file->demandes[file->nombre++] = *demande;
    return 0;
}

static int indice_prioritaire(const FileAttente *file)
{
    int meilleur = 0;
    for (int i = 1; i < file->nombre; i++)
    {
        if (demande_a_precedence(&file->demandes[i], &file->demandes[meilleur]))
            meilleur = i;
    }
    return meilleur;
}

static void retirer_indice(FileAttente *file, int indice)
{
    for (int i = indice; i < file->nombre - 1; i++)
        file->demandes[i] = file->demandes[i + 1];
    file->nombre--;
}

static void recevoir_nouvelles_demandes(int id_file, FileAttente *file)
{
    Demande demande;
    while (file->nombre < CAPACITE_FILE && recevoir_demande(id_file, &demande) == 0)
    {
        if (ajouter_demande(file, &demande) == 0)
        {
            if (!console_interactive)
                printf("[Gestionnaire] Demande %d recue (priorite %d).\n",
                       demande.id, demande.priorite);
            if (envoyer_notification(id_file, &demande,
                                     NOTIFICATION_ENREGISTREE, -1) < 0)
                perror("notification appel enregistre");
        }
    }
}

static void recevoir_fins(int id_file, int charges[NB_ASCENSEURS],
                          FileAttente *file, BilanSimulation *bilan,
                          StatistiquesThreads *statistiques)
{
    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        MessageFin message;
        while (recevoir_fin(id_file, i, &message) == 0)
        {
            if (message.statut == FIN_REPARATION)
            {
                if (!console_interactive)
                    printf("[Gestionnaire] Ascenseur %d de nouveau disponible.\n", i + 1);
                continue;
            }

            if (charges[i] > 0)
                charges[i]--;
            if (message.statut == FIN_PANNE)
            {
                message.demande.tentatives++;
                bilan->demandes_reaffectees++;
                if (envoyer_notification(id_file, &message.demande,
                                         NOTIFICATION_REAFFECTEE, -1) < 0)
                    perror("notification reaffectation");
                if (ajouter_demande(file, &message.demande) < 0)
                    fprintf(stderr, "[Gestionnaire] File d'attente pleine : demande %d non reaffectee.\n",
                            message.demande.id);
                else
                    if (!console_interactive)
                        printf("[Gestionnaire] Demande %d replacee dans la file apres panne.\n",
                               message.demande.id);
                continue;
            }

            bilan->demandes_terminees++;
            bilan->trajets[i]++;
            long attente = (long)(message.heure_prise_en_charge - message.demande.heure_creation);
            long duree = (long)(message.heure_fin - message.demande.heure_creation);
            if (attente < 0)
                attente = 0;
            if (duree < 0)
                duree = 0;
            bilan->attente_totale += attente;
            bilan->duree_totale += duree;
            ajouter_statistique(bilan, &message.demande, i, attente, duree);

            pthread_mutex_lock(&statistiques->mutex_compteurs);
            statistiques->demandes_terminees++;
            pthread_mutex_unlock(&statistiques->mutex_compteurs);
            if (!console_interactive)
                printf("[Gestionnaire] Demande %d terminee par l'ascenseur %d.\n",
                       message.demande.id, i + 1);
            if (envoyer_notification(id_file, &message.demande,
                                     NOTIFICATION_TERMINEE, i) < 0)
                perror("notification fin de trajet");
        }
    }
}

static int affecter_demandes(FileAttente *file, int id_file,
                             EtatPartage *partage,
                             int charges[NB_ASCENSEURS])
{
    int nombre_affecte = 0;
    while (file->nombre > 0)
    {
        int indice = indice_prioritaire(file);
        Demande demande = file->demandes[indice];
        EtatAscenseur etats[NB_ASCENSEURS];
        verrouiller_etat(partage);
        for (int i = 0; i < NB_ASCENSEURS; i++)
            etats[i] = partage->ascenseurs[i];
        deverrouiller_etat(partage);

        int choix = choisir_ascenseur(etats, charges, &demande);
        if (choix < 0)
            break; /* Les ascenseurs sont occupes ou en maintenance. */
        if (envoyer_commande(id_file, choix, COMMANDE_ATTRIBUER, &demande) < 0)
        {
            perror("msgsnd commande ascenseur");
            break;
        }
        retirer_indice(file, indice);
        charges[choix]++;
        nombre_affecte++;
        if (!console_interactive)
            printf("[Gestionnaire] Demande %d affectee a l'ascenseur %d (charge %d).\n",
                   demande.id, choix + 1, charges[choix]);
        if (envoyer_notification(id_file, &demande,
                                 NOTIFICATION_AFFECTEE, choix) < 0)
            perror("notification affectation");
    }
    return nombre_affecte;
}

void *thread_gestionnaire(void *argument)
{
    ParametreGestionnaire *parametre = (ParametreGestionnaire *)argument;
    console_interactive = parametre->partage->mode_interactif;
    FileAttente attente = {.nombre = 0};
    int charges[NB_ASCENSEURS] = {0, 0};
    BilanSimulation bilan = {0};
    while (bilan.demandes_terminees < parametre->nombre_attendu &&
           !*parametre->arret)
    {
        while (sem_wait(&parametre->partage->semaphore_evenements) < 0)
        {
            if (errno == EINTR)
            {
                if (*parametre->arret)
                    break;
                continue;
            }
            perror("sem_wait gestionnaire");
            *parametre->arret = 1;
            break;
        }
        if (*parametre->arret)
            break;

        recevoir_nouvelles_demandes(parametre->id_file, &attente);
        recevoir_fins(parametre->id_file, charges, &attente, &bilan,
                      parametre->statistiques);
        affecter_demandes(&attente, parametre->id_file,
                          parametre->partage, charges);
    }

    if (sauvegarder_statistiques(parametre->chemin_statistiques, &bilan) < 0)
        perror("ecriture du fichier de statistiques");
    afficher_bilan(&bilan, parametre->partage, parametre->chemin_statistiques);
    parametre->resultat = bilan.demandes_terminees;
    pthread_mutex_lock(&parametre->statistiques->mutex_compteurs);
    parametre->statistiques->demandes_terminees = bilan.demandes_terminees;
    pthread_mutex_unlock(&parametre->statistiques->mutex_compteurs);
    return NULL;
}
