#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "types.h"

int creer_file_messages(void);
int envoyer_demande(int file_id, Demande demande);
int envoyer_commande(int file_id, int numero_ascenseur, Demande demande);
int recevoir_commande(int file_id, int numero_ascenseur, MessageAscenseur *message);
int envoyer_fin_trajet(int file_id, int numero_ascenseur, Demande demande, int etage_actuel);
int recevoir_fin_trajet(int file_id, int numero_ascenseur, MessageAscenseur *message);
int recevoir_nouvelle_demande(int file_id, Demande *demande);
void supprimer_file_messages(int file_id);

#endif
