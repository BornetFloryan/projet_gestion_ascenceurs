# Gestion d'ascenseurs - Projet SR70

Simulation en C sous Linux de deux ascenseurs dans un immeuble de onze niveaux
(niveaux 0 a 10). Le projet met en oeuvre des processus, des threads et
plusieurs mecanismes IPC Linux.

## Fonctionnalites

- deux processus fils, un par ascenseur (`fork`) ;
- threads POSIX pour les usagers, le gestionnaire et les alertes ;
- demandes automatiques ou saisies en mode interactif ;
- priorites et ordonnancement selon la position, la direction et la charge ;
- files de messages System V, memoire partagee et semaphores POSIX ;
- tube anonyme pour les alertes de panne ;
- pannes aleatoires, reparation et reaffectation des demandes ;
- nombre maximal de demandes affectees configurable par ascenseur ;
- export d'un fichier CSV distinct pour chaque execution.

## Compilation

Dans WSL Ubuntu ou sur une machine Linux :

```bash
make
```

Les fichiers objets sont places dans `build/`, pas dans `src/`.

## Execution

```bash
./gestion-ascenseurs
./gestion-ascenseurs -h
```

Options disponibles :

```text
-n nombre       nombre d'usagers (1 a 128)
-d secondes     delai entre deux creations d'usagers
-p pourcentage  probabilite de panne (0 a 100)
-r secondes     duree de la reparation
-c capacite     nombre maximal de demandes affectees par ascenseur
-i              mode interactif
-s graine       graine initiale de la simulation
-v              affichage detaille, avec chaque niveau parcouru
```

Exemples :

```bash
./gestion-ascenseurs -n 3 -d 0 -p 0 -s 42
./gestion-ascenseurs -n 12 -d 1 -p 30 -r 5 -c 2 -s 42
./gestion-ascenseurs -n 12 -d 1 -p 30 -r 5 -c 2 -s 42 -v
```

Le mode interactif ouvre une console de simulation. Le nombre d'appels possibles
est fixe par `-n` (12 par defaut). Les commandes sont `appel`, `etat`, `panne`,
`reparer`, `aide` et `quitter`. Pour `appel`, le programme affiche la liste des
niveaux et demande les numeros du depart, de la destination et de la priorite.

```bash
./gestion-ascenseurs -i -n 5 -d 0 -p 0
```

Puis, par exemple :

```text
> appel
Etage de depart (numero) : 0
Etage de destination (numero) : 5
Priorite (0 normale, 1 prioritaire, 2 urgente) : 0
> etat
> panne
Numero de l'ascenseur (1 ou 2) : 1
> reparer
Numero de l'ascenseur (1 ou 2) : 1
```

## Resultats et nettoyage

Chaque execution cree un fichier unique dans `resultats/`, par exemple
`resultats/statistiques_1731250000_12345.csv`. Les anciens resultats ne sont
pas ecrases.

```bash
make clean
```

Cette commande supprime l'executable et `build/`, mais conserve `resultats/`.

## Organisation du code

Les sources sont regroupees dans `src/` par responsabilite : `main.c` initialise
le programme ; `ascenseur.c` gere un ascenseur ; `utilisateur.c` genere les
demandes ; `gestionnaire.c` affecte les appels ; `ordonnancement.c` choisit un
ascenseur ; `communication.c` gere les files de messages ;
`synchronisation.c` gere la memoire partagee et les semaphores ; `alertes.c`
gere le tube ; `scenario.c` gere les pannes ; `statistiques.c` exporte le
bilan ; `immeuble.c` decrit les niveaux.

Les declarations sont dans `include/`, les objets temporaires dans `build/` et
les resultats d'execution dans `resultats/`.
