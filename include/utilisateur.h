#ifndef UTILISATEUR_H
#define UTILISATEUR_H

#include "types.h"

typedef struct {
    int id;
    int file_id;
} ParametreUtilisateur;

void *generer_utilisateur(void *argument);

#endif
