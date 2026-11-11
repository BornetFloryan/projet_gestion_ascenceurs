/* Les alertes des processus ascenseurs arrivent au processus principal par un tube.
 * Le semaphore indique au thread d'alerte qu'un enregistrement est disponible. */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "alertes.h"

void envoyer_alerte(int tube_ecriture, EtatPartage *partage,
                    int type, int numero_ascenseur, int etage, int demande_id)
{
    Alerte alerte;
    alerte.type = type;
    alerte.numero_ascenseur = numero_ascenseur;
    alerte.etage = etage;
    alerte.demande_id = demande_id;

    /* Une seule ecriture d'une petite structure conserve l'alerte comme un bloc. */
    if (write(tube_ecriture, &alerte, sizeof(alerte)) != (ssize_t)sizeof(alerte))
    {
        perror("write tube alertes");
        return;
    }
    sem_post(&partage->semaphore_alertes);
}

static int lire_alerte(int descripteur, Alerte *alerte)
{
    char *destination = (char *)alerte;
    size_t lus = 0;
    while (lus < sizeof(*alerte))
    {
        ssize_t resultat = read(descripteur, destination + lus, sizeof(*alerte) - lus);
        if (resultat < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (resultat == 0)
            return -1;
        lus += (size_t)resultat;
    }
    return 0;
}

void *thread_alertes(void *argument)
{
    ParametreAlertes *parametre = (ParametreAlertes *)argument;
    for (;;)
    {
        while (sem_wait(&parametre->partage->semaphore_alertes) < 0 && errno == EINTR)
        {
        }
        Alerte alerte;
        if (lire_alerte(parametre->tube_lecture, &alerte) < 0)
            break;
        if (alerte.type == ALERTE_FIN_THREAD)
            break;

        if (alerte.type == ALERTE_PANNE)
        {
            if (alerte.demande_id >= 0)
            {
                printf("\n[ALERTE] Ascenseur %d en panne au niveau %d. Demande %d interrompue et remise en attente.\n",
                       alerte.numero_ascenseur + 1, alerte.etage, alerte.demande_id);
            }
            else
            {
                printf("\n[ALERTE] Ascenseur %d en panne au niveau %d.\n",
                       alerte.numero_ascenseur + 1, alerte.etage);
            }
        }
        else if (alerte.type == ALERTE_REPARATION)
        {
            printf("\n[MAINTENANCE] Ascenseur %d repare et remis en service au niveau %d.\n",
                   alerte.numero_ascenseur + 1, alerte.etage);
        }
        fflush(stdout);
    }
    return NULL;
}
