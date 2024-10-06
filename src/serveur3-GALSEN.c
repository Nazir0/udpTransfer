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
#include <signal.h>



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
int sizeOfLastSegment;
int size;
int retransmit;

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

    
    
    for (int k = 0; k<numberSeqs-1; k++){
        
            bzero(bufferS, SIZE);
            pthread_mutex_lock(&mutex2);
            sprintf(seq,"%06d", k +1);
            strcpy(bufferS,seq); 
            fseek(fp, (k)*(SIZE-6) , SEEK_SET );
            pthread_mutex_unlock(&mutex2);
            retransmit = fread(bufferS+6,1, SIZE-6, fp );
            sendto(pr->socket, bufferS, SIZE , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
          //  printf("[TRANSMIT]Data segment number:%d\n",  1 + k);

        } 
		bzero(bufferEnd1, sizeOfLastSegment + 6);
		pthread_mutex_lock(&mutex2);
		sprintf(seq,"%06d", numberSeqs);
		strcpy(bufferEnd1,seq); 
		fseek(fp, (numberSeqs-1)*(SIZE-6) , SEEK_SET );
		pthread_mutex_unlock(&mutex2);
		retransmit = fread(bufferEnd1+6,1, sizeOfLastSegment, fp );
		sendto(pr->socket, bufferEnd1, sizeOfLastSegment + 6 , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
		//printf("Sending last data segment:%s\n", bufferEnd1);

	
    

	if(numAck == numberSeqs){
		bzero(bufferS, SIZE );
		strcpy(bufferS, "FIN");
		sendto(pr->socket, bufferS, SIZE , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
		sendto(pr->socket, bufferS, SIZE , 0, (struct sockaddr*)&(pr->adr), sizeof(pr->adr));
	//	printf("[LAST SENDING] Data segment number:%s\n", bufferS);


	}


    

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
	fd_set readfds, master, master2;
	FD_ZERO(&readfds);
	FD_ZERO(&master);
	FD_ZERO(&master2);
	FD_SET(prr -> socket, &master);
	FILE *f = prr->f;
	ackToRetransmit = -1;
	ackToRetransmitTO = -1;
	timer.tv_sec = 0;
	timer.tv_usec = 100000;
	
	while(numAck < numberSeqs){


		readfds = master;
		master2 = master;
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

                    } else if (numAck == trueACK && trueACK != ackToRetransmit && trueACK < numberSeqs -4){
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
                     //   printf("[RETRANSMIT]Data segment number:%d\n", trueACK + 1);	
                        //printf("[RETRANSMIT]Data segment : %s\n", bufferRtr);
 

                    } else if(numAck == numberSeqs){
							bzero(bufferRtr, SIZE );
							strcpy(bufferRtr, "FIN");
							for(int b=0; b<50; b++){
								sendto(prr->socket, bufferRtr, SIZE , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
							}
						//	printf("[LAST SENDING R] Data segment number:%s\n", bufferRtr);
							break;
					} else if (numAck == numberSeqs - 1){
							bzero(bufferEnd, sizeOfLastSegment + 6);
							sprintf(seqR,"%06d",numAck + 1);
							strcpy(bufferEnd,seqR);
							fseek(f, (numAck)*(SIZE-6) , SEEK_SET );
							retransmit = fread(bufferEnd+6,1, sizeOfLastSegment, f );
							sendto(prr->socket, bufferEnd, sizeOfLastSegment + 6  , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
							// printf("Sending last data segment in Receive1\n");
							// printf("Sending last data segment:%s\n", bufferEnd);

					}
					}else if(trueACK < numberSeqs -1 ){

							if(trueACK < numberSeqs -100){
								//	printf("TIMEOUT REACHED!\n");
									ackToRetransmit = trueACK ;
									ackToRetransmitTO = trueACK;
									pthread_mutex_lock(&mutex2);
									for (int j = 0; j<150 ; j++){
										if(j + trueACK + 1 < numberSeqs-3){
											bzero(bufferRtr, SIZE);
											sprintf(seqR,"%06d",trueACK + 1 +j);
											strcpy(bufferRtr,seqR);
											fseek(f, (trueACK + j)*(SIZE-6) , SEEK_SET );
											retransmit = fread(bufferRtr+6,1, SIZE-6, f );
											sendto(prr->socket, bufferRtr, SIZE , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
									//		printf("[RETRANSMIT]Data segment number:%d\n", trueACK + j + 1);
																		

							
								}
							}
							pthread_mutex_unlock(&mutex2);

						} else {
							//printf("TIMEOUT REACHED!\n");
							recvfrom(prr->socket, bufferR, SIZE,0,(struct sockaddr*)&(prr->adr), NULL);
							numAck = atoi(&bufferR[3]);
                   		    bzero(bufferR, SIZE);
							 if(numAck>trueACK ){
                		        trueACK = numAck;
                 			   }
							pthread_mutex_lock(&mutex2);
							for (int j = 0; j<10 ; j++){
								if(j + trueACK + 1 < numberSeqs ){
									bzero(bufferRtr, SIZE);
									sprintf(seqR,"%06d",trueACK + 1 +j);
									strcpy(bufferRtr,seqR);
									fseek(f, (trueACK + j)*(SIZE-6) , SEEK_SET );
									retransmit = fread(bufferRtr+6,1, SIZE-6, f );
									sendto(prr->socket, bufferRtr, SIZE , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
								//	printf("[RETRANSMIT]Data segment number:%d\n", trueACK + j + 1);
							
								} else if(j + trueACK + 1 == numberSeqs){
									bzero(bufferEnd, sizeOfLastSegment + 6);
									sprintf(seqR,"%06d",trueACK + 1 +j);
									strcpy(bufferEnd,seqR);
									fseek(f, (trueACK + j)*(SIZE-6) , SEEK_SET );
									retransmit = fread(bufferEnd+6,1, sizeOfLastSegment, f );
									sendto(prr->socket, bufferEnd, sizeOfLastSegment + 6  , 0, (struct sockaddr*)&(prr->adr), sizeof(prr->adr));
									// printf("Sending last data segment in Receive\n");
									// printf("Sending last data segment:%s\n", bufferEnd);
								}
							}
							pthread_mutex_unlock(&mutex2);

						}
                        

                    }
 
                
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
	pthread_create(&t3, NULL, &thread_receive, (void *)&rcver);
	pthread_create(&t2, NULL, &thread_receive, (void *)&rcver);
	pthread_join(t2, NULL);
    pthread_join(t3, NULL);
	pthread_join(t1, NULL);
	if(trueACK == numberSeqs){
		bzero(bufferE, 4 );
		strcpy(bufferE, "FIN");
		for(int a=0; a<50; a++){
			sendto(sock, bufferE, 4 , 0, (struct sockaddr*)&(addr), sizeof(addr));
		}		
		printf("[LAST SENDING T] Data segment number:%s\n", bufferE);
	}

	fclose(fp);
	close(sock);
	exit(0);
}

void initConn( struct sockaddr_in cliaddr, struct sockaddr_in servconn, int PORT, int PORT_CONN,char *synack){
    	printf("Listening to new clients port: %d!\n", PORT );
        int  len = sizeof(cliaddr);

        char bufferI[SIZE];
		bzero(&servconn, sizeof(servconn));
		int datafd = socket(AF_INET, SOCK_DGRAM, 0);
		servconn.sin_addr.s_addr = htonl(INADDR_ANY);
		servconn.sin_port = htons(PORT_CONN);
		servconn.sin_family = AF_INET;
		if(bind(datafd, (struct sockaddr*)&servconn, sizeof(servconn))<0){
			printf("\n Error :Bind error \n");
			exit(0);
		}

		printf("Connection established\n");

		int n = recvfrom(datafd, bufferI, sizeof(bufferI),
					0, (struct sockaddr*)&cliaddr,&len); //receive message from server
		bufferI[n] = '\0';
		printf("Client: %s\n", bufferI);
		char *fileName = strtok(bufferI, " ");//On recupere le nom du fichier
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
// Driver code
int giveMeOne(int tab[4]){
    for(int i=0; i<4; i++){
        if(tab[i]==1){
            return i;
        }
    }
}
int main(int argc, char* argv[])
{	
	// int id = fork();
	if (argc<=1){
		perror("Veullez preciser le port\n");
		return -1;
    }

    int PORT = atoi(argv[1]);
    char buffer[SIZE];
    int n = 0;
    int m;
    int listenfd, len, datafd;
    struct sockaddr_in servaddr, cliaddr, servconn,servconn1, servconn2, servconn3, servconn4;
    struct sockaddr_in servconns[] = {servconn1, servconn2, servconn3, servconn4};
    char *synack []= {"SYN-ACK8000","SYN-ACK8001","SYN-ACK8002","SYN-ACK8003"};
    int PORTS[]={8000,8001,8002,8003};
    int status[] = {1,1,1,1};
    bzero(&servaddr, sizeof(servaddr));
    printf("Une place libre:%d\n",giveMeOne(status));
    while(1){
        // Create a UDP Socket
            listenfd = socket(AF_INET, SOCK_DGRAM, 0);
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            servaddr.sin_port = htons(PORT);
            servaddr.sin_family = AF_INET;

            // bind server address to socket descriptor
            bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
            len = sizeof(cliaddr);

            //receive the datagram
            
            int n = recvfrom(listenfd, buffer, sizeof(buffer),
                    0, (struct sockaddr*)&cliaddr,&len); //receive message from server
            buffer[n] = '\0';
            printf("Client: %s\n", buffer);

            // send the response
            int c = giveMeOne(status);
            status[c] = 0;
            sendto(listenfd, synack[c], SIZE, 0,
                (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            printf("Sending: %s\n", synack[c]);
            n =  recvfrom(listenfd, buffer, sizeof(buffer),
                    0, (struct sockaddr*)&cliaddr,&len);
            buffer[n] = '\0';
            printf("Client: %s\n", buffer);

            close(listenfd);
            int id = fork();

            if(id == 0 && (c == 0 || c==1)){
                int id2 = fork();
                if(id2 == 0 && c==0){
                    initConn( cliaddr,servconn, PORT,PORTS[0],synack[0]);
                    status[c] = 1;
                    
                } else if (id2 > 0 && c==1){
                    initConn(cliaddr,servconn1, PORT,PORTS[1],synack[1]); 
                    status[c] = 1; 
                      
                } else {
                    printf("Fork failed1\n");
                }
                
            } else if(id >0 && (c == 2 || c==3)){
                int id3 = fork();
                if(id3==0 && c==2){
                    initConn( cliaddr,servconn2, PORT,PORTS[2],synack[2]);
                    status[c] = 1;
                } else if(id3 >0 && c==3){
                    initConn( cliaddr,servconn3, PORT,PORTS[3],synack[3]);
                    status[c] = 1;
                } else {
                    printf("Fork Failed2\n");
                }
            } else {
                printf("Fork failed3\n");
            }

            }
    
}
