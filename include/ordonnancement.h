#ifndef ORDONNANCEMENT_H
#define ORDONNANCEMENT_H
#include "types.h"

int choisir_ascenseur(const EtatAscenseur etats[NB_ASCENSEURS],
                      const int charges[NB_ASCENSEURS], const Demande *demande);
int demande_a_precedence(const Demande *a, const Demande *b);

#endif
