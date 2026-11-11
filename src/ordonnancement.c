/* Strategie locale d'affectation : proximite, sens de circulation et charge. */
#include <stdlib.h>
#include <float.h>
#include "ordonnancement.h"

static double cout_ascenseur(const EtatAscenseur *etat, int charge,
                             const Demande *demande)
{
    if (etat->hors_service || charge >= etat->capacite)
        return DBL_MAX;

    double cout = (double)abs(etat->etage - demande->etage_depart);
    if (charge > 0)
    {
        int direction_appel = (demande->etage_depart > etat->etage) ? DIRECTION_MONTEE : ((demande->etage_depart < etat->etage) ? DIRECTION_DESCENTE : DIRECTION_IMMOBILE);
        int destination = etat->destination;

        /* Estimation du moment ou l'ascenseur pourra atteindre le nouvel appel. */
        if (etat->occupe && etat->direction == DIRECTION_MONTEE &&
            demande->etage_depart >= etat->etage && demande->etage_depart <= destination)
        {
            cout = (double)(demande->etage_depart - etat->etage);
        }
        else if (etat->occupe && etat->direction == DIRECTION_DESCENTE &&
                 demande->etage_depart <= etat->etage && demande->etage_depart >= destination)
        {
            cout = (double)(etat->etage - demande->etage_depart);
        }
        else if (etat->occupe)
        {
            cout = (double)(abs(destination - etat->etage) +
                            abs(demande->etage_depart - destination));
        }

        cout += 4.0 * charge; /* Penalite correspondant aux commandes deja affectees. */
        if (direction_appel != DIRECTION_IMMOBILE && etat->direction == direction_appel)
            cout -= 1.5;
        else if (direction_appel != DIRECTION_IMMOBILE && etat->direction != DIRECTION_IMMOBILE)
            cout += 2.0;
    }
    return cout;
}

int choisir_ascenseur(const EtatAscenseur etats[NB_ASCENSEURS],
                      const int charges[NB_ASCENSEURS], const Demande *demande)
{
    int meilleur_ascenseur = -1;
    double meilleur_cout = DBL_MAX;
    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        double cout = cout_ascenseur(&etats[i], charges[i], demande);
        if (cout < meilleur_cout)
        {
            meilleur_cout = cout;
            meilleur_ascenseur = i;
        }
    }
    return meilleur_ascenseur;
}

int demande_a_precedence(const Demande *a, const Demande *b)
{
    if (a->priorite != b->priorite)
        return a->priorite > b->priorite;
    if (a->heure_creation != b->heure_creation)
        return a->heure_creation < b->heure_creation;
    return a->id < b->id;
}
