/* Initialisation de la memoire partagee et des semaphores POSIX vus en cours. */
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "synchronisation.h"

EtatPartage *creer_etat_partage(int *identifiant_shm)
{
    *identifiant_shm = shmget(IPC_PRIVATE, sizeof(EtatPartage), IPC_CREAT | 0600);
    if (*identifiant_shm < 0)
        return NULL;

    EtatPartage *partage = (EtatPartage *)shmat(*identifiant_shm, NULL, 0);
    if (partage == (void *)-1)
    {
        shmctl(*identifiant_shm, IPC_RMID, NULL);
        return NULL;
    }
    memset(partage, 0, sizeof(*partage));

    /* pshared = 1 : les processus fils partagent ces semaphores. */
    if (sem_init(&partage->semaphore_etat, 1, 1) < 0)
        goto erreur;
    if (sem_init(&partage->semaphore_evenements, 1, 0) < 0)
    {
        sem_destroy(&partage->semaphore_etat);
        goto erreur;
    }
    if (sem_init(&partage->semaphore_alertes, 1, 0) < 0)
    {
        sem_destroy(&partage->semaphore_evenements);
        sem_destroy(&partage->semaphore_etat);
        goto erreur;
    }

    for (int i = 0; i < NB_ASCENSEURS; i++)
    {
        partage->ascenseurs[i].etage = 0;
        partage->ascenseurs[i].direction = DIRECTION_IMMOBILE;
        partage->ascenseurs[i].destination = 0;
        partage->ascenseurs[i].occupe = 0;
        partage->ascenseurs[i].hors_service = 0;
        partage->ascenseurs[i].capacite = MAX_COMMANDES_ASCENSEUR;
    }
    return partage;

erreur:
    shmdt(partage);
    shmctl(*identifiant_shm, IPC_RMID, NULL);
    return NULL;
}

void verrouiller_etat(EtatPartage *partage)
{
    while (sem_wait(&partage->semaphore_etat) < 0 && errno == EINTR)
    {
    }
}

void deverrouiller_etat(EtatPartage *partage)
{
    sem_post(&partage->semaphore_etat);
}

void detruire_etat_partage(int identifiant_shm, EtatPartage *partage)
{
    if (partage == NULL || partage == (void *)-1)
        return;
    sem_destroy(&partage->semaphore_etat);
    sem_destroy(&partage->semaphore_evenements);
    sem_destroy(&partage->semaphore_alertes);
    shmdt(partage);
    shmctl(identifiant_shm, IPC_RMID, NULL);
}
