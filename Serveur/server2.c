#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "server2.h"
#include "client2.h"




groupe* LISTE_DE_GROUPES[100];
int compteurGroupes;

static void init(void)
{
loadGroups();
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         // concaténsation en (socklen_t *) : éviter le warning à compilation
         int csock = accept(sock, (SOCKADDR *)&csin, (socklen_t *) &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };

         char* name_mdp = malloc(sizeof(char)*BUF_SIZE);
         strncpy(name_mdp, buffer, BUF_SIZE - 1);


         connectClient(name_mdp, c, clients, &actual);
         
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               //printf("c:%i\r\n", c);
               //printf("bufferSUPER:%s\r\n", buffer);


               /* client disconnected */
               if(strcmp(buffer, "quit") == 0)
               {
                  printf("%s disconnected\r\n", client.name);
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  // pas besoin de préciser à tout les clients que (pseudo) c'est deconnecté
                  //send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               // Etape du choix de groupe
               // On mets "|choix-de-groupe|" dans discussionActuelle pour signifier que le client est en train de choisir le groupe de discussion
               else if(strcmp(client.discussionActuelle,"|choix-de-groupe|")==0 && strcmp(buffer,"home")!=0){
                  if(chercheGroupeParNom(buffer)!=NULL){
                     strncpy(client.discussionActuelle,buffer, BUF_SIZE-1);

                     char* message = malloc(sizeof(char)*BUF_SIZE);
                     strncpy(message,"Discussion choisie : ",BUF_SIZE);
                     strcat(message,client.discussionActuelle);
                     strcat(message,".\r\n");
                     strcat(message, "(home) pour retourner à l'accueil\r\n");
                     strcat(message, "(quit) pour retourner quitter la messagerie\r\n");

                     write_client(client.sock, message);

                     // Ecriture de l'historique chez le client concerne
                     FILE* fichierHistorique;
                     char * pathHistorique;
                     pathHistorique = malloc(sizeof(char)* BUF_SIZE);
                     strcpy(pathHistorique, "");
                     strcat(pathHistorique, "Data/Discussion/");
                     strcat(pathHistorique, client.discussionActuelle);
                     strcat(pathHistorique, ".txt");
                     char* buffer_historique;
                     buffer_historique = malloc(sizeof(char)* BUF_SIZE);
                     strcpy(buffer_historique,"");
                     strcat(buffer_historique, "****** DEBUT HISTORIQUE ");
                     strcat(buffer_historique, client.discussionActuelle);
                     strcat(buffer_historique, " ******\r\n");
                     // si fichier existe
                     if(fichierHistorique = fopen(pathHistorique,"r")){
                        char* buffer_historique_temp;
                        buffer_historique_temp = malloc(sizeof(char)* BUF_SIZE);
                        strcpy(buffer_historique_temp,"");
                        fgets(buffer_historique_temp, BUF_SIZE, fichierHistorique);
                        while(!(feof(fichierHistorique))){
                           strcat(buffer_historique, buffer_historique_temp);
                           fgets(buffer_historique_temp, BUF_SIZE, fichierHistorique);
                        }
                        fgets(buffer_historique, BUF_SIZE, fichierHistorique);
                        fclose(fichierHistorique);
                     }
                     // sinon si fichier n'existe pas
                     else{

                     }
                     strcat(buffer_historique, "******* FIN HISTORIQUE ");
                     strcat(buffer_historique, client.discussionActuelle);
                     strcat(buffer_historique, " *******\r\n");
                     strcat(buffer_historique, "Ecrivez votre prochain message ou mettez une instruction (home) ou (quit)\r\n");
                     write_client(client.sock, buffer_historique);
                     free(message);
                  }else{
                     write_client(client.sock, "Groupe non existant, veuillez entrer à nouveau le groupe choisi\r\n");
                  }
               }else if(strcmp(client.discussionActuelle,"|creation-de-groupe|")==0 && strcmp(buffer,"home")!=0){
                     strcpy(client.discussionActuelle,"|choix-membres|");
                     
                     groupe * g = malloc(sizeof(groupe));
                     LISTE_DE_GROUPES[compteurGroupes]=g;
                     compteurGroupes++;
                     strcpy(g->nom,buffer);
                     g->nombreDeMembres=1;
                     g->membres[0] = malloc(sizeof(char)*BUF_SIZE);
                     strcpy(g->membres[0], client.name);

                     write_client(client.sock, "\nVeuillez choisir le nom des participants au groupe en appuyant sur entrée pour valider chacun d'entre eux (pour valider, entrez 'finish'): \r\n");
                  

               }else if(strcmp(client.discussionActuelle,"|choix-membres|")==0 && strcmp(buffer,"home")!=0){

                     if(strcmp(buffer,"finish")!=0){
                        groupe * g = LISTE_DE_GROUPES[compteurGroupes-1];
                        g->membres[g->nombreDeMembres] = malloc(sizeof(char)*BUF_SIZE);
                        strcpy(g->membres[g->nombreDeMembres],buffer);
                        g->nombreDeMembres++;

                     }else{
                        
                        //write_client(client.sock, "\nGroupe créé ! Retour sur la page d'accueil.\r\n");
                        write_client(client.sock, "home");

                        if(LISTE_DE_GROUPES[compteurGroupes-1]->nombreDeMembres==2){
                           char* val = malloc(sizeof(char)*BUF_SIZE);
                           strcpy(val, client.name);
                           strcat(val, "-");
                           strcat(val, LISTE_DE_GROUPES[compteurGroupes-1]->membres[1]);
                           strcpy(LISTE_DE_GROUPES[compteurGroupes-1]->nom, val);
                        }

                        groupe * g = LISTE_DE_GROUPES[compteurGroupes-1];

                        FILE* fichierGroupes;
                        fichierGroupes = fopen("Data/list_groups.txt","a+");
                        fseek(fichierGroupes, SEEK_END, 1);
                        fputs(g->nom, fichierGroupes);
                        fputs(":", fichierGroupes);

                        int j;
                        for(j=0; j<g->nombreDeMembres; j++){
                           
                           fputs(g->membres[j], fichierGroupes);
                           fputs(";", fichierGroupes);
                        }

                        fputs("\n", fichierGroupes);
                        fclose(fichierGroupes);
                        strcpy(client.discussionActuelle,"");

                     }
                     // printf("debut écriture groupe\r\n");
                     // FILE* fichierGroupes;
                     // fichierGroupes = fopen("Data/list_groups/txt","a+");
                     // fputs(buffer, fichierGroupes);
                     // fputs(fichierGroupes);
                     // printf("fin écriture groupe\r\n");
                     // fclose(fichierGroupes);

               }
               else 
               {
                  if(strcmp(buffer,"list")==0){
                     write_client(client.sock, groupesDeMembre(client.name));
                     strncpy(client.discussionActuelle,"|choix-de-groupe|", BUF_SIZE-1);
                  }else if(strcmp(buffer,"create")==0){
                     write_client(client.sock, "\nVeuillez choisir le nom de votre groupe (ceci n'aura pas d'effet si le groupe ne comporte qu'une personne) : \r\n");
                     strncpy(client.discussionActuelle, "|creation-de-groupe|", BUF_SIZE-1);
                  }else if(strcmp(buffer,"home")==0){
                     strcpy(client.discussionActuelle, "");
                  }
                  else{
                     if(strcmp(client.discussionActuelle,"")!=0){
                        FILE* fichierDiscussion;
                        char * pathDiscussion;
                        pathDiscussion = malloc(sizeof(char)* BUF_SIZE);
                        strcat(pathDiscussion, "Data/Discussion/");
                        strcat(pathDiscussion, client.discussionActuelle);
                        strcat(pathDiscussion, ".txt");
                        fichierDiscussion = fopen(pathDiscussion,"a+");
                        char* buffer_discussion;
                        buffer_discussion = malloc(sizeof(char)* BUF_SIZE);
                        strcpy(buffer_discussion, "");
                        // Pourquoi pas essayer d'avoir date+l'horaire du message TODO
                        //fgets(buffer_discussion, BUF_SIZE, fichierDiscussion);
                        strcat(buffer_discussion, "[");
                        strcat(buffer_discussion, getHoraire());
                        strcat(buffer_discussion, "] ");
                        strcat(buffer_discussion, client.name);
                        strcat(buffer_discussion, " : ");
                        strcat(buffer_discussion, buffer);
                        strcat(buffer_discussion, "\r\n");
                        fputs(buffer_discussion, fichierDiscussion);
                        fclose(fichierDiscussion);
                        printf("sauvegarde dans l'historique du message de %s dans le groupe %s:\r\n%s", client.name, client.discussionActuelle, buffer_discussion);
                     }
                     // J'écris dans l'historique
                     // J'écris chez les autres si ils sont sur cette discussion
                     // je n'envoie plus a tout le monde comme un débilos 
                     // send_message_to_all_clients(clients, client, actual, buffer, 0);
                     if(strcmp(client.discussionActuelle,"")!=0){
                        printf("Message recu de |%s| à destination de |%s|\n", client.name, client.discussionActuelle);
                        send_message_to_clients_in_group(buffer, clients, actual, client);
                     }
                     else{
                        write_client(client.sock, "home");
                     }
                  }
               
               }
               break;


            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;
   int i;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   } 
   
   //sizeof message - strlen(message) - 1

   // strncat(buffer, "", sizeof buffer - strlen(buffer) - 1);
   // printf("buffer:|%s|\r\n", buffer);
   // puts(buffer);

   // if(strcmp(buffer, "salut") == 0){
   //    printf("salutbg \r\n");
   // }
   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void afficherGroupes(){
   int i;
   for(i=0;i<compteurGroupes;i++){
      printf("Groupe : %s\r\nMembres : ", LISTE_DE_GROUPES[i]->nom);
      int j;
      for(j=0; j<LISTE_DE_GROUPES[i]->nombreDeMembres; j++){
         printf(" |%s| ", LISTE_DE_GROUPES[i]->membres[j]);
      }
      printf("\r\n\r\n");
   }
}

static char* groupesDeMembre(char* membre){
   int i;
   int j;
   char* c = malloc(sizeof(char*)*BUF_SIZE);
   strcpy(c,"");
   strcat(c,"----------------------------------\r\n\r\n");
   for(i=0;i<compteurGroupes;i++){
      if(estMembre(membre, LISTE_DE_GROUPES[i])){
         if(LISTE_DE_GROUPES[i]->nombreDeMembres==2){
            strcat(c,"Discussion amicale : ");
            strcat(c,LISTE_DE_GROUPES[i]->nom);
            strcat(c,"\r\nMembres : ");
         }
         else{
            strcat(c,"Groupe : ");
            strcat(c,LISTE_DE_GROUPES[i]->nom);
            strcat(c,"\r\nMembres : ");
         }
         for(j=0; j<LISTE_DE_GROUPES[i]->nombreDeMembres; j++){
            
            if(strcmp(LISTE_DE_GROUPES[i]->membres[j],membre)!=0){
               strcat(c," |");
               strcat(c,LISTE_DE_GROUPES[i]->membres[j]);
               strcat(c,"| ");
            }
         }
         strcat(c,"\r\n\r\n");
      }
   }
   strcat(c,"-----------------------------------\r\n");
   strcat(c, "Ecrivez le groupe ou ami avec qui vous voulez communiquer\r\n");
   strcat(c, "(home) pour retourner à l'accueil\r\n");
   return c;
}

static bool estMembre(char* membre, groupe* grp){
   int j;
   for(j=0; j<grp->nombreDeMembres; j++){
      if(strcmp(membre,grp->membres[j])==0){
         return true;
      }
   }
   return false;
}

static groupe* chercheGroupeParNom(char* nomGroupe){
   int i;
   for(i=0; i<compteurGroupes; i++){
      if(strcmp(LISTE_DE_GROUPES[i]->nom,nomGroupe)==0){
         return LISTE_DE_GROUPES[i];
      }
   }
   return NULL;
}

static void send_message_to_clients_in_group(char* buffer, Client* clients, int actuel, Client client){
   int i;
   
   char* message = malloc(sizeof(char)*BUF_SIZE);
   strcpy(message, "");
   strncat(message,"[", BUF_SIZE - 1);
   strncat(message, getHoraire(), BUF_SIZE - 1);
   strncat(message,"] ", BUF_SIZE - 1);
   strncat(message, client.name, BUF_SIZE - 1);
   strncat(message, " : ", BUF_SIZE - 1);
   strncat(message, buffer, BUF_SIZE - 1);


   for(i=0; i<actuel; i++){
      if(strcmp(client.discussionActuelle,"")!=0 && strcmp(clients[i].name, client.name)!=0 && strcmp(clients[i].discussionActuelle, client.discussionActuelle)==0){
         write_client(clients[i].sock, message);
      }
   }

   free(message);
}

static char * getHoraire(){
   struct sockaddr_in serv_addr_time;
   bzero((char *) &serv_addr_time, sizeof(serv_addr_time));
   serv_addr_time.sin_family           = AF_INET;
   serv_addr_time.sin_addr.s_addr      = inet_addr(IP_TIME);
   serv_addr_time.sin_port             = htons(atoi(PORT_TIME));

   int sockfd_time;
   if((sockfd_time = socket(AF_INET, SOCK_STREAM, 0))<0){
      printf("Erreur lecture horaire"); 
      return "";
   }
   if(connect(sockfd_time, (struct sockaddr*)&serv_addr_time, sizeof(serv_addr_time))<0){
      printf("Erreur lecture horaire"); 
      return "";
   }

   long int lng;
   read(sockfd_time, &lng, 8);
   long int lng2 = ntohl(lng);
   time_t timestamp = time (&lng2);
   close(sockfd_time);
   char * tempsString = ctime(&timestamp);
   tempsString[strlen(tempsString)-1] ='\0';
   return tempsString;
}

static void loadGroups(){
   printf("Start initialisation\r\n");

FILE* fichier;
fichier = fopen("Data/list_groups.txt","r");
char* buffer_group;
buffer_group = malloc(sizeof(char)* BUF_SIZE);
if(fichier!=NULL){
   char c;
   compteurGroupes = 0;

   int compteurMembres;
   
   // crée le groupe
   c = fgetc(fichier);
   while(c!=EOF){


      groupe* g = malloc(sizeof(groupe));
      // retour à 0 au retour à la ligne
      strcpy(buffer_group,"");
      compteurMembres = 0;
      while(c!='\n'){
         //c = fgetc(fichier);
         //printf("%c",c);
         if(c==':'){
            // nom du groupe
            strcpy(g->nom, buffer_group);
            compteurGroupes++;
            strcpy(buffer_group,"");
            c = fgetc(fichier);
         }
         if(c==';'){
            // nom de l'utilisateur a ajouter au groupe
            g->membres[compteurMembres] = malloc(sizeof(buffer_group));
            strcpy(g->membres[compteurMembres],buffer_group);
            compteurMembres++;
            strcpy(buffer_group,"");
         }
         else{
            //printf("\n|BUFFER CAT AVANT : %s |\n",buffer_group);
            strncat(buffer_group, &c, 1);
            //  buffer_group[strlen(buffer_group)] = c;
            //printf("\n|BUFFER CAT APRES : %s |\n",buffer_group);
         }
         c = fgetc(fichier);
      }
      //passe le retour a la ligne normalement
      c = fgetc(fichier);
      g->nombreDeMembres = compteurMembres;
      //ajoute le groupe à la liste de groupes
      LISTE_DE_GROUPES[compteurGroupes-1] = g;
      

   }

   //afficherGroupes();
}
   printf("End initialisation\r\n");
}

static void connectClient(char* name_mdp, Client c, Client* clients, int* pntactual){
   int actual = *pntactual;
   int j;
   char* name = malloc(sizeof(char)*BUF_SIZE);
   strcpy(name,"");
   char* mdp = malloc(sizeof(char)*BUF_SIZE);
   strcpy(mdp,"");
   char* charSepar = ":";
   for(j=0; j<strlen(name_mdp); j++){
      if(name_mdp[j]==':'){
         break;
      }
      //strcat(name, name_mdp[j]);
      name[j]=name_mdp[j];
      name[j+1]='\0';
   }
   int k=-1;
   while(j<strlen(name_mdp)){
      j++;
      k++;
      //strcat(mdp, name_mdp[j]);
      mdp[k]=name_mdp[j];
      mdp[k+1]='\0';
   }

   FILE* fichierlogin;
   char* buffer_login;
   buffer_login = malloc(sizeof(char)* BUF_SIZE);
   char* buffer_login_temp;
   buffer_login_temp = malloc(sizeof(char)* BUF_SIZE);
   strcpy(buffer_login_temp, "");
   // si fichier existe
   if(fichierlogin = fopen("Data/login_mdp.txt","r")){
      fgets(buffer_login_temp, BUF_SIZE, fichierlogin);
      int mauvaisMDP=0;
      int connect=0;
      while(!(feof(fichierlogin))){
         if(strlen(buffer_login_temp)!=0){
            buffer_login_temp[strlen(buffer_login_temp)-1]= '\0';
         }
         if(strcmp(buffer_login_temp, name_mdp)==0){
            //CONNECT

            connect=1;
            break;
            // strncpy(c.name, name, BUF_SIZE - 1);
            // c.discussionActuelle = malloc(sizeof(char)*BUF_SIZE);
            // clients[actual] = c;
            // actual++;
            // printf("Client n°%d nom %s connecte\n", actual-1, clients[actual-1].name);
         }else{
            char* nomlogin = malloc(sizeof(char)*BUF_SIZE);
            strcpy(nomlogin, "");
            for(j=0; j<strlen(buffer_login_temp); j++){
               if(buffer_login_temp[j]==':'){
                  break;
               }
               nomlogin[j]=buffer_login_temp[j];
               nomlogin[j+1]='\0';
            }
            if(strcmp(nomlogin,name)==0){
               mauvaisMDP=1;
               break;
            }
         }


         strcat(buffer_login, buffer_login_temp);
         fgets(buffer_login_temp, BUF_SIZE, fichierlogin);
      }
      fgets(buffer_login, BUF_SIZE, fichierlogin);
      fclose(fichierlogin);


      if(mauvaisMDP==1){
         printf("Client nom %s mauvais mdp\n",name);
         write_client(c.sock,"Mauvais login/mot de passe\r\n");
         closesocket(c.sock);
      }else if(connect==1){

         strncpy(c.name, name, BUF_SIZE - 1);
         c.discussionActuelle = malloc(sizeof(char)*BUF_SIZE);
         clients[actual] = c;
         actual++;
         printf("Client n°%d nom %s connecte\n", actual-1, clients[actual-1].name);
         *pntactual=actual;
      }else{

         fichierlogin = fopen("Data/login_mdp.txt","a+");
         fputs(name_mdp, fichierlogin);
         fputs("\n", fichierlogin);
         fclose(fichierlogin);
         
         strncpy(c.name, name, BUF_SIZE - 1);
         c.discussionActuelle = malloc(sizeof(char)*BUF_SIZE);
         clients[actual] = c;
         actual++;
         printf("Client n°%d nom %s cree\n", actual-1, clients[actual-1].name);
         *pntactual=actual;
      }
   }






   // strncpy(c.name, buffer, BUF_SIZE - 1);
   // c.discussionActuelle = malloc(sizeof(char)*BUF_SIZE);
   // clients[actual] = c;
   // actual++;
   // printf("Client n°%d nom %s connecte\n", actual-1, clients[actual-1].name);
}



int main(int argc, char **argv)
{

   init();

   app();

   end();

   return EXIT_SUCCESS;
}
