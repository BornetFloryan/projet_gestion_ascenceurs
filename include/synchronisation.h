#ifndef SYNCHRONISATION_H
#define SYNCHRONISATION_H
#include "types.h"

EtatPartage *creer_etat_partage(int *identifiant_shm);
void detruire_etat_partage(int identifiant_shm, EtatPartage *partage);
void verrouiller_etat(EtatPartage *partage);
void deverrouiller_etat(EtatPartage *partage);

#endif
