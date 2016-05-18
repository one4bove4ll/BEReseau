//Pour le moment l'établissement de connexion fonctionne bien
//La fiabilité partielle semble fonctionner, il faut la tester avec la video
//Cependant un bug persiste, pour le moment nous sommes obligés de 
//créer les pertes après la connexion sinon ce la fonctionne pas

#include <mictcp.h>
#include <api/mictcp_core.h>

int setting = 1;
mic_tcp_sock *tab_sock[1];
mic_tcp_sock sock ;
mic_tcp_sock socket_distant ;
unsigned int num_sequence=0;
unsigned int num_ack = 0 ;
unsigned int timer = 1000 ;
unsigned int nb_msg_envoye = 0 ;
unsigned int nb_msg_perdu = 0 ;
int perte_acceptee = 40 ;



int mic_tcp_socket(start_mode sm) 
// Permet de crÃ©er un socket entre lâapplication et MIC-TCP// Retourne le descripteur du socket ou bien -1 en cas d'erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  initialize_components(sm); // Appel obligatoire
  sock.fd = 0 ;
  sock.state = IDLE ;
  tab_sock[0]=&sock ;
  return tab_sock[0]->fd ;
}


int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
// Permet dâattribuer une adresse Ã  un socket. Retourne 0 si succÃšs, et -1 en cas dâÃ©chec
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  if(socket == (tab_sock[socket]->fd)) {
    tab_sock[socket]->addr = addr;
    return 0 ;
  }else {
    return -1 ;
  }
}


int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) //ici il faut faire des choses
// Met lâapplication en Ã©tat d'acceptation dâune requÃªte de connexion entrante
// Retourne 0 si succÃšs, -1 si erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

  while (tab_sock[socket]->state!=ESTABLISHED){
    sleep(1); // listen
  }

  return 0;
}




int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) //ici il faut faire des choses
// Permet de rÃ©clamer lâÃ©tablissement dâune connexion
// Retourne 0 si la connexion est Ã©tablie, et -1 en cas dâÃ©chec
{ 
  
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");    // Seul la source peut demander la connexion 

  int lg_adresse= sizeof(mic_tcp_sock_addr);
  mic_tcp_pdu SYN={{tab_sock[socket]->addr.port,socket_distant.addr.port,num_sequence,0,1,0,0},{"",0}};  // Construction du SYN
  mic_tcp_pdu SYNACK;
  mic_tcp_pdu ACK={{tab_sock[socket]->addr.port,socket_distant.addr.port,num_sequence,0,0,1,0},{"",0}};
  tab_sock[socket]->addr = addr ;   // implÃ©mentation du socket distant 
  SYNACK.payload.data=malloc(15);
  SYNACK.payload.size=15;
 
  // Socket distant initialisé
  socket_distant.state=IDLE;

  while(tab_sock[socket]->state !=ESTABLISHED){
    SYN.hd.seq_num = perte_acceptee ;
    if (IP_send(SYN,socket_distant.addr)>=0){ // Envoi SYN
      tab_sock[socket]->state=SYN_SENT;
      sleep(1);
      printf("---Envoi du SYN \n");
     
     
      if (IP_recv(&(SYNACK.payload),&socket_distant.addr,timer)>=0) // Réception SYNACK
	{
	  SYNACK.hd=get_header(SYNACK.payload.data);
	  perte_acceptee = SYNACK.hd.seq_num ; 
	  if(SYNACK.hd.syn==1 && SYNACK.hd.ack==1){ // Vérification SYNACK
	    printf("---SYNACK reçu \n");
	    // ACK.hd.seq_num = perte_acceptee ;
	    if (IP_send(ACK,socket_distant.addr)<0){ // Envoi ACK 1 seule fois. (voir la gestion de la perte de l'ACK dans process_recv_pdu)
	      printf("---Erreur envoi SYN (IP_send)\n");
	    }else{
	      printf("---Envoi d'un ACK\n");
	      tab_sock[socket]->state=ESTABLISHED;  // Connexion Ã©tablie
	      printf("Connexion etablie !!\n");
	      printf("La perte acceptée est : %d \n",perte_acceptee);
	    }
	  }
	}

    } 
  } 
  set_loss_rate(300); // !!!! Il faut le déplacer car l peut y avoir un problème en cas de perte du ACK
  return 0 ;

}


