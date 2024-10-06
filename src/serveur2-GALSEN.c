// server program for udp connection
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>

#define PORT_CONN 7300
#define SIZE 1500
int i ;
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
int fenetre;
int numAck;
int numAckprec;
int trueACK;
int ackToRetransmit;
int ackToRetransmitTO;
int lastToAck;
int lastAcked;
int sent;
int numberSeqs;
int size;
int retransmit;
int sizeOfLastSegment;


float RTT_estimator(struct timeval a,struct timeval b){
	return (a.tv_sec + 0.000001*a.tv_usec)-(b.tv_sec + 0.000001*b.tv_usec);
}
int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}


struct mesParams
	{
	int socket;
	struct sockaddr_in adr;
	FILE* f;
	};

void* thread_send(void * parametres){
	char seq[7];
	int read;
	char bufferS[SIZE];
	char bufferEnd1[sizeOfLastSegment+6];
	fenetre = 100;
	struct mesParams* pr = parametres;
	FILE *fp = pr->f;
	i = 1;
	lastToAck = -1;
    trueACK = 0;
	while(trueACK < numberSeqs){


            
		for (int k = 0; k<25 ; k++){
			pthread_mutex_lock(&mutex2);
			if(trueACK + k +1 < numberSeqs +1 ){
				if (trueACK +k +1 != numberSeqs){
					bzero(bufferS, SIZE);
					sprintf(seq,"%06d",trueACK + k +1);
					strcpy(bufferS,seq); 
					fseek(fp, (trueACK + k)*(SIZE-6) , SEEK_SET );
					retransmit = fread(bufferS+6,1, SIZE-6, fp );
					sendto(pr->socket, bufferS, SIZE , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
		
					//printf("[TRANSMIT]Data segment number:%d\n", trueACK + k + 1);
					//printf("Segments to send: %d\n",numberSeqs);          
					//printf("Data sent: %s\n",bufferS);          
				
					} else if (trueACK +k +1 ==  numberSeqs){
						bzero(bufferEnd1, sizeOfLastSegment +6);
						sprintf(seq,"%06d",trueACK + k +1);
						strcpy(bufferEnd1,seq); 
						fseek(fp, (trueACK + k)*(SIZE-6) , SEEK_SET );
						retransmit = fread(bufferEnd1+6,1, sizeOfLastSegment, fp );
						sendto(pr->socket, bufferEnd1, sizeOfLastSegment + 6 , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
			
					}
					
			}
			pthread_mutex_unlock(&mutex2);
		}

		

	}
	bzero(bufferS, SIZE );
	strcpy(bufferS, "FIN");
	sendto(pr->socket, bufferS, SIZE , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
	printf("[LAST SENDING] Data segment number:%s\n", bufferS);


    

}

void* thread_receive(void * parametres){
	struct mesParams* prr = parametres;
	FILE *fp = prr -> f;
	char seqR[7];
	char bufferR[SIZE];
	char bufferRtr[SIZE];
	char bufferEnd[sizeOfLastSegment + 6];
	numAckprec = 1;
	struct timeval timer;
	struct timeval timer2;
	fd_set readfds, master;
	FD_ZERO(&readfds);
	FD_ZERO(&master);
	FD_SET(prr -> socket, &master);
	FILE *f = prr->f;
	ackToRetransmit = -1;
	ackToRetransmitTO = -1;
	timer.tv_sec = 0;
	timer.tv_usec = 25000;
	while(trueACK < numberSeqs-1){

		
		readfds = master;
		select(prr->socket+1, &readfds, NULL, NULL, &timer);
        if(trueACK < numberSeqs + 5 ){
        if (FD_ISSET(prr -> socket, &readfds)) {
                    recvfrom(prr->socket, bufferR, SIZE,0,(struct sockaddr*)&(prr->adr), NULL);
                  //  printf("[RECEIVING]: %s\n", bufferR);
                    numAck = atoi(&bufferR[3]);
                    bzero(bufferR, SIZE);
                    numAckprec = numAck;
                    if(numAck>trueACK ){
                        trueACK = numAck;
                        pthread_mutex_lock(&mutex1);
                        fenetre = fenetre + 2;
                        pthread_mutex_unlock(&mutex1);
                    } else if (numAck == trueACK && trueACK != ackToRetransmit && trueACK < numberSeqs -1){
                   //     printf("DOUBLE ACK!\n");
                        pthread_mutex_lock(&mutex2);				
                        ackToRetransmit = trueACK;
                        bzero(bufferRtr, SIZE);
                        sprintf(seqR,"%06d",trueACK+1);
                        strcpy(bufferRtr,seqR);
                        fseek(f, (trueACK)*(SIZE-6) , SEEK_SET );
                        retransmit = fread(bufferRtr+6,1, SIZE-6, f );
                        sendto(prr->socket, bufferRtr, SIZE , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
						pthread_mutex_unlock(&mutex2);
                      //  printf("[RETRANSMIT]Data segment number:%d\n", trueACK + 1);	
                        //printf("[RETRANSMIT]Data segment : %s\n", bufferRtr);
						

                    } else if(trueACK == numberSeqs){
							bzero(bufferRtr, SIZE );
							strcpy(bufferRtr, "FIN");
							for(int b=0; b<30; b++){
								sendto(prr->socket, bufferRtr, SIZE , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
							}
							printf("[LAST SENDING R] Data segment number:%s\n", bufferRtr);
							break;	
					}				
					}else if(trueACK < numberSeqs -1){
                      //  printf("TIMEOUT REACHED!\n");
                        ackToRetransmit = trueACK ;
                        ackToRetransmitTO = trueACK;
						pthread_mutex_lock(&mutex2);
						for (int j = 0; j<25 ; j++){
							if(j + trueACK + 1 < numberSeqs ){
								bzero(bufferRtr, SIZE);
								sprintf(seqR,"%06d",trueACK + 1 +j);
								strcpy(bufferRtr,seqR);
								fseek(f, (trueACK + j)*(SIZE-6) , SEEK_SET );
								retransmit = fread(bufferRtr+6,1, SIZE-6, f );
								sendto(prr->socket, bufferRtr, SIZE , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
							//	printf("[RETRANSMIT]Data segment number:%d\n", trueACK + j + 1);
							} else if(j+trueACK+1==numberSeqs){
								bzero(bufferEnd, sizeOfLastSegment +6);
								sprintf(seqR,"%06d",trueACK + 1 +j);
								strcpy(bufferEnd,seqR);
								fseek(f, (trueACK + j)*(SIZE-6) , SEEK_SET );
								retransmit = fread(bufferEnd+6,1, sizeOfLastSegment, f );
								sendto(prr->socket, bufferEnd, sizeOfLastSegment +6 , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));

							}
						}
						pthread_mutex_unlock(&mutex2);


                    }
                   // printf("Dernier paquet recu en contigu: %d\n", trueACK);

                
                
                }
       }
		
		
	
}


void send_file_data(FILE* fp, FILE* fr, int sock, struct sockaddr_in addr)
{
	int n,m;
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	printf("Size of file: %d\n",size);
	numberSeqs =(int) (size/(SIZE - 6)) + 1;
	printf("Segments to send: %d\n",numberSeqs);
	sizeOfLastSegment = size - (numberSeqs -1)*(SIZE - 6);
	printf("Size of last segment: %d\n",sizeOfLastSegment);
	struct timeval start;
	struct timeval stop;
	struct mesParams params;
	struct mesParams rcver;
	float RTT;
	char bufferE[4];

	gettimeofday(&start, NULL);
	// Sending the data
	pthread_t t1;
	pthread_t t2;
	pthread_t t3;
	params.socket = sock;
	params.f = fp;
	//memcpy(params.buffer,bufferS, sizeof(bufferS));
	params.adr = addr;

	rcver.socket = sock;
	rcver.f = fr;
	rcver.adr = addr;

	pthread_create(&t1, NULL, &thread_send, (void *)&params);
	pthread_create(&t2, NULL, &thread_receive, (void *)&rcver);
	pthread_join(t2, NULL);
	pthread_join(t1, NULL);
	if(trueACK == numberSeqs){
		bzero(bufferE, 4 );
		strcpy(bufferE, "FIN");
		for(int a=0; a<30; a++){
			sendto(sock, bufferE, 4 , 0, (struct sockaddr*)&(addr), sizeof(addr));
		}
		
	}

	fclose(fp);
	close(sock);
	exit(0);
}

// Driver code
int main(int argc, char* argv[])
{
	if (argc<=1){
		perror("Veullez preciser le port\n");
		return -1;
    }
	int PORT = atoi(argv[1]);
	char buffer[SIZE];
	char *synack = "SYN-ACK7300";
	int listenfd, len, datafd;
	struct sockaddr_in servaddr, cliaddr, servconn;
	bzero(&servaddr, sizeof(servaddr));

	// Create a UDP Socket
	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);
	servaddr.sin_family = AF_INET;

	// bind server address to socket descriptor
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	//receive the datagram
	len = sizeof(cliaddr);

	int n = recvfrom(listenfd, buffer, sizeof(buffer),
			0, (struct sockaddr*)&cliaddr,&len); //receive message from server
	buffer[n] = '\0';
	printf("Client: %s\n", buffer);

	// send the response
	sendto(listenfd, synack, SIZE, 0,
		(struct sockaddr*)&cliaddr, sizeof(cliaddr));

 	n =  recvfrom(listenfd, buffer, sizeof(buffer),
			0, (struct sockaddr*)&cliaddr,&len);
 	buffer[n] = '\0';
	printf("Client: %s\n", buffer);

 	bzero(&servconn, sizeof(servconn));
	datafd = socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT_CONN);
	servaddr.sin_family = AF_INET;
  	if(bind(datafd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
		printf("\n Error :Bind error \n");
		exit(0);
	}

	printf("Connection established\n");

	n = recvfrom(datafd, buffer, sizeof(buffer),
				0, (struct sockaddr*)&cliaddr,&len); //receive message from server
	buffer[n] = '\0';
	printf("Client: %s\n", buffer);
	char *fileName = strtok(buffer, " ");//On recupere le nom du fichier
	FILE *fp = fopen(fileName, "r");
	FILE *fr = fopen(fileName, "r");
	if (fp == NULL){
		perror("[ERROR] reading the file");
		exit(1);
	}

	printf("[STARTING] UDP File Server started. \n");
	send_file_data(fp, fr, datafd, cliaddr);


	//printf("[SUCCESS] Data transfer complete.\n");
 	printf("[CLOSING] Closing the server.\n");
}