/*
 * TCP 서버 프로그램 (Ubuntu)
 * - 클라이언트 연결 관리
 * - 명령어 기반 제어 (EXIT, SEND, DISCON)
 * - 로그 파일 저장 (6시간 단위)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>

#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE  1024

int client_sockets[MAX_CLIENTS] = {0};
FILE *log_file = NULL;
time_t log_start_time;
char log_filename[128];

void write_log(const char *format, ...) {
    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (difftime(now, log_start_time) >= 21600) { // 6시간
        if (log_file) fclose(log_file);
        strftime(log_filename, sizeof(log_filename), "log_%Y%m%d_%H%M%S.txt", tm_info);
        log_file = fopen(log_filename, "a");
        log_start_time = now;
    }

    char time_buf[64];
    char temp_buff[500]={0,};
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", time_buf);
    //vprintf(format, args);
    vsprintf(temp_buff, format, args);
    printf("%s", temp_buff);

    if (log_file) {
        fprintf(log_file, "[%s] ", time_buf);
        //vfprintf(log_file, format, args);
        fprintf(log_file, "%s", temp_buff);
        fflush(log_file);
    }
    //vprintf(format, args);

    va_end(args);
}

void close_client(int index) {
    if (client_sockets[index] > 0) {
        write_log("Client %d disconnected\n", client_sockets[index]);
        close(client_sockets[index]);
        client_sockets[index] = 0;
    }
}

int main() {
    int server_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set read_fds, temp_fds;
    char buffer[BUFFER_SIZE];
    int max_fd;

    printf("Enter port to open: ");
    scanf("%d", &port);
    getchar();  // consume newline

    // 로그 초기화
    time(&log_start_time);
    strftime(log_filename, sizeof(log_filename), "log_%Y%m%d_%H%M%S.txt", localtime(&log_start_time));
    log_file = fopen(log_filename, "a");

    // 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
        exit(EXIT_FAILURE);
    }

    listen(server_socket, 5);
    write_log("TCP Server started on port %d\n", port);

    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    max_fd = server_socket;

    while (1) {
        temp_fds = read_fds;
        if (select(max_fd + 1, &temp_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            break;
        }

        // 클라이언트 접속
        if (FD_ISSET(server_socket, &temp_fds)) {
            int new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
            if (new_socket >= 0) {
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        FD_SET(new_socket, &read_fds);
                        if (new_socket > max_fd) max_fd = new_socket;
                        write_log("New client connected: socket %d\n", new_socket);
                        break;
                    }
                }
            }
        }

        // 표준 입력 명령어 처리
        if (FD_ISSET(STDIN_FILENO, &temp_fds)) {
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) continue;
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strncmp(buffer, "EXIT", 4) == 0) {
                write_log("EXIT command received. Closing all clients.\n");
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    close_client(i);
                }
                break;
            } else if (strncmp(buffer, "SEND", 4) == 0) {
                int sock_num = atoi(buffer + 4);
                write_log("Preparing to send message to socket %d\n", sock_num);
                printf("Enter message to send: ");
                fflush(stdout);

                if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                    buffer[strcspn(buffer, "\n")] = '\0';
                    char send_copy[BUFFER_SIZE];
                    strncpy(send_copy, buffer, BUFFER_SIZE);
                    send_copy[BUFFER_SIZE - 1] = '\0';

                    int sent_bytes = send(sock_num, send_copy, strlen(send_copy), 0);
                    if (sent_bytes > 0)
                        write_log("Sent to %d: %s\n", sock_num, send_copy);
                    else
                        write_log("Failed to send to %d. Message: %s\n", sock_num, send_copy);
                }
            } else if (strncmp(buffer, "DISCON", 6) == 0) {
                int sock_num = atoi(buffer + 6);
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (client_sockets[i] == sock_num) {
                        write_log("Disconnecting client socket %d\n", sock_num);
                        close_client(i);
                        FD_CLR(sock_num, &read_fds);
                        break;
                    }
                }
            } else {
                write_log("Unknown command: %s\n", buffer);
            }
        }

        // 클라이언트 메시지 처리
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &temp_fds)) {
                int bytes_read = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_read <= 0) {
                    close_client(i);
                    FD_CLR(sd, &read_fds);
                } else {
                    buffer[bytes_read] = '\0';
                     if((0x000000FF&buffer[0])==0xC1)
                    {
                        char recv_copy[BUFFER_SIZE*2]={0,};
                        int bytes_index = 0;
                        for(int j=0; j<bytes_read ; j++)
                        {
                                bytes_index += sprintf(recv_copy+bytes_index,"%02x",(0x000000FF&buffer[j]));
                        }
                        recv_copy[bytes_index] = '\0';
                        write_log("Receive from %d HEX[%d] : %s\n",sd,bytes_index,recv_copy);

                    }
                    else
                    {
                        char recv_copy[BUFFER_SIZE];
                        strncpy(recv_copy, buffer, BUFFER_SIZE);
                        recv_copy[BUFFER_SIZE - 1] = '\0';
                        write_log("Received from %d: %s\n", sd, recv_copy);
                    }
                }
                }
            }
        }
    }

    close(server_socket);
    if (log_file) fclose(log_file);
    write_log("TCP Server terminated.\n");
    return 0;
}
