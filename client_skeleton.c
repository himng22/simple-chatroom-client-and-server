#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number

int line = 0;
static int sockfd;
struct argstruct {
	WINDOW* win;
	WINDOW* chatwin;
	pthread_t* recv_server_msg_thread;	
}args;

void generate_menu(WINDOW* chat){
	wprintw(chat, "Hello dear user pls select one of the following options:\n");
	wprintw(chat, "EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    wprintw(chat, "WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    wprintw(chat, "#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    wprintw(chat, "Or input messages sending to everyone in the chatroom.\n");
    wrefresh(chat);
}

void recv_server_msg_handler(struct argstruct* args) {
    /********************************/
	/* receive message from the server and desplay on the screen*/
	/**********************************/
	line++;
	if(line > 22) {
		wclear(args->win);
		wrefresh(args->win);
		line = 0;
	}
	
	char buffer[1024];
    	if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        	perror("recv");
    	}
    	wprintw(args->win, "%s", buffer);
    	wrefresh(args->win);
    	wmove(args->chatwin, 1, 1);
    	wrefresh(args->chatwin);
    	
    	
    	pthread_cancel(*args->recv_server_msg_thread);
    	pthread_t recv_server_msg_thread1;
    	args->recv_server_msg_thread = &recv_server_msg_thread1;  	
	
	if (pthread_create(&recv_server_msg_thread1, NULL, (void*) recv_server_msg_handler, args)) {
		perror("Could not create thread");
	}
}

int main(){
    	int n;
    	int ch;
    	int i = 0;
	int nbytes;
	char input[1024];
	
	struct sockaddr_in server_addr, client_addr;
	char buffer[MAX];
	
	//clear the window and create the window
	initscr();
	noecho();
	cbreak();
	
	WINDOW* chat = newwin(22.5, 78, 0 , 0);
	WINDOW* chatbox = newwin(3, 78, 23, 0);
	
	wrefresh(chat);
	wrefresh(chatbox);
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		wprintw(chat, "Socket creation failed...\n");
		endwin();
		exit(0);
	}
	else
		wprintw(chat, "Socket successfully created...\n");
	
	wrefresh(chat);	
	bzero(&server_addr, sizeof(server_addr));
	
	// assign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(PORT);
	
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
		wprintw(chat, "Connection with the server failed...\n");
		endwin();
		exit(0);
	}
	else
		wprintw(chat, "Connected to the server...\n");

	wrefresh(chat);
	generate_menu(chat);
	
	// recieve welcome message to enter the nickname
    	bzero(buffer, sizeof(buffer));
    	if (nbytes = recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        	perror("recv");
   	 }
    	wprintw(chat, "%s", buffer);
    	wrefresh(chat);
	
	/*************************************/
	/* Input the nickname and send a message to the server */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is the register/login message*/
	/*******************************************/
	
	wmove(chatbox, 1, 1);
	wrefresh(chatbox);
	
	bzero(buffer, sizeof(buffer));
	bzero(input, sizeof(input));
	strcpy(buffer, "REGISTER");
	
	n = 8;
      		// Backspace
	while ((ch = getchar()) != 13) {    	
	      if (ch == 8 || ch == 127) {
		if (n > 0) {
		    wprintw(chatbox, "\b \b\0");
		    
		    bzero(buffer, sizeof(buffer));
		    strcat(buffer, "REGISTER");
		    strncat(buffer, input, strlen(input) - 1);
		    bzero(input, sizeof(input));
		    strncpy(input, buffer+8, strlen(buffer) - 8);
		    i--;
		    n--;
		    
		    wrefresh(chatbox);
		}
		else {
		   	wprintw(chatbox, "\b \0");
		}
	      }
	      else{
	      	    strcat(buffer, (char *)&ch);
	      	    strcat(input, (char *)&ch);
	      	    i++;
	      	    n++;
		    wprintw(chatbox, (char *)&ch);
		    wrefresh(chatbox);
		}
	}
	
	strcat(buffer, "\n");
	strcat(input, "\n");
	wclear(chatbox);
	wrefresh(chatbox);
	
	wprintw(chat, "%s", input);
	wrefresh(chat);
	
	char name[MAX];
	bzero(name, sizeof(name));
	for (int limit = 0; limit < strlen(buffer) - 8 - 1 ;limit ++){
		name[limit] = buffer[8 + limit];
	}
	
	wrefresh(chat);
	write(sockfd, buffer, sizeof(buffer));

	bzero(buffer, sizeof(buffer));
    	if (nbytes = recv(sockfd, buffer, sizeof(buffer), 0)==-1){
    	    perror("recv");
    	}
    	
    	/*inputting pasword for registration if it is registration*/
    	wprintw(chat, "%s", buffer);
    	wrefresh(chat);
    	
    	wmove(chatbox, 1, 1);
	wrefresh(chatbox);
    	
   	bzero(buffer, sizeof(buffer));
   	bzero(input, sizeof(input));
	strcpy(buffer, "password:");
	
	n = 9;
	i = 0;
	while ((ch = getchar()) != 13) {    	
	      if (ch == 8 || ch == 127) {
		if (n > 0) {
		    wprintw(chatbox, "\b \b\0");
		    
		    bzero(buffer, sizeof(buffer));
		    strcat(buffer, "password:");
		    strncat(buffer, input, strlen(input) - 1);
		    bzero(input, sizeof(input));
		    strncpy(input, buffer+9, strlen(buffer) - 9);
		    i--;
		    n--;
		    
		    wrefresh(chatbox);
		}
		else {
		   	wprintw(chatbox, "\b \0");
		}
	      }
	      else{
	      	    strcat(buffer, (char *)&ch);
	      	    strcat(input, (char *)&ch);
	      	    i++;
	      	    n++;
		    wprintw(chatbox, (char *)&ch);
		    wrefresh(chatbox);
		}
	}
	
	strcat(buffer, "\n");
	strcat(input, "\n");
	wclear(chatbox);
	wrefresh(chatbox);
	wprintw(chat, "%s", input);	
	wrefresh(chat);
	
	write(sockfd, buffer, sizeof(buffer));
	line += 15;
	
  	  // receive welcome message "welcome xx to joint the chatroom. A 	new account has been created." (registration case) or "welcome back! The message box contains:..." (login case)
	    bzero(buffer, sizeof(buffer));
	    if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
	        perror("recv");
	    }
	    
	    wprintw(chat, "%s", buffer);
	    wrefresh(chat);
	    
	    bzero(buffer, sizeof(buffer));
    
    /*****************************************************/
	/* Create a thread to receive message from the server*/
	/* pthread_t recv_server_msg_thread;*/
	/*****************************************************/

	pthread_t recv_server_msg_thread;
	struct argstruct* args = malloc(sizeof(struct argstruct));
	args->win = chat;
	args->chatwin = chatbox;
	args->recv_server_msg_thread = &recv_server_msg_thread;
	
	if (pthread_create(&recv_server_msg_thread, NULL, (void*) recv_server_msg_handler, args)) {
		perror("Could not create thread");
		return 1;
	}
	
	// chat with the server
	for (;;) {
		wmove(chatbox, 1, 1);
		wrefresh(chatbox);
	
		bzero(buffer, sizeof(buffer));
		bzero(input, sizeof(input));
		
		n = 0;
		i = 0;
		while ((ch = getchar()) != 13) {    	
		      if (ch == 8 || ch == 127) {
			if (n > 0) {
			    wprintw(chatbox, "\b \b\0");
			    
			    bzero(buffer, sizeof(buffer));
			    strncat(buffer, input, strlen(input) - 1);
			    bzero(input, sizeof(input));
			    strncpy(input, buffer, strlen(buffer));
			    i--;
			    n--;
			    
			    wrefresh(chatbox);
			}
			else {
			   	wprintw(chatbox, "\b \0");
			}
		      }
		      else{
		      	    strcat(buffer, (char *)&ch);
		      	    strcat(input, (char *)&ch);
		      	    i++;
		      	    n++;
			    wprintw(chatbox, (char *)&ch);
			    wrefresh(chatbox);
			}
		}
		
		strcat(buffer, "\n");
		strcat(input, "\n");
		wclear(chatbox);
		wrefresh(chatbox);
		wprintw(chat, "%s", input);
		line++;
		if(line > 22) {
			wclear(chat);
			wrefresh(chat);
			line = 0;
		}	
		wrefresh(chat);
		
		wmove(chatbox, 1, 1);
		wrefresh(chatbox);
				
		if ((strncmp(buffer, "EXIT", 4)) == 0) {
			wprintw(chat, "Client Exit...\n");
			/********************************************/
			/* Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
			
			write(sockfd, buffer, sizeof(buffer));
			pthread_cancel(recv_server_msg_thread);
			close(sockfd);
			endwin();
			exit(1);

		}
		else if (strncmp(buffer, "WHO", 3) == 0) {;
			line += 5;
			if(line > 22) {
				wclear(chat);
				wrefresh(chat);
				line = 0;
			}
			wprintw(chat, "Getting user list, pls hold on...\n");	
			wrefresh(chat);
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_WHO failed");
				endwin();
				exit(1);
			}
			wprintw(chat,"If you want to send a message to one of the users, pls send with the format: '#username:message'\n");
			wrefresh(chat);
			wmove(chatbox, 1, 1);
			wrefresh(chatbox);
			
		}
		else if (strncmp(buffer, "#", 1) == 0) {
			// If the user want to send a direct message to another user, e.g., aa wants to send direct message "Hello" to bb, aa needs to input "#bb:Hello"
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				wprintw(chat,"Sending direct message failed...");
				endwin();
				exit(1);
				
			}
		}
		else {
			/*************************************/
			/* Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/
			
			char username[MAX];
			bzero(username, MAX);
			strncpy(username, name, strlen(name));
			strcat(username, ": ");
			strcat(username, buffer);
			send(sockfd, username, sizeof(name), 0);
			
		}
	}
	endwin();
	return 0;
}

