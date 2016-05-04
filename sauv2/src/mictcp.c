#include <mictcp.h>
#include <api/mictcp_core.h>

int setting = 1;
mic_tcp_sock *tab_sock[1];
mic_tcp_sock sock ;
mic_tcp_sock socket_distant ;
unsigned int num_sequence=0;


int mic_tcp_socket(start_mode sm) 
// Permet de créer un socket entre l’application et MIC-TCP// Retourne le descripteur du socket ou bien -1 en cas d'erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  initialize_components(sm); // Appel obligatoire
  sock.fd = 0 ;
  sock.state = ESTABLISHED ; //A changer quand on aura etabli les SYN_SENT etc 
  tab_sock[0]=&sock ;
  set_loss_rate(300);
  return tab_sock[0]->fd ;
}


int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
// Permet d’attribuer une adresse à un socket. Retourne 0 si succès, et -1 en cas d’échec
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  if(socket == (tab_sock[socket]->fd)) {
    tab_sock[socket]->addr = addr;
    return 0 ;
  }else {
    return -1 ;
  }
}


int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
// Met l’application en état d'acceptation d’une requête de connexion entrante
// Retourne 0 si succès, -1 si erreur
{
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
  return 0;
}


int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
// Permet de réclamer l’établissement d’une connexion
// Retourne 0 si la connexion est établie, et -1 en cas d’échec
{   
  printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    
  return 0;
}


int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
// Permet de réclamer l’envoi d’une donnée applicative
// Retourne la taille des données envoyées, et -1 en cas d'erreur
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
    //numéro port source
    pdu.hd.source_port=tab_sock[mic_sock]->addr.port;
    //numéro port destination
    pdu.hd.dest_port=socket_distant.addr.port;
    //numéro de séquence 
    pdu.hd.seq_num = num_sequence;
    //numéro d'ACK
    pdu.hd.ack_num = 42;//// /!\temporaire !!!!
    //trois flag
    pdu.hd.syn = 0;
    pdu.hd.ack = 0;
    pdu.hd.fin = 0;
    //mic_tcp_payload
    //données applicatives
    pdu.payload.data = mesg;
    //taille
    pdu.payload.size = mesg_size ;

  while (envoi_ok ==0){

    erreur = IP_send(pdu,tab_sock[mic_sock]->addr);

    //IP receive pour recevoir un ack
    if( IP_recv(&data_recu,&tab_sock[mic_sock]->addr,1000)!= -1){
      ACK_recu = get_header(data_recu.data);

      if((ACK_recu.ack ==1)&&(ACK_recu.seq_num == (num_sequence+1)%2)){
	envoi_ok =1;
	// mise a jour num sequence
	num_sequence = (num_sequence +1)%2;
      }

    }
  }
  
  if(erreur==-1){
    return -1 ;
  }else {
    return pdu.payload.size;
  } 
}


int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
// Permet à l’application réceptrice de réclamer la récupération d’une donnée 
// stockée dans les buffers de réception du socket
// Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
// NB : cette fonction fait appel à la fonction app_buffer_get() 
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


int mic_tcp_close (int socket)
// Permet de réclamer la destruction d’un socket. 
// Engendre la fermeture de la connexion suivant le modèle de TCP. 
// Retourne 0 si tout se passe bien et -1 en cas d'erreur

{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");    
  return -1;
}


void process_received_PDU(mic_tcp_pdu pdu)
// Gère le traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
// et d'acquittement, etc.) puis insère les données utiles du PDU dans le buffer 
// de réception du socket. Cette fonction utilise la fonction app_buffer_add().   
{
  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
  mic_tcp_pdu ack ;
  //mic_tcp_header
  //numéro port source
  ack.hd.source_port=0; //// /!\ c'est bizarre !!!!
  //numéro port destination
  ack.hd.dest_port=0; //// /!\ c'est bizarre !!!!
  //numéro d'ACK
  ack.hd.ack_num = 42;//// /!\temporaire !!!!
  //trois flag
  ack.hd.syn = 0;
  ack.hd.ack = 1;
  ack.hd.fin = 0;
  //mic_tcp_payload
  //données applicatives
  ack.payload.data = malloc(0);
  //taille
  ack.payload.size =0 ;

  if(num_sequence == pdu.hd.seq_num){
    app_buffer_set(pdu.payload);
    num_sequence =( num_sequence + 1 ) % 2 ;
    //numéro de séquence 
    ack.hd.seq_num = num_sequence;
    IP_send(ack,tab_sock[0]->addr);  //// /!\ c'est bizarre !!!!
  }else {
    //numéro de séquence 
    ack.hd.seq_num = num_sequence;
    IP_send(ack,tab_sock[0]->addr);  //// /!\ c'est bizarre !!!!
  }

}
