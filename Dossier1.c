/* --------------------------------------------------------------------------*/
/* Dossier 1 - PRODUCTEUR CONSOMMATEUR */
/* Reggers Eve - Sepulchre Clément (2226) */
/* Date de création : 08/02/2018 13:11 */
/* Dernière modification : 09/02/2018 12:10 */
/* --------------------------------------------------------------------------*/


/* --------------------------------------------------------------------------*/
/* Déclaration des librairies : */
/* --------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

/* --------------------------------------------------------------------------*/
/* Déclaration des variables globales : */
/* --------------------------------------------------------------------------*/

//Permet de ralentir les producteurs, délai de x microsecondes à chaque production. Indiquer 0 pour désactiver.
#define DELAI 1
//Taille du buffer.
#define TAILLE_BUFFER 20
//Nombre d'itérations pour chaque production de chaque producteur.
#define ITERATIONS 30

/* --------------------------------------------------------------------------*/
/* Structure de la mémoire partagée : indices, buffer et sémaphores : */
/* --------------------------------------------------------------------------*/

struct StruSemaphore
{
	//Indice de la prochaine lettre consommée.
	size_t iConsNextVal;
	//Indice de la prochaine lettre produite.
	size_t iProdNextVal;
	char cBuffer[TAILLE_BUFFER];
	//Sémaphore comptant le nombre de places libres.
	sem_t semEscapeLibre;
	//Sémaphore comptant le nombre de lettres produites.
	sem_t semProduit;
	//Sémaphore pour l'accès exclusif au buffer.
	sem_t semBufferAccess;
};
struct StruSemaphore *struSem;

/* --------------------------------------------------------------------------*/
/* Déclaration des fonctions utilisées : */
/* --------------------------------------------------------------------------*/

//Fonction exécutée par le consommateur.
void BoucleConsommateur(void);
//Fonction exécutée par les producteurs.
void BoucleProducteur(char);
//Fonction affichant l'activité du producteur et du consommateur.
void Affichage (void);


/* --------------------------------------------------------------------------*/
/* Fonction main : */
/* --------------------------------------------------------------------------*/

