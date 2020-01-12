#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>


#define PORT 6000
#define MAX_BUFFER 1000
#define MAX_USER 2
#define boolean int
#define false 0
#define true 1
#define nb_vies 9
#define nb_parties 10

typedef struct{
  boolean fini; //boolean qui indique l'état de la partie
  char mot[100]; //le mot a trouver
  int vies; //les vies réstantes


  char lettres[100]; //les letrres données par l'utilisateur

  char ad_client[100]; //ad de l'utilisateur
  int port; // port

  boolean utiliser; //boolen qui dis si la structure est utiliser ou non

  int index; // index du tableau
  int fdSocketAttente;
  int fdSocketCommunication;
pthread_mutex_t mutex;
}partie;

partie parties[nb_parties]; //on crée le tableau ou seront stocker les parties
pthread_t threads[nb_parties]; //on crée le tableau ou seront stocker les threads. Chaque thread a sa partie associer.

void selectMot(char motretourne[100]){
  FILE* dico;
  int nbmots = 0, nbmot=0, caractereLu = 0;
  dico = fopen("liste_mot.txt", "r");

  if(dico==NULL){
    return;
  }
  do{
    caractereLu = fgetc(dico);
    if(caractereLu=='\n') nbmot++;
  }while(caractereLu != EOF);

  srand(time(NULL));

  nbmot = rand()%nbmot;

  rewind(dico);
  while(nbmot > 0){
    caractereLu=fgetc(dico);
    if(caractereLu=='\n') nbmot--;
  }

  fgets(motretourne, 100, dico);

  motretourne[strlen(motretourne)-1] = '\0';
  fclose(dico);
}

void create_partie(partie* p){
  p->fini = true;
  p->utiliser = false;
  pthread_mutex_init(&p->mutex, NULL);
  p->port = -1;
}

void init_partie(partie* p){
  pthread_mutex_lock(&p->mutex);
  p->fini = false;
  selectMot(p->mot);
  p->vies = nb_vies;
  pthread_mutex_unlock(&p->mutex);
}


void afficher_partie(partie *p){//Affiche les infos d'une partie
  pthread_mutex_lock(&p->mutex);
  if(!p->fini){
    printf("%s", p->ad_client);
    printf("\t\t");
    printf("%d", p->vies );
    printf("\t\t\t\t");
    printf("%s\n", p->mot );

  }
  pthread_mutex_unlock(&p->mutex);
}

void *afficher_parties(void *arg){//Boucle sur le tableau partie pour trouver des parties en cours et les afficher
  partie *partiess = (partie *) arg;
  char input[32];
  while(1){
  fgets(input,sizeof(input),stdin);
  if(input[0]=='\n'){
    printf("client \t\t nombre de vies restantes \t\t mot\n");
    for(int i=0;i<nb_parties;i++){
      afficher_partie(&partiess[i]);
    }
  }
}
}



int index_dispo(partie p[nb_parties]){ //retourne un index de partie ou aucune partie n'est actuellement jouer, retourne -1 si aucun slot n'est libre
  for(int i=0; i<nb_parties;i++) if(p[i].port == -1) return i;
  return -1;

}


int chercheLettre(char lettre,char mottrouvee[],char message[]){
  int bool = 0;
  for(int i=0;i<strlen(mottrouvee);i++){
    if(toupper(mottrouvee[i])==lettre){
      message[i]=lettre;
      bool=1;
    }
  }
  return bool;
}

int gagne(char mot[]){//vérifie si le mot a été trouvé
  int var =1;
  for(int i=0;i<strlen(mot);i++){
    if(mot[i]=='*'){
      var =0;
      break;
    }

  }
  return var;
}