int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
// Permet de rÃ©clamer lâenvoi dâune donnÃ©e applicative
// Retourne la taille des donnÃ©es envoyÃ©es, et -1 en cas d'erreur
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  //construction de PDU
  
  mic_tcp_pdu pdu ;
  int envoi_ok = 0 ;
  mic_tcp_header ACK_recu;
  mic_tcp_payload data_recu;
  data_recu.data = malloc(15);
  data_recu.size = 15 ;
  int erreur ;

  // envoi comme en dessous
  //mic_tcp_header
  //numÃ©ro port source
  pdu.hd.source_port=tab_sock[mic_sock]->addr.port;
  //numÃ©ro port destination
  pdu.hd.dest_port=socket_distant.addr.port;
  //numÃ©ro de sÃ©quence 
  pdu.hd.seq_num = num_sequence;
  //numÃ©ro d'ACK
  pdu.hd.ack_num = 42;//// /!\temporaire !!!!
  //trois flag
  pdu.hd.syn = 0;
  pdu.hd.ack = 0;
  pdu.hd.fin = 0;
  //mic_tcp_payload
  //donnÃ©es applicatives
  pdu.payload.data = mesg;
  //taille
  pdu.payload.size = mesg_size ;
  nb_msg_envoye ++;

  while (envoi_ok ==0){
    
    erreur = IP_send(pdu,tab_sock[mic_sock]->addr);
   
    //IP receive pour recevoir un ack
    if( IP_recv(&data_recu,&tab_sock[mic_sock]->addr,1000)!= -1){ 

      ACK_recu = get_header(data_recu.data);
      if(((ACK_recu.ack ==1)&&(ACK_recu.ack_num == (num_sequence+1)%2))|| (envoi_ok != 1)){ //verification du ACK reÃ§u
	envoi_ok =1;
	// mise a jour num sequence
	num_sequence = (num_sequence +1)%2;
      }
    }else {
      nb_msg_perdu ++;
      if((float)nb_msg_perdu/(float)nb_msg_envoye < (float)perte_acceptee/100.0){
	envoi_ok = 1;
	num_sequence = (num_sequence +1)%2;
	printf("---Perte tolÃ©rÃ©e \n");
      }else{
	nb_msg_perdu -- ;
      }
    }

  }
  
  printf("---Nb de messages envoyÃ©s : %d \n",nb_msg_envoye);
  printf("---Nb de messages perdu : %d \n",nb_msg_perdu);
  float prec = ((float)nb_msg_perdu/(float)nb_msg_envoye);
  printf("---Perte actuelle : %f \n",prec);


  if(erreur==-1){
    return -1 ;
  }else {
    return pdu.payload.size;
  } 
}


int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
// Permet Ã  lâapplication rÃ©ceptrice de rÃ©clamer la rÃ©cupÃ©ration dâune donnÃ©e 
// stockÃ©e dans les buffers de rÃ©ception du socket
// Retourne le nombre dâoctets lu ou bien -1 en cas dâerreur
// NB : cette fonction fait appel Ã  la fonction app_buffer_get() 
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  
  mic_tcp_pdu pdu ;
  int nb_octets ;
  pdu.payload.data = mesg ; 
  pdu.payload.size = max_mesg_size ;
  nb_octets = app_buffer_get(pdu.payload) ;
  
  if (nb_octets>0){
    return nb_octets;
  }else{
    return -1 ;
  }
}


int mic_tcp_close (int socket) // ici il faut faire des choses
// Permet de rÃ©clamer la destruction dâun socket. 
// Engendre la fermeture de la connexion suivant le modÃšle de TCP. 
// Retourne 0 si tout se passe bien et -1 en cas d'erreur

{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");    

  mic_tcp_pdu FIN={{tab_sock[socket]->addr.port,socket_distant.addr.port,num_sequence,0,0,0,0},{"",0}};
  mic_tcp_payload data_recu;
  mic_tcp_header reception ;
  int sortie_boucle = 0 ;

  FIN.hd.ack = 0 ;
  FIN.hd.syn = 0 ;
  FIN.hd.fin = 1 ;

  tab_sock[socket]->state = FIN_WAIT ;


  do{ 
    printf("--- Envoi d'un FIN \n");
    IP_send(FIN,tab_sock[0]->addr);
    if(IP_recv(&data_recu,&tab_sock[socket]->addr,1000)==-1){
      reception = get_header(data_recu.data);
      if(reception.ack == 1 && reception.fin == 1){
	sortie_boucle = 1 ;
      }
    }
  }while(!sortie_boucle);//tout pendant que nous n'avons pas reçu de finack 
  printf("--- Reception du FINACK \n");

  sortie_boucle = 0 ;

  FIN.hd.ack = 1 ;
  FIN.hd.syn = 0 ;
  FIN.hd.fin = 0 ;
  do{ 
    printf("--- Envoi d'un ACK \n");
    IP_send(FIN,tab_sock[0]->addr);
    if(IP_recv(&data_recu,&tab_sock[0]->addr,1000)==-1){//on ne recoi plus rien
      sortie_boucle = 1 ;
    }
  }while(!sortie_boucle);//tout pendant quele ack n'est pas recu (on attend le silence)

  tab_sock[socket]->state = CLOSED ;
  printf("Connexion fermée !!!\n");
  

  return -1;
}


