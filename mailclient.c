#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h> 
#include<sys/types.h> 
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdbool.h>

#define MAX_MESSAGE_LENGTH 4000
#define MAX_BUF 120
int temp = 0;
char DName[100];
int mail_no = 0;

bool check_From_To(char* T) {
    char *a, *b;
    a = strtok(T, "@");
    b = strtok(NULL, "\n");
    if (a == NULL || b == NULL) return false;
    return true;
}

void retrieve_and_concatenate(int popsockfd, int i, int flag, int is_mark) {
    char message[MAX_MESSAGE_LENGTH];
    char BUF[MAX_BUF];
    bzero(message, sizeof(message));

    // Send the RETR command
    bzero(BUF, sizeof(BUF));
    sprintf(BUF, "RETR %d\r\n", i);
    send(popsockfd, BUF, strlen(BUF), 0);

    if (is_mark) {
        int bytes_received;
        bzero(BUF, sizeof(BUF));
        bytes_received = recv(popsockfd, BUF, MAX_BUF - 1, 0);
        if (bytes_received < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        printf("%d. Message Marked for Delete\n", i);
        return;
    }

    // Receive and concatenate messages until "." is received
    do {
        int bytes_received;
        bzero(BUF, sizeof(BUF));
        bytes_received = recv(popsockfd, BUF, MAX_BUF - 1, 0);
        if (bytes_received < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        strcat(message, BUF);
    } while ((strncmp(BUF + strlen(BUF) - 5, "\r\n.\r\n", 5) != 0));
    
    char *message_copy = strdup(message);
    char *status = strtok(message_copy, "\n"); // status will contain the first line
    char *content = strtok(NULL, ""); // actual content of the message

    if (flag == 0)
        printf("%s\n", content);
    else if (flag == 1) {
        char *from = strtok(message, "\r\n");
        //printf("1st line: %s\n", from);
        from = strtok(NULL, "\r\n");
        char *to = strtok(NULL, "\r\n");
        char *Time = strtok(NULL, "\r\n");
        char *subject = strtok(NULL, "\r\n");
        strcat(from, "\r\n");
        strcat(subject, "\r\n");

        from = strtok(from, " ");
        from = strtok(NULL, "\r\n");

        subject = strtok(subject, " ");
        subject = strtok(NULL, "\r\n");
      //  printf("=> %s\n",Time);

        printf("%d. <%s> <%s> <%s>\n", i, from,Time, subject);
    }
}

void print_menu() {
    printf("\n----------------Client Menu----------------\n\n");
    printf("1. Manage Mail: Shows the stored mail\n");
    printf("2. Send Mail\n");
    printf("3. Quit: Quits the program\n");
}

void send_mail(int sockfd, char* usname, char* passwd) {
    char BUF[MAX_BUF];
    char message[MAX_MESSAGE_LENGTH] = "";

    printf("Enter the message (type '.' on a new line to end):\n");

    // Loop to read the message until a line with a single '.' is encountered
    while (1) {
        fgets(BUF, MAX_BUF, stdin);
        strncat(message, BUF, sizeof(message) - strlen(message) - 1);
        if (strcmp(BUF, ".\n") == 0) {
            break;  // Exit the loop when '.' is encountered
        }
    }

    char *from, *to, *subject;
    // Extract From, To, and Subject fields
    char message1[4000];
    strcpy(message1, message);
    from = strtok(message, "\n");
    to = strtok(NULL, "\n");
    subject = strtok(NULL, "\n");

    from = strtok(from, " ");
    from = strtok(NULL, "\n");

    to = strtok(to, " ");
    to = strtok(NULL, "\n");

    subject = strtok(subject, " ");
    subject = strtok(NULL, "\n");

    int i = 0;
    while (1) {
        memset(BUF, 0, sizeof(BUF));
        recv(sockfd, BUF, MAX_BUF, 0);

        printf("\nS:%s", BUF);
        if (strncmp(BUF, "220", 3) == 0) {
            memset(BUF, 0, sizeof(BUF));
            sscanf(BUF, "220 <%[^>]", DName);
            sprintf(BUF, "HELO %s\r\n", DName);
            send(sockfd, BUF, sizeof(BUF), 0);
            printf("\nC:%s\n", BUF);
        } else if (strncmp(BUF, "250", 3) == 0) {
            i++;
            if (i == 1) {
                memset(BUF, 0, sizeof(BUF));
                sprintf(BUF, "MAIL FROM: <%s>\r\n", from);
                send(sockfd, BUF, sizeof(BUF), 0);
                printf("\nC:%s\n", BUF);
            }
            if (i == 2) {
                memset(BUF, 0, sizeof(BUF));
                sprintf(BUF, "RCPT TO: <%s>\r\n", to);
                send(sockfd, BUF, sizeof(BUF), 0);
                printf("\nC:%s\n", BUF);
            }
            if (i == 3) {
                memset(BUF, 0, sizeof(BUF));
                sprintf(BUF, "DATA\r\n");
                send(sockfd, BUF, sizeof(BUF), 0);
                printf("\nC:%s\n", BUF);
            }
            if (i == 4) {
                memset(BUF, 0, sizeof(BUF));
                sprintf(BUF, "QUIT\r\n");
                send(sockfd, BUF, sizeof(BUF), 0);
                printf("\nC:%s\n", BUF);
            }
        } else if (strncmp(BUF, "354", 3) == 0) {
            char *token;
            token = strtok(message1, "\n");
            while (strcmp(token, ".") != 0) {
                memset(BUF, 0, sizeof(BUF));
                sprintf(BUF, "%s\r\n", token);
                send(sockfd, BUF, strlen(BUF), 0);
                printf("\nC:%s\n", BUF);

                memset(BUF, 0, sizeof(BUF));
                token = strtok(NULL, "\n");
            }
            memset(BUF, 0, sizeof(BUF));
            sprintf(BUF, "%s\r\n", token);
            send(sockfd, BUF, strlen(BUF), 0);
            printf("\nC:%s\n", BUF);
        } else if (strncmp(BUF, "550", 3) == 0) {
            memset(BUF, 0, sizeof(BUF));
            printf("\n\nNo Such Receiver exists !!\n");
            break;
        } else if (strncmp(BUF, "221", 3) == 0) {
            memset(BUF, 0, sizeof(BUF));
            printf("\nC: <client hangs up>\n\n");
            break;
        }
    }
}

void manage_mail(int popsockfd, char* usname, char* passwd) {
    // printf("n==1\n");
    char BUF[MAX_BUF];
    recv(popsockfd, BUF, MAX_BUF, 0);
    if (strncmp(BUF, "+OK", 3) == 0) { // Send USER
        printf("\nGreetings Received :)\n");
        sprintf(BUF, "USER %s\r\n", usname);
        send(popsockfd, BUF, sizeof(BUF), 0);
        // printf("Sent User Name Successfully\n");
    } else {
        printf("-ERR receving greetings\n");
        return;
    }

    recv(popsockfd, BUF, MAX_BUF, 0);
    if (strncmp(BUF, "+OK", 3) == 0) { // Send PASS
       // printf("+OK received: passwd\n");
        sprintf(BUF, "PASS %s\r\n", passwd);
        send(popsockfd, BUF, sizeof(BUF), 0);
        // printf("Sent Password Successfully\n");
    } else {
        printf("-ERR receiving after sending PASS \n");
        return;
    }

    recv(popsockfd, BUF, MAX_BUF, 0);
    if (strncmp(BUF, "+OK", 3) == 0) {
        printf("\nLogin Successful :)\n");
        //printf("recieved=%s\n", BUF);
        bzero(BUF, sizeof(BUF));
        sprintf(BUF, "STAT");
        send(popsockfd, BUF, sizeof(BUF), 0); // sending STAT
        //printf("Sent: %s\n", BUF);
        recv(popsockfd, BUF, MAX_BUF, 0); // receiving no_of_msgs
        strcat(BUF, "\0");
        
       // printf("Received: %s\n", BUF);
        int no_of_msgs, siz_of_msgs;

        sscanf(BUF, "+OK %d %d", &no_of_msgs, &siz_of_msgs);
       // printf("no_of_msgs: %d\n", no_of_msgs);
       // printf("siz_of_msgs: %d\n", siz_of_msgs);
        int Del[no_of_msgs + 1];
        for (int i = 0; i < no_of_msgs + 1; i++) {
            Del[i] = 0; // Initialize each element to 0
        }
        
        while (mail_no != -1) {
            //printf("came\n");
            printf("\n-----------------------------------------\n");
            printf("\t\tInbox:\n\n");
            printf("S.No     Sender\t\t\t  Time\t\t\t   Subject\n\n");
            for (int i = 1; i <= no_of_msgs; i++) {
                retrieve_and_concatenate(popsockfd, i, 1, Del[i]);
            } // end of for loop RETR
            printf("\n-----------------------------------------\n");
            printf("\n\tInbox Menu:\n");
            printf("\n0  : Reset Mails Marked for Delete.\n%d-%d: To view the Corresponding Mail.\n-1 :To Quit\nEnter: ",1,no_of_msgs);
            scanf("%d", &mail_no);
            bool valid_no = (mail_no >= 1) && (mail_no <= no_of_msgs);
            if (mail_no == 0) {
                bzero(BUF, sizeof(BUF));
                sprintf(BUF, "RSET\r\n");
                send(popsockfd, BUF, sizeof(BUF), 0);
                memset(Del, 0, sizeof(Del));
                printf("Reset Done !!\n");
                continue;
            }
            if (mail_no == -1) {
                bzero(BUF, sizeof(BUF));
                sprintf(BUF, "QUIT\r\n");
                send(popsockfd, BUF, sizeof(BUF), 0);
                continue;
            } else if (!valid_no) {
                printf("Mail does not Exist !!\n");
                continue;
            } else if (valid_no) {
                printf("\n-----------------------------------------\n");
                printf("Mail info:\n\n");
                retrieve_and_concatenate(popsockfd, mail_no, 0, Del[mail_no]);
                printf("-----------------------------------------\n");
                getchar();
                printf("Options for Selected Mail:\n");
                printf("\t\td: delete mail.\n\t\tAny key: skip delete.\n\t\tEnter: ");
                char ch = getchar();
                if (ch == 'd') {
                    Del[mail_no] = 1;
                    bzero(BUF, sizeof(BUF));
                    sprintf(BUF, "DELE %d\r\n", mail_no);
                    send(popsockfd, BUF, sizeof(BUF), 0);
                    bzero(BUF, sizeof(BUF));
                    recv(popsockfd, BUF, MAX_BUF, 0);
                    if(strncmp(BUF, "+OK",3) != 0){
                        printf("-ERR in Deleting !!\n");
                    }
                    //printf("DELE: Recv: %s\n", BUF);

                } else {
                        continue;
                }
            }
        } // end of while loop
    } // end of if (+OK)
    else {
        printf("%s\n", BUF);
        printf("Invalid Credentials :(\n\n");
        temp = 1;
        return;
    }
}

int main(int argc, char *argv[]) {
    // argv[1]: server_IP, argv[2]: smtp_port, argv[3]: pop3_port
    if (argc != 4) {
        printf("Run with server_IP, smtp_port, pop3_port as command line arguments\n");
        printf("exiting...\n");
        exit(EXIT_FAILURE);
    }

    int sockfd, popsockfd;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    inet_aton(argv[1], &serv_addr.sin_addr);
    serv_addr.sin_port = htons(atoi(argv[2]));

    struct sockaddr_in popserv_addr;
    popserv_addr.sin_family = AF_INET;
    inet_aton(argv[1], &popserv_addr.sin_addr);
    popserv_addr.sin_port = htons(atoi(argv[3]));

    char usname[20], passwd[20];
    printf("UserName: ");
    scanf("%s", usname);
    printf("Password: ");
    scanf("%s", passwd);
    
    while (1) {
        
        int n;
        print_menu();
        printf("\nSelect: ");
        scanf("%d", &n);
        if (n == 2) {
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Unable to create socket\n");
                exit(EXIT_FAILURE);
            }
            if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(EXIT_FAILURE);
            }
            printf("C: <client connects to SMTP port>\n");
            send_mail(sockfd, usname, passwd);
            close(sockfd);
        } else if (n == 1) {
            mail_no = 0;
            if ((popsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Unable to create socket\n");
                exit(EXIT_FAILURE);
            }

            if ((connect(popsockfd, (struct sockaddr *) &popserv_addr, sizeof(popserv_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(EXIT_FAILURE);
            }

            printf("\nConnected to POP3 Server !!\n");
            manage_mail(popsockfd, usname, passwd);
            close(popsockfd);
        } else if (n == 3) {
            printf("Closing Program..\n");
            break;
        } else {
            printf("choose valid option !!\n");
            continue;
        }
    }

    return 0;
}
