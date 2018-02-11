/* --------------------------------------------------------------------------*/
/* Dossier 1 - PRODUCTEUR CONSOMMATEUR */
/* Reggers Eve - Sepulchre Cl�ment (2226) */
/* Date de cr�ation : 08/02/2018 13:11 */
/* Derni�re modification : 09/02/2018 12:10 */
/* --------------------------------------------------------------------------*/


/* --------------------------------------------------------------------------*/
/* D�claration des librairies : */
/* --------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

/* --------------------------------------------------------------------------*/
/* D�claration des variables globales : */
/* --------------------------------------------------------------------------*/

//Permet de ralentir les producteurs, d�lai de x microsecondes � chaque production. Indiquer 0 pour d�sactiver.
#define DELAI 1
//Taille du buffer.
#define TAILLE_BUFFER 20
//Nombre d'it�rations pour chaque production de chaque producteur.
#define ITERATIONS 30

/* --------------------------------------------------------------------------*/
/* Structure de la m�moire partag�e : indices, buffer et s�maphores : */
/* --------------------------------------------------------------------------*/

struct StruSemaphore
{
	//Indice de la prochaine lettre consomm�e.
	size_t iConsNextVal;
	//Indice de la prochaine lettre produite.
	size_t iProdNextVal;
	char cBuffer[TAILLE_BUFFER];
	//S�maphore comptant le nombre de places libres.
	sem_t semEscapeLibre;
	//S�maphore comptant le nombre de lettres produites.
	sem_t semProduit;
	//S�maphore pour l'acc�s exclusif au buffer.
	sem_t semBufferAccess;
};
struct StruSemaphore *struSem;

/* --------------------------------------------------------------------------*/
/* D�claration des fonctions utilis�es : */
/* --------------------------------------------------------------------------*/

//Fonction ex�cut�e par le consommateur.
void BoucleConsommateur(void);
//Fonction ex�cut�e par les producteurs.
void BoucleProducteur(char);
//Fonction affichant l'activit� du producteur et du consommateur.
void Affichage (void);


/* --------------------------------------------------------------------------*/
/* Fonction main : */
/* --------------------------------------------------------------------------*/

int main()
{
	int iErreur; //Indique une erreur lors de la cr�ation d'un IPC
	int i;

	//Initialisation des IPC :
	struSem=mmap(NULL, sizeof(struct StruSemaphore), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

	/*
	mmap - �tablir une projection en m�moire des fichiers ou des p�riph�riques  
	
			NULL -> le noyau choisit l'adresse � laquelle cr�er la projection
			sizeof(struct StruSemaphore)
			PROT_READ | PROT_WRITE -> indique la protection m�moire que l'on d�sire pour cette projection
			MAP_ANONYMOUS | MAP_SHARED -> d�termine si les modifications de la projection sont visibles par les autres processus projetant la m�me r�gion et si les modifications sont appliqu�es au fichier sous-jacent
	*/
	
	
	//Gestion d'erreurs :
	iErreur=sem_init(&struSem->semEscapeLibre, 1, TAILLE_BUFFER);
	if (iErreur)
	{
		perror("Erreur durant l'initialisation du semaphore 'semEscapeLibre'");
		return EXIT_FAILURE;
	}

	/*
	sem_init - Initialiser un s�maphore non nomm�  
	
			&struSem->semEscapeLibre -> s�maphore non nomm� initialis� � cette adresse
			1 -> le s�maphore est partag� entre processus et devrait �tre situ� dans une r�gion de m�moire partag�e (on fait des fork dans la suite ! )
			TAILLE_BUFFER -> sp�cifie la valeur initiale du s�maphore. Comme le producteur n'a pas commenc� � produire, tout est libre.
	*/
	
	
	iErreur=sem_init(&struSem->semProduit, 1, 0); // 0 -> Les producteurs n'ont encore rien produit ! Ils incr�menteront ceci � chaque production.
	if (iErreur)
	{
		perror("Erreur durant l'initialisation du semaphore 'semProduit'");
		return EXIT_FAILURE;
	}
	

	iErreur=sem_init(&struSem->semBufferAccess, 1, 1); // 1 -> Acc�s au buffer autoris� tant que la s�maphore est � 1
	if (iErreur)
	{
		perror("Erreur durant l'initialisation du semaphore 'semBufferAccess'");
		return EXIT_FAILURE;
	}

	//Initialisation du cBuffer et des NextVal (indices) pour le consommateur et les producteurs.
	struSem->iConsNextVal=0;
	struSem->iProdNextVal=0;
	for (i=0;i<TAILLE_BUFFER;i++)
		struSem->cBuffer[i]='P'; //P convention de l'�nonc�. L'on d�marre avec des 'P' partout

	
	//Cr�ation des processus consommateur et producteur.
	
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
	wait();   // FIN des trois forks avant de d�sallouer la m�moire partag�e et terminer le programme
	wait();
		
		
	
	if( munmap(struSem, sizeof(struct StruSemaphore))// D�sallocation de la m�moire partag�e
	{
			printf("Probl�me lors de la d�sallocation de la m�moire partag�e.");
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
		//Verrouillage des s�maphores.
		sem_wait(&struSem->semProduit);     // On "consomme" donc on d�cr�mente la production. NE PEUT PAS CONSOMMER SI LA PRODUCTION EST A ZERO !! Tant qu'il n'y a pas de production on attend.
		sem_wait(&struSem->semBufferAccess); // On bloque l'acces au buffer car on va �crire dedans. On attend tant que quelqu'un occupe d�j� le buffer.

		struSem->cBuffer[struSem->iConsNextVal]='&';    
		struSem->iConsNextVal++;
		struSem->iConsNextVal %= TAILLE_BUFFER;     // Cette ligne Made in Eve est brillante bordel.
		Affichage();

		//D�verrouillage des s�maphores.
		sem_post(&struSem->semBufferAccess); // On d�bloque l'acc�s au buffer pour le suivant
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

		//Verrouillage des s�maphores.
		sem_wait(&struSem->semEscapeLibre); // Vu que l'on va produire, l'on prend un espace libre. Tant qu'il n'y a pas d'espace libre, on attend ! Possible si les producteurs sont beaucoup trop rapides et que tout le buffer est rempli de production !
		sem_wait(&struSem->semBufferAccess); // On bloque l'acces au buffer car on va �crire dedans

		struSem->cBuffer[struSem->iProdNextVal] = cLettre;
		struSem->iProdNextVal++;								// L'on produit dans le buffer
		struSem->iProdNextVal%=TAILLE_BUFFER;
		Affichage();

		//D�verrouillage des s�maphores :
		sem_post(&struSem->semBufferAccess);  // On d�bloque l'acc�s au buffer pour le suivant
		sem_post(&struSem->semProduit);       // Incr�mentation de la production. Tant que production > 0, le consommateur pourra consommer.

		cLettre++;
		//V�rification de la lettre :
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
