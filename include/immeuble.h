#ifndef IMMEUBLE_H
#define IMMEUBLE_H
#include "types.h"

typedef struct
{
    int numero;
    const char *usage;
} Etage;

typedef struct
{
    Etage niveaux[NB_ETAGES + 1];
} Immeuble;

extern const Immeuble IMMEUBLE_BUREAUX;
const char *nom_etage(int etage);

#endif
