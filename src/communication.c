
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "communication.h"

#define TYPE_DEMANDE 1L
#define TYPE_COMMANDE_BASE 100L
#define TYPE_FIN_BASE 200L

int creer_file_messages(void)
{
    return msgget(IPC_PRIVATE, IPC_CREAT | 0600);
}

int envoyer_demande(int file_id, Demande demande)
{
    MessageDemande message;

    message.type = TYPE_DEMANDE;
    message.demande = demande;

    return msgsnd(file_id, &message,
                  sizeof(message.demande), 0);
}

int envoyer_commande(int file_id, int numero_ascenseur, Demande demande)
{
    MessageAscenseur message;

    message.type = TYPE_COMMANDE_BASE + numero_ascenseur;
    message.demande = demande;
    message.etage_actuel = 0;

    return msgsnd(file_id, &message,
                  sizeof(message) - sizeof(long), 0);
}

int recevoir_commande(int file_id, int numero_ascenseur,
                      MessageAscenseur *message)
{

    return msgrcv(file_id, message,
                  sizeof(*message) - sizeof(long),
                  TYPE_COMMANDE_BASE + numero_ascenseur, 0);
}

int envoyer_fin_trajet(int file_id, int numero_ascenseur,
                       Demande demande, int etage_actuel)
{

    MessageAscenseur message;

    message.type = TYPE_FIN_BASE + numero_ascenseur;
    message.demande = demande;
    message.etage_actuel = etage_actuel;

    return msgsnd(file_id, &message,
                  sizeof(message) - sizeof(long), 0);
}

int recevoir_fin_trajet(int file_id, int numero_ascenseur,
                        MessageAscenseur *message)
{

    return msgrcv(file_id, message,
                  sizeof(*message) - sizeof(long),
                  TYPE_FIN_BASE + numero_ascenseur, IPC_NOWAIT);
}

int recevoir_nouvelle_demande(int file_id, Demande *demande)
{
    MessageDemande message;

    int resultat = msgrcv(file_id, &message,
                          sizeof(message.demande),
                          TYPE_DEMANDE, IPC_NOWAIT);

    if (resultat >= 0)
    {
        *demande = message.demande;
    }

    return resultat;
}

void supprimer_file_messages(int file_id)
{
    msgctl(file_id, IPC_RMID, NULL);
}
