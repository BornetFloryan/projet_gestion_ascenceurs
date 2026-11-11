/* Description de l'immeuble et des espaces associes a chaque niveau. */
#include "immeuble.h"

const Immeuble IMMEUBLE_BUREAUX = {
    .niveaux = {
        {0, "Accueil"},
        {1, "Espace coworking"},
        {2, "Ressources humaines"},
        {3, "Comptabilite"},
        {4, "Developpement"},
        {5, "Cafeteria"},
        {6, "Salle Horizon"},
        {7, "Marketing"},
        {8, "Support technique"},
        {9, "Innovation"},
        {10, "Direction generale"}}};

const char *nom_etage(int etage)
{
    if (etage < 0 || etage > NB_ETAGES)
        return "Niveau inconnu";
    return IMMEUBLE_BUREAUX.niveaux[etage].usage;
}
