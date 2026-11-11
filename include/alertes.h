#ifndef ALERTES_H
#define ALERTES_H
#include "types.h"
void envoyer_alerte(int tube_ecriture, EtatPartage *partage,
                    int type, int numero_ascenseur, int etage, int demande_id);
void *thread_alertes(void *argument);
#endif