void *jeu(void *arg){
  char tampon[MAX_BUFFER];
  char tampon2[MAX_BUFFER];
  int nbRecu;
  int trouvee;
  partie *p = (partie*) arg;
  int contin = 1;

    while(contin){
      init_partie(p);
      char message[15]; //message qui sera envoyer au Client
      for (int i=0;i<=15;i++){
        message[i]='\0'; //on vide le message
      }
      for(int i=0;i<strlen(p->mot)-1;i++){
        message[i]='*';
      }

      while(!p->fini){
        tampon[0] = p->vies;

        send(p->fdSocketCommunication, tampon, strlen(tampon), 0); //on envoi le nombre de vies réstantes au client
        usleep(800);
        strcpy(tampon,message);
        send(p->fdSocketCommunication, tampon, strlen(tampon), 0); //on envoi au client le mot du pendu
        nbRecu = recv(p->fdSocketCommunication, tampon2, MAX_BUFFER, 0); // on attend la réponse du client

        if (nbRecu > 0) {
          tampon2[nbRecu] = 0;
          char lettre = tampon2[0];
          int res=chercheLettre(lettre,p->mot,message);//On chercher si la lettre est dans le mot
          pthread_mutex_lock(&p->mutex);
          if(res==false) p->vies--;
          pthread_mutex_unlock(&p->mutex);
          pthread_mutex_lock(&p->mutex);
          if(p->vies == 0) p->fini = true;
          pthread_mutex_unlock(&p->mutex);
          trouvee = gagne(message);//Si le mot est trouvé, la partie est gagnée

          pthread_mutex_lock(&p->mutex);
          if(trouvee==1) p->fini = true;
          pthread_mutex_unlock(&p->mutex);


        }
      }

        if(trouvee==1) tampon[0]=11;//On envoie 11 si le joueur a gagné ou 12 si il a perdu (pour afficher le bon message)
        else tampon[0]=12;
        send(p->fdSocketCommunication, tampon, strlen(tampon), 0);
        strcpy(tampon,p->mot);

        usleep(800);
        send(p->fdSocketCommunication, tampon, strlen(tampon), 0);

        recv(p->fdSocketCommunication, tampon2, MAX_BUFFER, 0); // on attend la réponse du client, si il veut continuer ou non
        if(tampon2[0] == 12){//si le client ne veut plus rejoeur, on arrete
          contin = 0;
        }

  }
    create_partie(&parties[p->index]);


  }


int main(int argc, char const *argv[]) {

  for (int i = 0; i < nb_parties; i++) {
    create_partie(&parties[i]);
  }



  int fdSocketAttente;
  int fdSocketCommunication;

  struct sockaddr_in coordonneesServeur;
  struct sockaddr_in coordonneesAppelant;
  int nbRecu;
  int longueurAdresse;
  int pid;


    fdSocketAttente = socket(PF_INET, SOCK_STREAM, 0);

    if (fdSocketAttente < 0) {
      printf("socket incorrecte\n");
      exit(EXIT_FAILURE);
    }

    // On prépare l’adresse d’attachement locale
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&coordonneesServeur, 0x00, longueurAdresse);

    coordonneesServeur.sin_family = PF_INET;
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_port = htons(PORT);


    if (bind(fdSocketAttente, (struct sockaddr *) &coordonneesServeur, sizeof(coordonneesServeur)) == -1) {
      printf("erreur de bind\n");
      exit(EXIT_FAILURE);
    }

    if (listen(fdSocketAttente, 5) == -1) {
      printf("erreur de listen\n");
      exit(EXIT_FAILURE);
    }

    printf("En attente de connexion...\n");
    printf("Pour voir les clients connectés et leurs status, appuyez sur Entrée\n");

socklen_t tailleCoord = sizeof(coordonneesAppelant);

pthread_t thread_affichage;
pthread_create(&thread_affichage,NULL,afficher_parties,(void*)&parties);//Thread pour afficher les parties


while(1){
  if ((fdSocketCommunication = accept(fdSocketAttente, (struct sockaddr *) &coordonneesAppelant,
  &tailleCoord)) == -1) {
    printf("erreur de accept\n");
    exit(EXIT_FAILURE);
  }

  int cmp=0;
    printf("Client connecté - %s:%d\n",
             inet_ntoa(coordonneesAppelant.sin_addr),
             ntohs(coordonneesAppelant.sin_port));

  int fd1 = fdSocketAttente;
  int fd2 =fdSocketCommunication;

////////////////////////// on initialise la partie :



int index_d = index_dispo(parties);


while(index_d == -1){
  printf("Erreur lors de la création de partie : aucun emplacement de partie n'est disponible\n");
  index_d = index_dispo(parties);
  sleep(5);
}

strcpy(parties[index_d].ad_client, (char*) inet_ntoa(coordonneesAppelant.sin_addr));
pthread_mutex_lock(&parties[index_d].mutex);
parties[index_d].port = coordonneesAppelant.sin_port;
parties[index_d].fdSocketAttente =fd1;
parties[index_d].index =index_d;
parties[index_d].fdSocketCommunication=fd2;
pthread_mutex_unlock(&parties[index_d].mutex);
int ret;
ret = pthread_create(&threads[index_d],NULL,jeu,(void*)&parties[index_d]);




}

  return 0;
}
