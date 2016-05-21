//Pour le moment l'Ã©tablissement de connexion fonctionne bien
//La fiabilitÃ© partielle semble fonctionner, il faut la tester avec la video
//Cependant un bug persiste, pour le moment nous sommes obligÃ©s de 
//crÃ©er les pertes aprÃ¨s la connexion sinon ce la fonctionne pas

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
// Permet de crÃÂ©er un socket entre lÃ¢ÂÂapplication et MIC-TCP// Retourne le descripteur du socket ou bien -1 en cas d'erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  initialize_components(sm); // Appel obligatoire
  sock.fd = 0 ;
  sock.state = IDLE ;
  tab_sock[0]=&sock ;
  return tab_sock[0]->fd ;
}


int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
// Permet dÃ¢ÂÂattribuer une adresse ÃÂ  un socket. Retourne 0 si succÃÅ¡s, et -1 en cas dÃ¢ÂÂÃÂ©chec
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
// Met lÃ¢ÂÂapplication en ÃÂ©tat d'acceptation dÃ¢ÂÂune requÃÂªte de connexion entrante
// Retourne 0 si succÃÅ¡s, -1 si erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

  while (tab_sock[socket]->state!=ESTABLISHED){
    sleep(1); // listen
  }

  return 0;
}




int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) //ici il faut faire des choses
// Permet de rÃÂ©clamer lÃ¢ÂÂÃÂ©tablissement dÃ¢ÂÂune connexion
// Retourne 0 si la connexion est ÃÂ©tablie, et -1 en cas dÃ¢ÂÂÃÂ©chec
{ 
  set_loss_rate(800);
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");    // Seul la source peut demander la connexion    

  mic_tcp_pdu SYN={{tab_sock[socket]->addr.port,socket_distant.addr.port,num_sequence,0,0,0,0},{"",0}};
  mic_tcp_payload data_recu;
  data_recu.data = malloc(15);
  data_recu.size = 15 ;
  mic_tcp_header reception ;
  int sortie_boucle = 0 ;

  SYN.hd.ack = 0 ;
  SYN.hd.syn = 1 ;
  SYN.hd.fin = 0 ;
  SYN.hd.seq_num = perte_acceptee ;
  tab_sock[socket]->state = SYN_SENT ;


  do{ 
    printf("--- Envoi d'un SYN \n");
    IP_send(SYN,tab_sock[0]->addr);
    if(IP_recv(&data_recu,&tab_sock[socket]->addr,1000)!=-1){
      reception = get_header(data_recu.data);
      if(reception.ack == 1 && reception.syn == 1){
	sortie_boucle = 1 ;
      }
    }
  }while(!sortie_boucle);//tout pendant que nous n'avons pas recu de finack 
  printf("--- Reception du SYNACK \n");

  sortie_boucle = 0 ;

  SYN.hd.ack = 1 ;
  SYN.hd.syn = 0 ;
  SYN.hd.fin = 0 ;
  perte_acceptee = SYN.hd.seq_num;
  do{ 
    printf("--- Envoi d'un ACK \n");
    IP_send(SYN,tab_sock[0]->addr);
    if(IP_recv(&data_recu,&tab_sock[0]->addr,10000)==-1){//on ne recoit plus rien
      sortie_boucle = 1 ;
    }
  }while(!sortie_boucle);//tout pendant quele ack n'est pas recu (on attend le silence)

  tab_sock[socket]->state =ESTABLISHED ;
  printf("Connexion �tablie !!!\n");
  printf("Perte acceptee : %d \n",perte_acceptee);
  
  free(data_recu.data);
  return 0;

}


