/* Fonctions d'echange de messages entre le gestionnaire et les processus ascenseurs. */
#include <sys/ipc.h>
#include <sys/msg.h>
#include "communication.h"

int creer_file_messages(void)
{
    return msgget(IPC_PRIVATE, IPC_CREAT | 0600);
}

int envoyer_demande(int id_file, const Demande *demande)
{
    MessageDemande message;
    message.type = TYPE_DEMANDE;
    message.demande = *demande;
    return msgsnd(id_file, &message, sizeof(message) - sizeof(message.type), 0);
}

int recevoir_demande(int id_file, Demande *demande)
{
    MessageDemande message;
    int resultat = (int)msgrcv(id_file, &message,
                               sizeof(message) - sizeof(message.type), TYPE_DEMANDE, IPC_NOWAIT);
    if (resultat >= 0)
        *demande = message.demande;
    return resultat < 0 ? -1 : 0;
}

int envoyer_commande(int id_file, int numero_ascenseur, int action,
                     const Demande *demande)
{
    MessageCommande message;
    message.type = TYPE_COMMANDE_BASE + numero_ascenseur;
    message.action = action;
    if (demande != NULL)
        message.demande = *demande;
    else
    {
        message.demande.id = -1;
        message.demande.etage_depart = 0;
        message.demande.etage_arrivee = 0;
        message.demande.priorite = PRIORITE_NORMALE;
        message.demande.tentatives = 0;
        message.demande.heure_creation = time(NULL);
    }
    return msgsnd(id_file, &message, sizeof(message) - sizeof(message.type), 0);
}

int recevoir_commande(int id_file, int numero_ascenseur,
                      MessageCommande *message, int options)
{
    int resultat = (int)msgrcv(id_file, message,
                               sizeof(*message) - sizeof(message->type),
                               TYPE_COMMANDE_BASE + numero_ascenseur, options);
    return resultat < 0 ? -1 : 0;
}

int envoyer_fin(int id_file, int numero_ascenseur, const Demande *demande,
                int statut, time_t prise, time_t fin)
{
    MessageFin message;
    message.type = TYPE_FIN_BASE + numero_ascenseur;
    message.demande = *demande;
    message.numero_ascenseur = numero_ascenseur;
    message.statut = statut;
    message.heure_prise_en_charge = prise;
    message.heure_fin = fin;
    return msgsnd(id_file, &message, sizeof(message) - sizeof(message.type), 0);
}

int recevoir_fin(int id_file, int numero_ascenseur, MessageFin *message)
{
    int resultat = (int)msgrcv(id_file, message,
                               sizeof(*message) - sizeof(message->type),
                               TYPE_FIN_BASE + numero_ascenseur, IPC_NOWAIT);
    return resultat < 0 ? -1 : 0;
}

/* Chaque usager lit un type de message qui lui est reserve : TYPE_NOTIFICATION_BASE + id. */
int envoyer_notification(int id_file, const Demande *demande,
                         int statut, int numero_ascenseur)
{
    MessageNotification notification;
    notification.type = TYPE_NOTIFICATION_BASE + demande->id;
    notification.demande_id = demande->id;
    notification.statut = statut;
    notification.numero_ascenseur = numero_ascenseur;
    return msgsnd(id_file, &notification,
                  sizeof(notification) - sizeof(notification.type), 0);
}

int recevoir_notification(int id_file, int id_usager,
                          MessageNotification *notification)
{
    int resultat = (int)msgrcv(id_file, notification,
                               sizeof(*notification) - sizeof(notification->type),
                               TYPE_NOTIFICATION_BASE + id_usager, 0);
    return resultat < 0 ? -1 : 0;
}

void supprimer_file_messages(int id_file)
{
    if (id_file >= 0)
        msgctl(id_file, IPC_RMID, NULL);
}
