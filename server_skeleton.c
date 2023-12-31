#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>
#include <fcntl.h>

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {0}; // list of users


/* Add user to userList */
void user_add(user_info_t *user);
/* Get user name from userList */
char * get_username(int sockfd);
/* Get user sockfd by name */
int get_sockfd(char *name);
/* Send msg to the client*/
void send_msg(char *filename, char* sendmsg);

void send_msg(char *filename, char* sendmsg) {
	FILE* receiver;
	receiver = fopen(filename, "a");
	
	fputs(sendmsg, receiver);
	
	fclose(receiver);
	return ;
}

/* Add user to userList */
void user_add(user_info_t *user){
	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	/***************************/
	/* add the user to the list */
	/**************************/
	int flag = 1;
	
	for (int count = 0; count < MAX_USERS; count++) {
		if (listOfUsers[count] != NULL) {
		if (strcmp(listOfUsers[count]->username, user->username) == 0) 
		{
			listOfUsers[count] = user;	
			return ;
		}
		}
	}
	
	for (int count = 0; count < MAX_USERS; count++) {		
		if (listOfUsers[count] == NULL) {
			listOfUsers[count] = user;	
			users_count++;
			return ;
		}
	}		
	
}

/* Determine whether the user has been registered  */
int isNewUser(char* name) {
	int i;
	int flag = -1;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/
	char temp[24];
	FILE* fp = fopen("user.txt", "r");
	
	while (fgets(temp, 24, fp) != NULL) {
		if ((strcmp(temp, name)) == 0) {
			flag = 1;
			fclose(fp);
			return flag;	
		}
	}
	fclose(fp);
	return flag;
}

/* Get user name from userList */
char * get_username(int ss){
	int i;
	static char uname[MAX];
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	bzero(uname, sizeof(uname));
	
	for (int count = 0; count < MAX_USERS; count++) {
		if (listOfUsers[count] != NULL) { 
			if (listOfUsers[count]->sockfd == ss) {
			strncpy(uname, listOfUsers[count]->username, strlen(listOfUsers[count]->username)-1);
			}
		}
	}

	printf("get user name: %s\n", uname);
	return uname;
}

/* Get user sockfd by name */
int get_sockfd(char *name){
	int i;
	int sock;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	sock = -1;
	for (int count = 0; count < MAX_USERS; count++) {
		if (listOfUsers[count] != NULL) {
			if (strcmp(listOfUsers[count]->username,name) == 0) 	
			{
				sock = listOfUsers[count]->sockfd;
			}
		}
	}


	return sock;
}
// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}



