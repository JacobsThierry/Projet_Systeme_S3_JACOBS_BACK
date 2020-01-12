#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>



#define PORT 6000

#define MAX_BUFFER 1000

char lireCaractere();

int main(int argc , char const *argv[]) {
  int fdSocket;
  int nbRecu;
  struct sockaddr_in coordonneesServeur;
  int game=1;
  int longueurAdresse;
  char tampon[MAX_BUFFER];
  char tampon2[1];

  fdSocket = socket(AF_INET, SOCK_STREAM, 0);

  if (fdSocket < 0) {
    printf("socket incorrecte\n");
    exit(EXIT_FAILURE);
  }

  // On prépare les coordonnées du serveur
  longueurAdresse = sizeof(struct sockaddr_in);
  memset(&coordonneesServeur, 0x00, longueurAdresse);

  coordonneesServeur.sin_family = PF_INET;
  // adresse du serveur
  inet_aton("localhost", &coordonneesServeur.sin_addr);
  // toutes les interfaces locales disponibles
  coordonneesServeur.sin_port = htons(PORT);

  if (connect(fdSocket, (struct sockaddr *) &coordonneesServeur, sizeof(coordonneesServeur)) == -1) {
    printf("connexion impossible\n");
    exit(EXIT_FAILURE);
  }


  int lejeux = 1;

  printf("Bienvenue dans le jeu de pendu !\n\n");
  while(lejeux==1){
    game = 1;
    while(game==1){
      tampon[0]='\0';
      nbRecu = recv(fdSocket, tampon, MAX_BUFFER, 0); // on attend la réponse du serveur
      if (nbRecu > 0) {
        tampon[nbRecu] = 0;
        if(tampon[0]==11){//on vérifie si c'est gagne ou perdu ou en cours
        printf("Gagné ! Le mot etait bien : ");
        game=0;
        break;
      }else if(tampon[0]==12){
        printf("Perdu ! Le mot était : ");
        game=0;
        break;
      }else
      printf("Chance restante %d\n",tampon[0] );
    }
    tampon[0]='\0';
    nbRecu = recv(fdSocket, tampon, MAX_BUFFER, 0); // on attend la réponse du serveur
    if (nbRecu > 0) {
      tampon[nbRecu] = 0;
      printf("Mot à trouver: %s\n", tampon);
      tampon[0]='\0';
      printf("Quelle lettre voulez-vous chercher ? ");
      char c;
      c=lireCaractere();
      strcpy(tampon2,&c);
      int nbEnvoie = send(fdSocket, tampon2,1, 0);
    }
  }


  nbRecu = recv(fdSocket, tampon, MAX_BUFFER, 0); // on attend la réponse du serveur qui donne le bon mot
  printf("%s\n\n\n Voulez vous rejouer ? Y/N ", tampon);
  char choix [100];

  scanf("%s", choix );

  if(choix[0] == 'Y' || choix[0]=='y'){
    tampon[0] = 11;
    send(fdSocket, tampon, strlen(tampon), 0); //dis au serveur que l'on veut continuer
  }else{
    tampon[0] = 12;
    send(fdSocket, tampon, strlen(tampon), 0); //dis au serveur que l'on veut arrêter
    lejeux = 0;
  }

}


close(fdSocket);

return EXIT_SUCCESS;
}

char lireCaractere()
{


  char c[10];
  scanf("%s", &c );
  return toupper(c[0]); // On retourne le premier caractère qu'on a lu
}
