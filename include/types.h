#ifndef TYPES_H
#define TYPES_H

#define NB_ETAGES 10
#define NB_ASCENSEURS 2
#define NB_UTILISATEURS 8

typedef struct {
    int id;
    int etage_depart;
    int etage_arrivee;
} Demande;

typedef struct {
    long type;
    Demande demande;
} MessageDemande;

typedef struct {
    long type;
    Demande demande;
    int etage_actuel;
} MessageAscenseur;

typedef struct {
    int etage;
    int occupe;
} EtatAscenseur;

#endif
