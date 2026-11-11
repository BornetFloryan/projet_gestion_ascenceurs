#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include "types.h"

int creer_file_messages(void);
int envoyer_demande(int id_file, const Demande *demande);
int recevoir_demande(int id_file, Demande *demande);
int envoyer_commande(int id_file, int numero_ascenseur, int action,
                     const Demande *demande);
int recevoir_commande(int id_file, int numero_ascenseur,
                      MessageCommande *message, int options);
int envoyer_fin(int id_file, int numero_ascenseur, const Demande *demande,
                int statut, time_t prise, time_t fin);
int recevoir_fin(int id_file, int numero_ascenseur, MessageFin *message);
int envoyer_notification(int id_file, const Demande *demande,
                         int statut, int numero_ascenseur);
int recevoir_notification(int id_file, int id_usager,
                          MessageNotification *notification);
void supprimer_file_messages(int id_file);

#endif