int main()
{
	int iErreur; //Indique une erreur lors de la création d'un IPC
	int i;

	//Initialisation des IPC :
	struSem=mmap(NULL, sizeof(struct StruSemaphore), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

	/*
	mmap - Établir une projection en mémoire des fichiers ou des périphériques  
	
			NULL -> le noyau choisit l'adresse à laquelle créer la projection
			sizeof(struct StruSemaphore)
			PROT_READ | PROT_WRITE -> indique la protection mémoire que l'on désire pour cette projection
			MAP_ANONYMOUS | MAP_SHARED -> détermine si les modifications de la projection sont visibles par les autres processus projetant la même région et si les modifications sont appliquées au fichier sous-jacent
	*/
	
	
	//Gestion d'erreurs :
	iErreur=sem_init(&struSem->semEscapeLibre, 1, TAILLE_BUFFER);
	if (iErreur)
	{
		perror("Erreur durant l'initialisation du semaphore 'semEscapeLibre'");
		return EXIT_FAILURE;
	}

	/*
	sem_init - Initialiser un sémaphore non nommé  
	
			&struSem->semEscapeLibre -> sémaphore non nommé initialisé à cette adresse
			1 -> le sémaphore est partagé entre processus et devrait être situé dans une région de mémoire partagée (on fait des fork dans la suite ! )
			TAILLE_BUFFER -> spécifie la valeur initiale du sémaphore. Comme le producteur n'a pas commencé à produire, tout est libre.
	*/
	
	
	iErreur=sem_init(&struSem->semProduit, 1, 0); // 0 -> Les producteurs n'ont encore rien produit ! Ils incrémenteront ceci à chaque production.
	if (iErreur)
	{
		perror("Erreur durant l'initialisation du semaphore 'semProduit'");
		return EXIT_FAILURE;
	}
	

	iErreur=sem_init(&struSem->semBufferAccess, 1, 1); // 1 -> Accès au buffer autorisé tant que la sémaphore est à 1
	if (iErreur)
	{
		perror("Erreur durant l'initialisation du semaphore 'semBufferAccess'");
		return EXIT_FAILURE;
	}

	//Initialisation du cBuffer et des NextVal (indices) pour le consommateur et les producteurs.
	struSem->iConsNextVal=0;
	struSem->iProdNextVal=0;
	for (i=0;i<TAILLE_BUFFER;i++)
		struSem->cBuffer[i]='P'; //P convention de l'énoncé. L'on démarre avec des 'P' partout

	
	//Création des processus consommateur et producteur.
	
	pid_t child_pid=fork();
	if (!child_pid) 				//Processus consommateur.
	{
		BoucleConsommateur();
		exit(1);
	}
	
	child_pid=fork();
	if (!child_pid) 				//Processus producteur 1 
	{
		BoucleProducteur('A');
		exit(1);
	}
	
	child_pid=fork(); 				//Processus producteur 2
	if(!child_pid)
	{
		BoucleProducteur('a');
		exit(1);
	}
	
	wait(); 
	wait();   // FIN des trois forks avant de désallouer la mémoire partagée et terminer le programme
	wait();
		
		
	
	if( munmap(struSem, sizeof(struct StruSemaphore))// Désallocation de la mémoire partagée
	{
			printf("Problème lors de la désallocation de la mémoire partagée.");
			exit EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

/* --------------------------------------------------------------------------*/
/* Fonction BoucleConsommateur */
/* --------------------------------------------------------------------------*/

void BoucleConsommateur(void)
{
	int i;
	for (i=0;i<ITERATIONS*2;i++)
	{
		//Verrouillage des sémaphores.
		sem_wait(&struSem->semProduit);     // On "consomme" donc on décrémente la production. NE PEUT PAS CONSOMMER SI LA PRODUCTION EST A ZERO !! Tant qu'il n'y a pas de production on attend.
		sem_wait(&struSem->semBufferAccess); // On bloque l'acces au buffer car on va écrire dedans. On attend tant que quelqu'un occupe déjà le buffer.

		struSem->cBuffer[struSem->iConsNextVal]='&';    
		struSem->iConsNextVal++;
		struSem->iConsNextVal %= TAILLE_BUFFER;     // Cette ligne Made in Eve est brillante bordel.
		Affichage();

		//Déverrouillage des sémaphores.
		sem_post(&struSem->semBufferAccess); // On débloque l'accès au buffer pour le suivant
		sem_post(&struSem->semEscapeLibre);  // Vu que l'on vient de consommer, il y a un nouvel espace libre dans le buffer que le producteur peut remplir.
	}
}

/* --------------------------------------------------------------------------*/
/* Fonction BoucleProducteur */
/* --------------------------------------------------------------------------*/

void BoucleProducteur(char cLettreBase)
{
	int i;
	char cLettre = cLettreBase;
	for (i=0;i<ITERATIONS;i++)
	{
		if(DELAI>0) //Ralentit les producteurs
			usleep(DELAI);

		//Verrouillage des sémaphores.
		sem_wait(&struSem->semEscapeLibre); // Vu que l'on va produire, l'on prend un espace libre. Tant qu'il n'y a pas d'espace libre, on attend ! Possible si les producteurs sont beaucoup trop rapides et que tout le buffer est rempli de production !
		sem_wait(&struSem->semBufferAccess); // On bloque l'acces au buffer car on va écrire dedans

		struSem->cBuffer[struSem->iProdNextVal] = cLettre;
		struSem->iProdNextVal++;								// L'on produit dans le buffer
		struSem->iProdNextVal%=TAILLE_BUFFER;
		Affichage();

		//Déverrouillage des sémaphores :
		sem_post(&struSem->semBufferAccess);  // On débloque l'accès au buffer pour le suivant
		sem_post(&struSem->semProduit);       // Incrémentation de la production. Tant que production > 0, le consommateur pourra consommer.

		cLettre++;
		//Vérification de la lettre :
		if (cLettre>cLettreBase+25)
			cLettre=cLettreBase;
	}
}

/* --------------------------------------------------------------------------*/
/* Fonction Affichage. */
/* --------------------------------------------------------------------------*/

void Affichage(void)
{
	int i;
	for (i=0;i<TAILLE_BUFFER;i++)
		printf("%c ", struSem->cBuffer[i]);
	printf("\n");
}
