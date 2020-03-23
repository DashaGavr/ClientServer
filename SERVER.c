#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#define MESSAGE_L 1024
#define NAME_L 256
#define COMMAND_L 128
#define MAX_CL 5

int cur = 0;

struct mesbuf {
	char message[MESSAGE_L];
	char name[NAME_L];
	int sock;
}  clmess, servmess, clients[MAX_CL];

int listener, serv_fd;

int extern errno;

int strsearch (char* str1, char* str2) {
	int k = 0, i = 0, j = 0, max = 0, min = 0, flag = 0;

	if (strstr(str1, str2) != NULL) { //подстрока есть
		max = strlen(str1);
		min = strlen(str2);
		
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

int origin (char* str) { //проверяет на оригинальность
	
	int i = 0;

	if (strlen(str) >= NAME_L || strlen(str) == 0) return 1;

	while (i < MAX_CL && strcmp(clients[i].name, str)) i++;
    
    if (i == MAX_CL) return 0;

    else return 1;
}

void StopClients() {
	int i = 0;
	for (i = 0; i < MAX_CL; i++) {
		shutdown(clients[i].sock, 2);
		close(clients[i].sock);
		memset(clients[i].name, '\0', NAME_L);
		memset(clients[i].message, '\0', MESSAGE_L);
	}
}

int SendAll(struct mesbuf* message) { 	
	
	printf("<%s> : %s\n", clmess.name, clmess.message);
	for (int i = 0; i < cur; i++) {
		if (clients[i].sock > 0) {
			if (send(clients[i].sock, message, sizeof(*message), MSG_DONTWAIT) < 0) {
				fprintf(stderr, "Error with sending to %s\n", servmess.name);	// обработка ошибки, как и в раньше = общая вседоступная функция которая решает порблему с ошибкой
				return 1;
			}
		}	
	}
	memset((*message).name, '\0', NAME_L);
	memset((*message).message, '\0', MESSAGE_L);
	return 0;
}

int command(struct mesbuf* message, int n) { 

	int i = 0;


	strcpy((*message).name, "$$$");

	const char* helper = "/help  list of all availabile command\n/quit  leave chat\n/nick <newname>  to change your nickname on newname\n/users list of online users\n";
	
	if (strsearch((*message).message, "/quit") >= 0) { 					//раздели сообщение, отправь прощальное 
		
		char sms[MESSAGE_L];
        strcpy(sms,"User <");
        strcat(sms,clients[n].name);
        
        if (strlen((*message).message) > 6) {
        	strcat(sms ,"> left us with message: ");
        	strcat(sms, (*message).message + 6);
        	strcat(sms, "\n");

        }
        else {
        	strcat(sms,"> left us\n");
        }
        strcpy((*message).message, sms);
        shutdown(clients[n].sock, 2);
        close(clients[n].sock);
        clients[n].sock = 0;
        memset(clients[n].message,'\0', MESSAGE_L);
        memset(clients[n].name,'\0', NAME_L);

        SendAll(message);
    }

	else if (strsearch((*message).message, "/help") == 0) {
		memset((*message).message,'\0', MESSAGE_L);	
		strcpy((*message).message, helper);
		printf("<%s> : /help\n", (*message).name);
		if (send(clients[n].sock, message, sizeof(*message), 0) < 0) {
			fprintf(stderr, "Error with sending to %s\n", servmess.name);
			return 1;
		}
	}

	else if (strsearch((*message).message, "/nick") == 0) {
		i = 7; 
		int k = 0;
		char name[NAME_L];
		
		while ((*message).message[i] != '>' && k != NAME_L) {
			name[k++] = (*message).message[i++];
		}
		name[k] = '\0';

		if (origin(name) || k == NAME_L) { 

				memset((*message).message,'\0', MESSAGE_L);				
				strcpy((*message).message,"WRONG USERNAME");				// клиента выкинуть с прощальным сообщением : у тебя неправильное имя
				send(clients[n].sock, message, sizeof(*message), 0); 		//обработать на стороне клиента

				shutdown(clients[n].sock, 2);
				close(clients[n].sock);
				clients[n].sock = 0;

				memset(clients[n].message,'\0', MESSAGE_L);
				strcpy((*message).message,"User <");
	        	strcat((*message).message,clients[n].name);
	        	strcat((*message).message,"> left us\n");

	        	memset(clients[n].message,'\0', MESSAGE_L);
	        	memset(clients[n].name,'\0', NAME_L);

	        	SendAll(message);

		}
		else {

			strcpy((*message).message, "New nickname for <");
			strcat((*message).message, clients[n].name);
			strcat((*message).message, "> ");
			strcpy(clients[n].name, name);
			strcat((*message).message, "is ");
			strcat((*message).message, clients[n].name);
			
			SendAll(message);
		}
	}

	else if ((strsearch((*message).message, "/users") == 0)) {
		memset((*message).message,'\0', MESSAGE_L);	
		strcpy((*message).message, "Users online: ");
		for (int j = 0; j < MAX_CL; j++) {
			if (clients[j].sock > 0) {
				strcat((*message).message, clients[j].name);
				strcat((*message).message, "\n");
			}
		}
		send(clients[n].sock, message, sizeof(*message), MSG_DONTWAIT);
	}
		
	return 0;
}

void handler() {
	memset(servmess.name, '\0', NAME_L);
	memset(servmess.message, '\0', MESSAGE_L);
	strcpy(servmess.name,"###");
    strcpy(servmess.message, "server is shutting down, thanks to everyone\n");

    SendAll(&servmess);


    for (int i = 0; i < MAX_CL; i++) {
        if (clients[i].sock > 0) {
            shutdown(clients[i].sock, 2);
            close(clients[i].sock);
           	
        }
    }
   
    shutdown(listener, 2);
    close(listener);
    listener=-1;
    exit(0);
};

int main(int argc, char** argv) {

	int port, optval, i = 0, max_d;

	struct sockaddr_in serv_addr;

	fd_set readfd;
	
	signal(SIGINT, handler);
    signal(SIGTSTP, handler);

    for (i = 0; i < MAX_CL; i++) {
    	clients[i].sock = 0;
    	memset(clients[i].name, '\0', NAME_L);
    	memset(clients[i].message, '\0', MESSAGE_L);
    	
    }

    memset(clmess.name, '\0', NAME_L);
	memset(clmess.message, '\0', MESSAGE_L);

	port = atoi(argv[1]);

	fprintf(stderr, "portnumber %d\n", port);

	if ((listener = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("server: socket");
		exit(1);
	}

	printf("Server is ready to work hard\n\n");

	optval = 1;														 //устранение залипшего порта
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listener, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: bind");
		exit(1);
	}

	if (listen(listener, MAX_CL) < 0) {					// сколько?
		perror("listen");
		exit(1);
	}

	max_d = listener;
	int res; 								

	for	(;;) {

		FD_ZERO(&readfd);
		FD_SET(listener, &readfd);
		
		for (i = 0; i < MAX_CL; i++) {
			
			if (clients[i].sock != 0) 
				FD_SET(clients[i].sock, &readfd);
			if (clients[i].sock > max_d) max_d = clients[i].sock;
		}
	
		res = select(max_d + 1, &readfd, NULL, NULL, NULL);
		
		if (res < 0) {  
			if (errno != EINTR) {
				perror("select");
				continue;
			}
			else { 	
				fprintf(stderr, "shutdown DEEEEAAADD\n");		
				StopClients();											// выключаем все клиентские сокеты, закрываем их = функция
				shutdown(listener, 2);
				close(listener);
				listener = -1;
				exit(1);
			}
		} 
		
		if (FD_ISSET(listener, &readfd)) {								// Пришел новый запрос на соединение, прнимаем его

			serv_fd = accept(listener, NULL, NULL);

			memset(servmess.name, '\0', NAME_L);
			memset(servmess.message, '\0', MESSAGE_L);
			strcpy(servmess.name,"$$$");

			if (cur != MAX_CL) {
			
				if (recv(serv_fd, &clmess, sizeof(clmess), 0) < 0) { 	//убрать из namespace & clients = все сдвинуть, изменить счетчик 
					perror("recv");
					fprintf(stderr, "Error with introduction\n");
					shutdown(serv_fd, 2);
					close(serv_fd);
					serv_fd = 0;
				}

				if (origin(clmess.name)) {					// записать его имя... 
					
					strcpy(servmess.message,"WRONG USERNAME");	// клиента выкинуть с прощальным сообщением : у тебя неправильное имя

					send(serv_fd, &servmess, sizeof(servmess), 0);

					memset(servmess.name, '\0', NAME_L);
					memset(servmess.message, '\0', MESSAGE_L);

					shutdown(serv_fd, 2);
					close(serv_fd); 	
					serv_fd = 0;
					continue;
				} 
				int j = 0;
				while(clients[j].sock>0){j++;}
                    clients[j].sock = serv_fd;
                    strcpy(clients[j].name, clmess.name);
				
                strcpy(servmess.message, "New user: ");
                strcat(servmess.message, clients[cur].name);
                strcat(servmess.message, "\n");
                cur++;
            	SendAll(&servmess);

					
			} 
			else {		// достигнут максимум

                strcpy(servmess.message,"maximum is reached");
                send(serv_fd, &servmess, sizeof(servmess), 0);
                printf("%s : %s\n", servmess.name, servmess.message);
                shutdown(serv_fd, 2);
                close(serv_fd);
                serv_fd = 0;
                continue;
            }
		}

		memset(servmess.message, '\0', MESSAGE_L);						// вход в чат закончен; сur показывает сколько клиентов

																		
		for (i = 0; i < MAX_CL; i++) {  								// прочитали -- отправили 

			if (FD_ISSET(clients[i].sock, &readfd)) {					// пришли данные от клиента, читаем их, обрабатываем


				if ((res = recv(clients[i].sock, &clmess, sizeof(clmess), 0)) < 0)	{  // OR CLMESS
					fprintf(stderr, "Error with reading socket! \n");
					shutdown(clients[i].sock, 2);
					close(clients[i].sock);
					clients[i].sock = 0;
					memset(clients[i].name, '\0', NAME_L);
					memset(clients[i].message, '\0', MESSAGE_L);
					continue;
				}

				if (clmess.message[0] == '/') {
					command(&clmess, i);
				}
					
				else if (SendAll(&clmess)) {							// функ, которая рассылает всем сокетам полученное сообщение
					fprintf(stderr, "Error with writing socket! \n");	// отключаем клиента у которого ошибка!?
					StopClients();
					shutdown(listener, 2);
					close(listener);
					listener = -1;
				}
			}															// макрос проверил
		}
	}																	// главный бесконечный цикл - сигнальный обработчик для самоубийства 

	printf("Chat is closed\n");
	StopClients();
	shutdown(listener, 2);
	close(listener);
	listener = -1;
	return 0;
}