int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
// Permet de rÃÂ©clamer lÃ¢ÂÂenvoi dÃ¢ÂÂune donnÃÂ©e applicative
// Retourne la taille des donnÃÂ©es envoyÃÂ©es, et -1 en cas d'erreur
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
  //numÃÂ©ro port source
  pdu.hd.source_port=tab_sock[mic_sock]->addr.port;
  //numÃÂ©ro port destination
  pdu.hd.dest_port=socket_distant.addr.port;
  //numÃÂ©ro de sÃÂ©quence 
  pdu.hd.seq_num = num_sequence;
  //numÃÂ©ro d'ACK
  pdu.hd.ack_num = 42;//// /!\temporaire !!!!
  //trois flag
  pdu.hd.syn = 0;
  pdu.hd.ack = 0;
  pdu.hd.fin = 0;
  //mic_tcp_payload
  //donnÃÂ©es applicatives
  pdu.payload.data = mesg;
  //taille
  pdu.payload.size = mesg_size ;
  nb_msg_envoye ++;

  while (envoi_ok ==0){
    
    erreur = IP_send(pdu,tab_sock[mic_sock]->addr);
   
    //IP receive pour recevoir un ack
    if( IP_recv(&data_recu,&tab_sock[mic_sock]->addr,1000)!= -1){ 

      ACK_recu = get_header(data_recu.data);
      if(((ACK_recu.ack ==1)&&(ACK_recu.ack_num == (num_sequence+1)%2))|| (envoi_ok != 1)){ //verification du ACK reÃÂ§u
	envoi_ok =1;
	// mise a jour num sequence
	num_sequence = (num_sequence +1)%2;
      }
    }else {
      nb_msg_perdu ++;
      if((float)nb_msg_perdu/(float)nb_msg_envoye < (float)perte_acceptee/100.0){
	envoi_ok = 1;
	num_sequence = (num_sequence +1)%2;
	printf("---Perte tolÃÂ©rÃÂ©e \n");
      }else{
	nb_msg_perdu -- ;
      }
    }

  }
  
  printf("---Nb de messages envoyÃÂ©s : %d \n",nb_msg_envoye);
  printf("---Nb de messages perdu : %d \n",nb_msg_perdu);
  float prec = ((float)nb_msg_perdu/(float)nb_msg_envoye);
  printf("---Perte actuelle : %f \n",prec);

  free(data_recu.data);
  if(erreur==-1){
    return -1 ;
  }else {
    return pdu.payload.size;
  } 
}


int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
// Permet ÃÂ  lÃ¢ÂÂapplication rÃÂ©ceptrice de rÃÂ©clamer la rÃÂ©cupÃÂ©ration dÃ¢ÂÂune donnÃÂ©e 
// stockÃÂ©e dans les buffers de rÃÂ©ception du socket
// Retourne le nombre dÃ¢ÂÂoctets lu ou bien -1 en cas dÃ¢ÂÂerreur
// NB : cette fonction fait appel ÃÂ  la fonction app_buffer_get() 
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  mic_tcp_pdu pdu ;
  int nb_octets ;
  pdu.payload.data = mesg ; 
  pdu.payload.size = max_mesg_size ;
  nb_octets = app_buffer_get(pdu.payload) ;
  
  if(sock.state == CLOSED){
    return 666 ;
  }

  if (nb_octets>0){
    return nb_octets;
  }else{
    return -1 ;
  }
}


int mic_tcp_close (int socket) // ici il faut faire des choses
// Permet de rÃÂ©clamer la destruction dÃ¢ÂÂun socket. 
// Engendre la fermeture de la connexion suivant le modÃÅ¡le de TCP. 
// Retourne 0 si tout se passe bien et -1 en cas d'erreur