int main(){
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof * pfds * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    	int i, j, u, rv;
	int rc, on = 1;
    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Socket creation failed...\n");
		exit(0);}
	else
		printf("Socket successfully created...\n");
	
	rc = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	
	if (rc < 0) {
		perror("setsockopt() failed");
		close(listener);
		exit(-1);}
		
	rc = ioctl(listener, FIONBIO, (char*)&on);
	
	if (rc < 0) {
		perror("ioctl() failed");
		close(listener);
		exit(-1);}
		
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	
	rc = (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)));
	
	if (rc !=0) {
		printf("Socket bind failed...\n");
		exit(0);}
	else
		printf("Socket successfully bined...\n");

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
	
	
	FILE *fp;
	fp = fopen("user.txt", "a");
	fclose(fp);
	
	fp = fopen("user.txt", "r");
	char file_username[24];
	while(fgets(file_username, 24, fp) != NULL) {
	user_info_t* user_info = malloc(sizeof(user_info_t));

	strcpy(user_info->username, file_username);
	user_info->state = 0;
	user_info_t* user = user_info;																					
	user_add(user);
	}
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener
	
	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);
		
			if (poll_count == -1) {
				perror("poll");
				exit(1);
			}

		// run through the existing connections looking for data to read
		
        	for(i = 0; i < fd_count; i++) {
            	  if (pfds[i].revents & POLLIN) { // we got one!!
                    if (pfds[i].fd == listener) {
                      /**************************/
					  /* we are the listener and we need to handle new connections from clients */
					  /****************************/

						addr_size = sizeof(client_addr);
						newfd = accept(listener, (struct sockaddr*)&client_addr, &addr_size);

						if (newfd == -1) 					
						{perror("accept");}


						// send welcome message
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.\n");
						if (send(newfd, buffer, sizeof(buffer), 0) == -1)
							perror("send");
						else {
						add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
						printf("pollserver: new connection from %s on socket %d\n",inet_ntoa(client_addr.sin_addr),newfd);}
						
                    }  
                    else {
                        // handle data from a client
                                           
						bzero(buffer, sizeof(buffer));
                        if ((nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0)) <= 0) {
                          // got error or connection closed by client
                          if (nbytes == 0) {
                            // connection closed
                            printf("pollserver: socket %d hung up\n", pfds[i].fd);
                          } else {
                            perror("recv");
                          }
						  close(pfds[i].fd); // Bye!
						  del_from_pfds(pfds, i, &fd_count);
                        } else {
                            // we got some data from a client
                  
							if (strncmp(buffer, "REGISTER", 8)==0){
								printf("Got register/login message\n");
								/********************************/
								/* Get the user name and add the user to the userlist*/
								/**********************************/ 								
int a = strlen(buffer);																	
char name[24];
bzero(name, sizeof(name));

int clientfd = pfds[i].fd;
newfd = pfds[i].fd;

strncpy(name, buffer + 8, a - 8);
user_info_t* user_info = malloc(sizeof(user_info_t));
user_info->sockfd = clientfd;

strcpy(user_info->username, name);
user_info->state = 1;
user_info_t* user = user_info;																					
user_add(user);
								if (isNewUser(name) == -1) {
									/********************************/
									/* it is a new user and we need to handle the registration*/
									/**********************************/	
																
FILE *fp;

char mailbox[32];
bzero(mailbox, sizeof(mailbox));

fp = fopen("user.txt", "a");
int num = 0;

while (num < a - 8) {
	fputc(*(name + num), fp);
 	num++;
}

fclose(fp);
									/********************************/
									/* create message box (e.g., a text file) for the new user */
									/**********************************/
									
strncpy(mailbox, name, a-9);
strcat(mailbox, "_mail.txt");

FILE* mail;														
mail = fopen(mailbox, "a");

/*asking for password*/

bzero(buffer, sizeof(buffer));
strcpy(buffer, "Please enter a password.\n");
send(clientfd, buffer, sizeof(buffer), 0);

fclose(mail);

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome ");
									strncat(buffer, name, a - 9);
									strcat(buffer, " to join the chat room!\n");
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/

for (int count = 0; count < MAX_USERS; count++) {
	if (listOfUsers[count] != NULL) {
		if (newfd != listOfUsers[count]->sockfd) {
		if (listOfUsers[count]->state != 0) {
		send(listOfUsers[count]->sockfd, buffer, sizeof(buffer), 0);
		}
		}
	}
}

									/*****************************/
									/* send registration success message to the new user*/
									/*****************************/									strcat(buffer, "A new account has been created.\n");
send(pfds[i].fd, buffer, sizeof(buffer), 0);

								}else {
									/********************************/
									/* it's an existing user and we need to handle the login. Note the state of user,*/
									/**********************************/

char mailname[32];
bzero(mailname, sizeof(mailname));

strncpy(mailname, name, a-9);
strcat(mailname, "_mail.txt");								

bzero(buffer, sizeof(buffer));
strcpy(buffer, "Plese input your password: \n"); 
send(pfds[i].fd, buffer, sizeof(buffer), 0);

/* checking password cortect or not*/ 

bzero(buffer, sizeof(buffer));
strcpy(buffer, "Welcome back! The message box contains:\n"); 
send(pfds[i].fd, buffer, sizeof(buffer), 0);	

/********************************/
									/* send the offline messages to the user and empty the message box*/
									/**********************************/
char temp1[MAX];
FILE* mail;
mail = fopen(mailname, "r");
			
while (fgets(temp1, MAX, mail) != NULL) {
	send(pfds[i].fd, temp1, strlen(temp1), 0);
}

mail = fopen(mailname, "w");
mail = fopen(mailname, "a");
fclose(mail);
									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strncat(buffer, name, strlen(name) - 1);
									strcat(buffer, " is online!\n");
																		/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
for (int count = 0; count < MAX_USERS; count++) {
	if (listOfUsers[count] != NULL) {
		if (pfds[i].fd != listOfUsers[count]->sockfd) {
		if (listOfUsers[count]->state != 0) {
		send(listOfUsers[count]->sockfd, buffer, sizeof(buffer), 0);
		}
		}
	}
}

								}
							}
							else if (strncmp (buffer, "password:",9) ==0){						
char filename [28];
bzero(filename, sizeof(filename));
char temp[MAX];

strncpy(filename, get_username(pfds[i].fd), strlen(get_username(pfds[i].fd)));

strcat(filename, ".txt");

FILE* password;				
password = fopen(filename, "a");
password = fopen(filename, "r");

if (fgets(temp, MAX, password) != NULL) {

if (strcmp(temp, buffer) != 0) {
	bzero(buffer, sizeof(buffer));
	strcpy(buffer, "Wrong password! Please exit and try it again\n"); 
	send(pfds[i].fd, buffer, sizeof(buffer), 0);
	printf("123123\n");
	close(pfds[i].fd);
	del_from_pfds(pfds, i, &fd_count);
}}
else {
password = fopen(filename, "a");
fputs(buffer, password);
}

fclose(password);
							}
							
							else if (strncmp(buffer, "EXIT", 4)==0){
								printf("Got exit message. Removing user from system\n");
								// send leave message to the other members
								
bzero(buffer, sizeof(buffer));
								strcpy(buffer, get_username(pfds[i].fd));
								strcat(buffer, " has left the chatroom\n");
								/*********************************/
								/* Broadcast the leave message to the other users in the group*/
								/**********************************/

for (int count = 0; count < MAX_USERS; count++) {
	if (listOfUsers[count] != NULL) {
		if (pfds[i].fd != listOfUsers[count]->sockfd) {
		if (listOfUsers[count]->state != 0) {
		send(listOfUsers[count]->sockfd, buffer, sizeof(buffer), 0);
			}
		}else {
		listOfUsers[count]->state = 0;
		}
	}
}
								/*********************************/
								/* Change the state of this user to offline*/
								/**********************************/						
								//close the socket and remove the socket from pfds[]
																
close(pfds[i].fd);
del_from_pfds(pfds, i, &fd_count);
							}
							else if (strncmp(buffer, "WHO", 3)==0){
								// concatenate all the user names except the sender into a char array
								printf("Got WHO message from client.\n");
								char ToClient[MAX];
								bzero(ToClient, sizeof(ToClient));
								/***************************************/
								/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
								/* The state of each user (online or offline)should be labelled.*/
								/***************************************/
								
char who_name[24];
for (int count = 0; count < users_count; count++) {
	if (listOfUsers[count] != NULL) {
	if (listOfUsers[count]->sockfd != pfds[i].fd) {
		if(count != 0) {
			if (strlen(ToClient) != 0)
			strcat(ToClient, " ,");	
		}	
		bzero(who_name, sizeof(who_name));
		strncpy(who_name, listOfUsers[count]->username, 
		strlen(listOfUsers[count]->username)-1);
		strcat(ToClient, who_name);
		if (listOfUsers[count]->state == 1) {
			strcat(ToClient, "*");
		}	
	}
	}
}
strcat(ToClient, "\n");
send(pfds[i].fd, ToClient, sizeof(ToClient), 0);
bzero(buffer, sizeof(buffer));
strcpy(buffer, "* means this user online\n");
send(pfds[i].fd, buffer, sizeof(buffer), 0);

							}
							else if (strncmp(buffer, "#", 1)==0){
								// send direct message 
								// get send user name:
								printf("Got direct message.\n");
								// get which client sends the message
								char sendname[MAX];
								// get the destination username
								char destname[MAX];
								// get dest sock
								int destsock;
								// get the message
								char msg[MAX];
								/**************************************/
								/* Get the source name xx, the target username and its sockfd*/
								/*************************************/
int count = 1;
bzero(destname, sizeof(destname));
bzero(sendname, sizeof(sendname));

while (buffer[count] != ':') {
	destname[count-1] = buffer[count];
	count++;
}

strcat(destname, "\n");
strcpy(msg, buffer + count + 1);
strcpy(sendname, get_username(pfds[i].fd));
destsock = get_sockfd(destname);

								if (destsock == -1) {
									/**************************************/
									/* The target user is not found. Send "no such user..." messsge back to the source client*/
									/*************************************/
	bzero(buffer,sizeof(buffer));
	strcpy(buffer, "no such user..\n");
	send(pfds[i].fd, buffer, sizeof(buffer), 0);
								}
								else {
									// The target user exists.
									// concatenate the message in the form "xx to you: msg"
								char sendmsg[MAX];

bzero(sendmsg, sizeof(sendmsg));									

strncpy(sendmsg, sendname, strlen(sendname));
									strcat(sendmsg, " to you: ");
									strcat(sendmsg, msg);

									/**************************************/
									/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
									/* For the offline case, send "...Leaving message successfully" message to the source client*/
									/*************************************/

int receiver_state = 0;

for (int x = 0; x < users_count; x++) {
	if(listOfUsers[x] != NULL) {
		if(listOfUsers[x]->sockfd == destsock) {
			receiver_state = listOfUsers[x]->state;
		}
	}

} 								

if (receiver_state == 0) {

	bzero(buffer, sizeof(buffer));
	char receiver_file_name[MAX];
	bzero(receiver_file_name, sizeof(receiver_file_name));
	strncpy(receiver_file_name, destname, strlen(destname) - 1);
	strcat(receiver_file_name, "_mail.txt");

	send_msg(receiver_file_name, sendmsg);
		
	strcpy(buffer, "...Leaving message successfully\n");
	send(pfds[i].fd, buffer, sizeof(buffer), 0);
}								
else {
	send(destsock, sendmsg, sizeof(sendmsg), 0);
}								
								}

							}
							else{
								printf("Got broadcast message from user\n");
								/*********************************************/
								/* Broadcast the message to all users except the one who sent the message*/
								/*********************************************/
								
for (int count = 0; count < MAX_USERS; count++) {
	if (listOfUsers[count] != NULL) {
		if (pfds[i].fd != listOfUsers[count]->sockfd) {
		if (listOfUsers[count]->state != 0) {
		send(listOfUsers[count]->sockfd, buffer, sizeof(buffer), 0);
		}
		}
	}
}								
							}   

                        }
                    } // end handle data from client
                  } // end got new incoming connection
                } // end looping through file descriptors
         	}// end for(;;) 
		

	return 0;
}