void process_received_PDU(mic_tcp_pdu pdu)
// GÃšre le traitement dâun PDU MIC-TCP reÃ§u (mise Ã  jour des numÃ©ros de sÃ©quence
// et d'acquittement, etc.) puis insÃšre les donnÃ©es utiles du PDU dans le buffer 
// de rÃ©ception du socket. Cette fonction utilise la fonction app_buffer_add().   
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  mic_tcp_pdu ack ;
  mic_tcp_payload data_recu ;
  mic_tcp_header reception ; 
  //mic_tcp_header
  //numÃ©ro port source
  ack.hd.source_port=0; //// /!\ c'est bizarre !!!!
  //numÃ©ro port destination
  ack.hd.dest_port=0; //// /!\ c'est bizarre !!!!
  //numÃ©ro d'ACK
  ack.hd.seq_num = 42;//// /!\temporaire !!!!
  //trois flag
  ack.hd.syn = 0;
  ack.hd.ack = 1;
  ack.hd.fin = 0;
  //mic_tcp_payload
  //donnÃ©es applicatives
  ack.payload.data = malloc(0);
  //taille
  ack.payload.size =0 ;

  if(sock.state == ESTABLISHED){

    if(num_sequence == pdu.hd.seq_num){
      app_buffer_set(pdu.payload);
      num_sequence =( num_sequence + 1 ) % 2 ;
      //numero de sequence 
      ack.hd.ack_num = num_sequence;
      IP_send(ack,tab_sock[0]->addr);  //// /!\ c'est bizarre !!!!
    }else if (pdu.hd.fin == 1 && pdu.hd.ack == 0 && pdu.hd.syn == 0 ) {
	printf("--- Reception d'un FIN\n--- Fermeture du socket \n");
	
	int sortie_boucle = 0;
	do{
	  ack.hd.syn = 0 ;
	  ack.hd.ack = 1 ;
	  ack.hd.fin = 1 ;
	  IP_send(ack,tab_sock[0]->addr);
	  printf("--- Envoi d'un FINACK \n");
	  if(IP_recv(&data_recu,&tab_sock[0]->addr,1000)!=-1){
	    reception = get_header(data_recu.data);
	    if(reception.ack == 1 && reception.syn == 0 && reception.fin == 0 ){
	      printf("---Reception d'un ACK \n");
	      sortie_boucle = 1 ;
	    }
	  }

	}while(!sortie_boucle) ; 
	
	sock.state = CLOSED;
	printf("---Connexion fermée !!!!\n");

      }else {
      //numero de sequence 
      ack.hd.ack_num = num_sequence;
      IP_send(ack,tab_sock[0]->addr);  //// /!\ c'est bizarre !!!!
    }

  }else if (sock.state !=CLOSED) {//etablissement de connexion

    if(sock.state ==IDLE){

      if(pdu.hd.syn==1){//on a reÃ§u un SYN

	int perte_distante = pdu.hd.seq_num ;

	if(perte_distante<perte_acceptee){
	  printf("La perte distante est : %d",perte_distante);
	  perte_acceptee = perte_distante;
	}

	printf("---Reception d'un SYN\n");
	sock.state = SYN_RECEIVED; //envoi d'un SYNACK
	ack.hd.syn = 1 ;
	ack.hd.ack = 1;
	ack.hd.seq_num = perte_acceptee ;

	if(IP_send(ack,tab_sock[0]->addr)>=0){
	  printf("---Envoi du SYNACK \n");
	}

      }

    }else{
      if(pdu.hd.ack ==1){//on a recu un ACK
	printf("---Reception d'un ACK\n");
	sock.state = ESTABLISHED ;
	printf("Connexion etablie !! \n");
	printf("La perte acceptée est : %d \n",perte_acceptee);
      }
    }

  }

}
