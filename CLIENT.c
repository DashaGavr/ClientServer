#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>

#define MESSAGE_L 1024
#define MESSAGE_P 512
#define NAME_L 256

extern int errno;

struct mesbuf {
	char message[MESSAGE_L];
	//char private[MESSAGE_P];
	char name[NAME_L];
	int sock_ad;
}  cmessage, servmess;

int sockfd;

enum curstate {
	start, 
	chat
};

int strsearch (char* str1, char* str2) {   //ЗАЧЕМ?
	int k = 0, i = 0, j = 0, max = 0, min = 0, flag = 0;

	if (strstr(str1, str2) != NULL) { 	//подстрока есть
		max = strlen(str1);
		min = strlen(str2);
		//printf("strlen1 = %d, strlen2 = %d\n", max, min);
		while (k < max) {
			if (str1[k] == str2[0] && !flag) {
				i = k;
				flag = 1;
			}
			while (str1[k] == str2[j] && j < min && k < max) {
				j++;
				k++;
			}
			if (j == min) return i;
			else {
				k++;
				flag = 0;
				j = 0;
			}
		}
	}
	else return -1;
}

void cleanmess	(struct mesbuf* clmess) { 
	int i = 0;
	while(i != MESSAGE_L) {
		((*clmess).message)[i++] = '\0';
	}
}

void handler () {
	
    strcpy(cmessage.message,"User ");
    strcat(cmessage.message,cmessage.name);
    strcat(cmessage.message," left us\n");

   	strcpy(cmessage.name,"$$$");

    send(sockfd, &cmessage, sizeof(cmessage), MSG_DONTWAIT);

    shutdown(sockfd,2);
    close(sockfd);
    exit(0);
}

int main (int argc, char** argv) {

	char c;
	int port;
	struct sockaddr_in 		serv_addr;
	enum curstate state = start;

	const char* begin = "Start chat. Remember: your message should not exceed 1024 symbols\0";

	fd_set readfd;

	signal(SIGINT, handler);
	signal(SIGTSTP, handler);

	port = atoi(argv[1]);

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
			perror("client: socket");
			exit(1);
	}

	int opt = 1;

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = PF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	fprintf(stderr, "portnumber %d\n", port);

	printf("Trying to connect...\n");
	if (connect (sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0 ) {
		perror ("client: connect");
		exit(1);
	}
	else printf("Connected\n");

	int try = 0;
	
	while (state == start && try < 5) { 				// предварительный сенд с именем, вход в чат;

		int k = 0;

		printf("Enter the username:  ");

		while ((c = getchar()) != '\n') {
			cmessage.name[k++] = c;
		}

		send(sockfd, &cmessage, sizeof(cmessage), MSG_DONTWAIT);			//отправила имя
		recv(sockfd, &servmess, sizeof(servmess), MSG_DONTWAIT);			//прошло ли проверку
		
		if (!strcmp(servmess.message, "WRONG USERNAME")) {
			fprintf(stderr, "Wrong username. Try again!\n"); 	
			try++;
		}
		else {
			state = chat; 													//не печатается
			
		}
	}

	printf("%s\n\n", begin); 				// выводить правила чата

	int cur = 0;							//длина текущего сообщения

	printf("Start chat: \n");

	FD_ZERO(&readfd);

	int max_d = sockfd;
	int flag_nick = 0;
	char nick[NAME_L];

	while(1) {

		FD_SET(0, &readfd);
	    FD_SET(sockfd, &readfd);

        select(max_d + 1, &readfd, NULL, NULL, NULL);

        if (FD_ISSET(0, &readfd)) {

			cleanmess(&servmess);
			
			int ENTER_END = 0;

			while (cur < MESSAGE_L && !ENTER_END) { 
				c = getchar();
				cmessage.message[cur] = c;
				cur++;
				
				if ((c == EOF) || (c == '\n')) {
					cmessage.message[cur] = '\0';
					ENTER_END = 1; 
					fflush(stdin);
				}
			}

			if (cur > MESSAGE_L) {						//отловить ситауацию переполнения сообщения
				cleanmess(&cmessage);								
				fprintf(stderr, "Your message too long! Try again\n");
				while ((c = getchar()) != '\n');		//подвисание = ждем нажатия enter
			}			
			else {			
					if (strsearch(cmessage.message, "/nick") == 0) {
						flag_nick = 1;
						int r = 0, m = 7;
						while (cmessage.message[m] != '>' && m != NAME_L) {
							nick[r++] = cmessage.message[m++];
						}
						nick[r] = '\0';
						
					}
						
					if (send(sockfd, &cmessage, sizeof(cmessage), MSG_DONTWAIT) < 0) {
						fprintf(stderr, "Can't writing socket\n"); 	 	//если не можем отправить -> отключаемся
						break;
					}

					if (strsearch(cmessage.message, "/quit") == 0) {		// если выход 
						printf("You leave the chat\n");
						shutdown(sockfd, 2);
						close(sockfd);
						exit(0);
					}
					
				//}
			}
			cleanmess(&cmessage);
			cur = 0;
		}

        if (FD_ISSET(sockfd, &readfd)) {

			if (recv(sockfd, &servmess, sizeof(servmess), MSG_DONTWAIT) > 0) { 

				if (flag_nick) {
					if (strcmp(servmess.message,"WRONG USERNAME") != 0) 
						strcpy(cmessage.name, nick);
					flag_nick = 0;
				}

				if (strsearch(servmess.message, "/quit") == 0) {
					printf("%s\n", servmess.message); 			//получить прощальное сообщение
					return 0;									
				}

				if (!strcmp(servmess.name,"$$$")) {
					printf("$$$ : %s\n", servmess.message);
					continue; 									
				}

				printf("<%s> : " , servmess.name);
				printf("%s\n" , servmess.message);
				fflush(stdout);
			}

			else { 
				printf("Chat is over\n");
				shutdown(sockfd, 2);
				close(sockfd);
				break;
			}
		}
		
	}

	shutdown(sockfd, 2);
	close(sockfd); 	
	return 0;
}