{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");    

  mic_tcp_pdu FIN={{tab_sock[socket]->addr.port,socket_distant.addr.port,num_sequence,0,0,0,0},{"",0}};
  mic_tcp_payload data_recu;
  data_recu.data = malloc(15);
  data_recu.size = 15 ;
  mic_tcp_header reception ;
  int sortie_boucle = 0 ;

  FIN.hd.ack = 0 ;
  FIN.hd.syn = 0 ;
  FIN.hd.fin = 1 ;

  tab_sock[socket]->state = FIN_WAIT ;


  do{ 
    printf("--- Envoi d'un FIN \n");
    IP_send(FIN,tab_sock[0]->addr);
    if(IP_recv(&data_recu,&tab_sock[socket]->addr,1000)!=-1){
      reception = get_header(data_recu.data);
      if(reception.ack == 1 && reception.fin == 1){
	sortie_boucle = 1 ;
      }
    }
  }while(!sortie_boucle);//tout pendant que nous n'avons pas reÃ§u de finack 
  printf("--- Reception du FINACK \n");

  sortie_boucle = 0 ;

  FIN.hd.ack = 1 ;
  FIN.hd.syn = 0 ;
  FIN.hd.fin = 0 ;
  do{ 
    printf("--- Envoi d'un ACK \n");
    IP_send(FIN,tab_sock[0]->addr);
    if(IP_recv(&data_recu,&tab_sock[0]->addr,10000)==-1){//on ne recoi plus rien
      sortie_boucle = 1 ;
    }
  }while(!sortie_boucle);//tout pendant quele ack n'est pas recu (on attend le silence)

  tab_sock[socket]->state = CLOSED ;
  printf("Connexion fermÃ©e !!!\n");
  
  free(data_recu.data);
  return 0;
}


void process_received_PDU(mic_tcp_pdu pdu)
// GÃÅ¡re le traitement dÃ¢ÂÂun PDU MIC-TCP reÃÂ§u (mise ÃÂ  jour des numÃÂ©ros de sÃÂ©quence
// et d'acquittement, etc.) puis insÃÅ¡re les donnÃÂ©es utiles du PDU dans le buffer 
// de rÃÂ©ception du socket. Cette fonction utilise la fonction app_buffer_add().   
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  mic_tcp_pdu ack ;
  mic_tcp_payload data_recu ;
  data_recu.data = malloc(15);
  data_recu.size = 15 ;
  mic_tcp_header reception ; 
  //mic_tcp_header
  //numÃÂ©ro port source
  ack.hd.source_port=0; //// /!\ c'est bizarre !!!!
  //numÃÂ©ro port destination
  ack.hd.dest_port=0; //// /!\ c'est bizarre !!!!
  //numÃÂ©ro d'ACK
  ack.hd.seq_num = 42;//// /!\temporaire !!!!
  //trois flag
  ack.hd.syn = 0;
  ack.hd.ack = 1;
  ack.hd.fin = 0;
  //mic_tcp_payload
  //donnÃÂ©es applicatives
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
      printf("---Connexion fermÃ©e !!!!\n");
      app_buffer_set(data_recu);

    }else {
      //numero de sequence 
      ack.hd.ack_num = num_sequence;
      IP_send(ack,tab_sock[0]->addr);  //// /!\ c'est bizarre !!!!
    }
  }else if (sock.state !=CLOSED) {//etablissement de connexion

    if(pdu.hd.syn == 1 ){
      int perte_distante = pdu.hd.seq_num ;

      if(perte_distante<perte_acceptee){
	printf("La perte distante est : %d",perte_distante);
	perte_acceptee = perte_distante;
      }

      printf("--- Reception d'un SYN\n");
	
      int sortie_boucle = 0;
      do{
	ack.hd.syn = 1 ;
	ack.hd.ack = 1 ;
	ack.hd.fin = 0 ;
	IP_send(ack,tab_sock[0]->addr);
	printf("--- Envoi d'un SYNACK \n");
	if(IP_recv(&data_recu,&tab_sock[0]->addr,1000)!=-1){
	  reception = get_header(data_recu.data);
	  if(reception.ack == 1 && reception.syn == 0 && reception.fin == 0 ){
	    printf("---Reception d'un ACK \n");
	    sortie_boucle = 1 ;
	  }
	}
	
      }while(!sortie_boucle) ; 
		
      sock.state = ESTABLISHED;
      printf("---Connexion etbalie !!!!\n");
      printf("Perte acceptee : %d \n",perte_acceptee);
      app_buffer_set(data_recu);
    }
	
	
  }

  free(data_recu.data);
}
