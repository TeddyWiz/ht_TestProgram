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
#include "tcp_server_cmd.h"

#define MAX_CLIENTS  250
#define BUFFER_SIZE  1024


int client_sockets[MAX_CLIENTS] = {0};
int last_sock = 0;
FILE *log_file = NULL;
time_t log_start_time;
char log_filename[128];


char ch2hex(char in)
{
    char result = 0xFF;
    if((in >= '0')&&(in <='9'))
    {
        result = in -'0';
    }
    else if((in >= 'a')&&(in <= 'f'))
    {
        result = in - 'a' + 0x0a;
    }
    else if((in >= 'A')&&(in <= 'F'))
    {
        result = in - 'A' + 0x0a;
    }
    else
    {
        printf("[%c] hex Error!\n", in);
    }
    return result;
}

char str2hex(char *msg)
{
    char result = 0;
    result = ch2hex(*msg);
    //printf("hex1=%02x\n", (result)&0x000000FF);
    result = result << 4;
    result = result | ch2hex(*(msg+1));
    //printf("hex2=%02x\n", (result)&0x000000FF);
    return result;
}

int hex2str(char *InMsg, int InLen, char *OutMsg)
{
    int sendLen =0;
    for(int i=0; i<InLen;i++)
    {
        sendLen += sprintf(OutMsg+sendLen, "%02X",(*(InMsg + i)&0x000000FF));
    }
    return sendLen;
}

unsigned char checkSum(char *buffer, int len)
{
    int i =0;
    unsigned char result = 0;
    for(i=0;i<len; i++)
    {
        result += *(buffer+i);
    }
    return result;
}

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

int send_cmd_all(char *buffer, int size)
{
    int i = 0, temp_Socket = 0, sent_bytes = 0;
    for(i=0;i<last_sock;i++)
    {
        printf("sock[%d] : %d\n", i, client_sockets[i]);
        temp_Socket = client_sockets[i];
        sent_bytes = send(temp_Socket, buffer, size, 0);
        if (sent_bytes > 0)
            write_log("Sent to %d: %s\n", temp_Socket, buffer);
        else
            write_log("Failed to send to %d. Message: %s\n", temp_Socket, buffer);
    }
    return 0;
}

int Recv_Protocol_Process(char *buffer, int size)
{
    unsigned char cal_checksum = 0;
    Meter_proto_header *tempHeader = (Meter_proto_header *)buffer;
    switch(tempHeader->Type)
    {
        case 0x01:  //CONNECT_INFO
            CONNECT_INFO *temp_Data = (CONNECT_INFO *)buffer;
            cal_checksum = checkSum(buffer+2, temp_Data->Header.Len);
            write_log("Checksum Cal: %02x msg: %02x\n", cal_checksum&0x000000FF, temp_Data->Chk&0x000000FF);
            write_log("======= CONNECT_INFO =======\n");
            write_log(" Type | Line |    S/N     |        IMEI      | CheckSum \n");
            write_log(" %02X   | %d    | %02X%02X%02X%02X%02X | %02X%02X%02X%02X%02X%02X%02X%02X | %02X\n",
                temp_Data->Header.Type&0x000000FF,
                temp_Data->Line, temp_Data->S_N[0]&0x000000FF, temp_Data->S_N[1]&0x000000FF
                , temp_Data->S_N[2]&0x000000FF, temp_Data->S_N[3]&0x000000FF, temp_Data->S_N[4]&0x000000FF
                , temp_Data->IMEI[0]&0x000000FF, temp_Data->IMEI[1]&0x000000FF
                , temp_Data->IMEI[2]&0x000000FF, temp_Data->IMEI[3]&0x000000FF
                , temp_Data->IMEI[4]&0x000000FF, temp_Data->IMEI[5]&0x000000FF
                , temp_Data->IMEI[6]&0x000000FF, temp_Data->IMEI[7]&0x000000FF
                , temp_Data->Chk&0x000000FF
            );
        break;
        case 0xb2: //METER_CONF_RES
            METER_CONF_RES *temp_Data1 = (METER_CONF_RES *)buffer;
            unsigned short curr_temp = 0, set_temp = 0;
            unsigned short _q3=0, _qt=0, _qs=0, _q2=0, _q1=0;
            curr_temp = temp_Data1->CurTemp[1]<<8 | temp_Data1->CurTemp[0];
            set_temp = temp_Data1->SetTemp[1]<<8 | temp_Data1->SetTemp[0];
            _q3 = temp_Data1->Q3[1]<<8 | temp_Data1->Q3[0];
            _qt = temp_Data1->Qt[1]<<8 | temp_Data1->Qt[0];
            _qs = temp_Data1->Qs[1]<<8 | temp_Data1->Qs[0];
            _q2 = temp_Data1->Q2[1]<<8 | temp_Data1->Q2[0];
            _q1 = temp_Data1->Q1[1]<<8 | temp_Data1->Q1[0];
            cal_checksum = checkSum(buffer+2, temp_Data1->Header.Len);
            write_log("Checksum Cal: %02x msg: %02x\n", cal_checksum&0x000000FF, temp_Data1->Chk&0x000000FF);
            write_log("======= METER_CONF_RES =======\n");
            write_log(" Type | Caliber | Serial   |  Q3  |  Qt  |  Qs  |  Q2  |  Q1  | Set Temp | Cur Temp | Maker | FW | Result | CheckSum\n");
            write_log("  %02x  |    %02X   | %02X%02X%02X%02X | %4d | %4d | %4d | %4d | %4d |   %4d   |   %4d   |  %4d | %2d |   %02X   | %02X\n",
            temp_Data1->Header.Type&0x000000FF, temp_Data1->Cliber&0x000000FF, temp_Data1->Serial[3]&0x000000FF,
            temp_Data1->Serial[2]&0x000000FF,temp_Data1->Serial[1]&0x000000FF,temp_Data1->Serial[0]&0x000000FF,
            _q3, _qt, _qs, _q2, _q1, curr_temp, set_temp, temp_Data1->Maker, temp_Data1->FW, temp_Data1->Result&0x000000FF,
            temp_Data1->Chk&0x000000FF
            );
        break;
        case 0xbb:  //METER_CALIB_S_RES
            METER_CALIB_S_RES *temp_Data2 = (METER_CALIB_S_RES *)buffer;
            cal_checksum = checkSum(buffer+2, temp_Data2->Header.Len);
            write_log("Checksum Cal: %02x msg: %02x\n", cal_checksum&0x000000FF, temp_Data2->Chk&0x000000FF);
            write_log("======= METER_CALIB_S_RES =======\n");
            write_log(" Type | Status |CheckSum\n");
            write_log("  %02X  |   %02X   | %02X\n", 
                temp_Data2->Header.Type&0x000000FF, temp_Data2->Status&0x000000FF, temp_Data2->Chk&0x000000FF
            );
        break;
        case 0xbc:  //METER_CALIB_R_RES
            METER_CALIB_R_RES *temp_Data3 = (METER_CALIB_R_RES *)buffer;
            unsigned int Server_Flow = 0, Meter_Flow = 0;
            Server_Flow = temp_Data3->ServerFlowRate[3]<<24 | temp_Data3->ServerFlowRate[2]<<16 | temp_Data3->ServerFlowRate[1]<<8 |temp_Data3->ServerFlowRate[0];
            Meter_Flow = temp_Data3->MeterFlowRate[3]<<24 | temp_Data3->MeterFlowRate[2]<<16 | temp_Data3->MeterFlowRate[1]<<8 |temp_Data3->MeterFlowRate[0];
            cal_checksum = checkSum(buffer+2, temp_Data3->Header.Len);
            write_log("Checksum Cal: %02x msg: %02x\n", cal_checksum&0x000000FF, temp_Data3->Chk&0x000000FF);
            write_log("======= METER_CALIB_R_RES =======\n");
            write_log(" Type | Qn | ServerFlowRate |  MeterFlowRate | Result |CheckSum\n");
            write_log("  %02X  | %02X |    %8ld    |    %8ld    |   %02X   | %02X\n", 
                temp_Data3->Header.Type&0x000000FF, temp_Data3->Qn&0x000000FF, Server_Flow, Meter_Flow,
                temp_Data3->Result,
                temp_Data3->Chk&0x000000FF
            );
        break;
        case 0x08:  //METER_VALUE_RES
            METER_VALUE_RES *temp_Data4 = (METER_VALUE_RES *)buffer;
            cal_checksum = checkSum(buffer+2, temp_Data4->Header.Len);
            write_log("Checksum Cal: %02x msg: %02x\n", cal_checksum&0x000000FF, temp_Data4->Chk&0x000000FF);
            write_log("======= METER_VALUE_RES =======\n");
            write_log(" Type |  Serial  |   Value  | CheckSum\n");
            write_log("  %02X  | %02X%02X%02X%02X | %02X%02X%02X%02X | %02X\n", 
                temp_Data4->Header.Type&0x000000FF,
                temp_Data4->Serial[3]&0x000000FF,temp_Data4->Serial[2]&0x000000FF,
                temp_Data4->Serial[1]&0x000000FF,temp_Data4->Serial[0]&0x000000FF,
                temp_Data4->Value[3]&0x000000FF,temp_Data4->Value[2]&0x000000FF,
                temp_Data4->Value[1]&0x000000FF,temp_Data4->Value[0]&0x000000FF,
                temp_Data4->Chk&0x000000FF
            );
        break;
        default:
            printf("Recv Protocol Type Error[%02x]\n", (tempHeader->Type)&0x000000FF);
        break;
    }
    return 0;
}

int main() {
    int server_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set read_fds, temp_fds;
    char buffer[BUFFER_SIZE];
    int max_fd;
    char METER_CALIB_START[13] = "C103AB0100AC";
    char METER_VALUE_REQ[11] = "C1025B015C";
    char METER_CALIB_FINISH[11] = "C102070108";
    char *transbuffer = NULL;
    char strBuffer[100];
    METER_CALIB_START[12]=0;
    METER_VALUE_REQ[10]=0;
    METER_CALIB_FINISH[10]=0;
    int testCount = 0;
    #if 0
    Meter_Conf_Set temp_meter_conf_set = {
    .Header = {
        .STX = 0xC1,
        .Len = 0x14,
        .Type = 0xA0,
        .Ver = 0x01
    },
    .Calib = 0x01,
    .Serial = {0x00, 0x00, 0x00, 0x00},
    .Q3 = {0x00, 0x00},
    .Qt = {0x00, 0x00},
    .Qs = {0x00, 0x00},
    .Q2 = {0x00, 0x00},
    .Q1 = {0x00, 0x00},
    .SetTemp = {0xFB, 0x00},
    .Maker = 0x01,
    .Chk = 0x9E
    };
    #endif

    #if 1
    char temp_conf_set[] = {
    0xC1, 0x14, 0xA0, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFB, 0x00, 0x01, 0x9E
    };
    #endif

    char temp_FR_Set[] ={
    0xC1, 0x07, 0xAC, 0x01, 0x04, 0x01, 0x04, 0x00, 0x00, 0xB6
    };

    char temp_Flow_Set[]={
        0xC1, 0x07, 0xAA, 0x01, 0x04, 0x0F, 0x27, 0xEB, 0x00, 0xD0
    };

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
                        last_sock = i+1;
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

            if ((strncmp(buffer, "EXIT", 4) == 0) || (strncmp(buffer, "exit", 4) == 0)) {
                write_log("EXIT command received. Closing all clients.\n");
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    close_client(i);
                }
                break;
            } else if ((strncmp(buffer, "SEND", 4) == 0) ||(strncmp(buffer, "send", 4) == 0)){
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
            } else if ((strncmp(buffer, "DISCON", 6) == 0) || (strncmp(buffer, "discon", 6) == 0)) {
                int sock_num = atoi(buffer + 6);
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (client_sockets[i] == sock_num) {
                        write_log("Disconnecting client socket %d\n", sock_num);
                        close_client(i);
                        FD_CLR(sock_num, &read_fds);
                        break;
                    }
                }
            }else if (strncmp(buffer, "looptest", 8) == 0) {
                testCount = atoi(buffer + 8);
                printf("loop count : %d\n", testCount);
                testCount--;
                send_cmd_all(METER_VALUE_REQ, strlen(METER_VALUE_REQ));
                #if 0
                for(int i = 0; i < count; i++)
                {
                    send_cmd_all(METER_VALUE_REQ, strlen(METER_VALUE_REQ));
                    sleep(2);   //2초 지
                    usleep(500000); // 0.5초 = 500,000 마이크로초 지연
                }
                printf("loop test finish\n");
                #endif
            }else if (strncmp(buffer, "cmd", 3) == 0) {
                unsigned char temp_cmd = 0;
                int sendLen = 0;
                //printf("cmd 4:%c 5:%c\n",*(buffer+4),*(buffer+5));
                temp_cmd = str2hex(buffer + 4);
                switch(temp_cmd)
                {
                    case 0xA0:  //METER_CONF _SET
                        transbuffer = temp_conf_set;
                        Meter_Conf_Set *tempConfSet = (Meter_Conf_Set *)temp_conf_set;
                        unsigned short tempTemp =0;
                        int tempTrans = 0;
                        tempTrans = atoi(buffer+7);
                        printf("in temp =%d\n", tempTrans);
                        tempConfSet->SetTemp[0] = tempTrans & 0x000000FF;
                        tempConfSet->SetTemp[1] = (tempTrans >> 8)& 0x000000FF;
                        printf("Set Temp = %02X %02X\n",tempConfSet->SetTemp[0], tempConfSet->SetTemp[1]); 
                        tempConfSet->Chk = checkSum(transbuffer+2,tempConfSet->Header.Len);
                        printf("CheckSUM = %02x\n", tempConfSet->Chk &0x000000FF);
                        sendLen = hex2str(transbuffer, sizeof(Meter_Conf_Set), strBuffer);
                        #if 0
                        for(int i=0; i<sizeof(Meter_Conf_Set);i++)
                        {
                            sendLen += sprintf(strBuffer+sendLen, "%02X",(*(transbuffer + i)&0x000000FF));
                        }
                        #endif
                        send_cmd_all(strBuffer, sendLen);
                    break;
                    case 0xAA:
                        char *tempNext = NULL;
                        transbuffer = temp_Flow_Set;
                        Meter_Flow_Set *tempFlowSet = (Meter_Flow_Set *)temp_Flow_Set;
                        int temp_qn1 = 0;
                        int temp_flow = 0;
                        int temp_value = 0;
                        temp_qn1 = atoi(buffer+7);
                        temp_flow = atoi(buffer+9);
                        temp_value = atoi(strstr(buffer+10," "));
                        printf("qn:%d, flow:%d, value:%d\n", temp_qn1, temp_flow, temp_value);
                        if(temp_flow < 0)
                        {
                            temp_flow = 0x8000 |(0xFFFF-temp_flow +1);
                        }
                        tempFlowSet->Qn = temp_qn1;
                        tempFlowSet->Flow[0] = temp_flow&0x000000FF;
                        tempFlowSet->Flow[1] = (temp_flow>>8)&0x000000FF;
                        tempFlowSet->Value[0] = temp_value&0x000000FF;
                        tempFlowSet->Value[1] = (temp_value>>8)&0x000000FF;
                        printf("Flow %02x %02x value %02x %02x\n",tempFlowSet->Flow[0],tempFlowSet->Flow[1], tempFlowSet->Value[0], tempFlowSet->Value[1]);
                        tempFlowSet->Chk = checkSum(transbuffer+2, tempFlowSet->Header.Len);
                        printf("CheckSum = %02x\n",tempFlowSet->Chk);
                        sendLen = hex2str(transbuffer, sizeof(Meter_Flow_Set), strBuffer);
                        send_cmd_all(strBuffer, sendLen);
                    break;
                    case 0xAB:  //METER_CALIB_START
                        printf("send %x\n", temp_cmd);
                        send_cmd_all(METER_CALIB_START, strlen(METER_CALIB_START));
                    break;
                    case 0xAC:  //METER_FLOW_RATE_SET
                        transbuffer = temp_FR_Set;
                        Meter_Flow_Rate_Set *tempFRset = (Meter_Flow_Rate_Set *)temp_FR_Set;
                        int temp_qn = 0;
                        long tempFlowRate = 0;
                        temp_qn = atoi(buffer+7);
                        tempFlowRate = atol(buffer+9);
                        printf("qn : %d, FlowRate : %ld\n", temp_qn, tempFlowRate);
                        tempFRset->Qn = temp_qn & 0x000000FF;
                        tempFRset->FlowRate[0] = (tempFlowRate) & 0x000000FF;
                        tempFRset->FlowRate[1] = (tempFlowRate>>8) & 0x000000FF;
                        tempFRset->FlowRate[2] = (tempFlowRate>>16) & 0x000000FF;
                        tempFRset->FlowRate[3] = (tempFlowRate>>24) & 0x000000FF;
                        printf("FlowRateHex  %02X %02X %02X %02X\n",tempFRset->FlowRate[0], tempFRset->FlowRate[1],
                        tempFRset->FlowRate[2], tempFRset->FlowRate[3]);
                        tempFRset->Chk = checkSum(transbuffer+2, tempFRset->Header.Len);
                        printf("CheckSum = %02x\n",tempFRset->Chk);
                        sendLen = hex2str(transbuffer, sizeof(Meter_Flow_Rate_Set), strBuffer);
                        send_cmd_all(strBuffer, sendLen);
                    break;
                    case 0x5B:  //METER_VALUE_REQ
                        send_cmd_all(METER_VALUE_REQ, strlen(METER_VALUE_REQ));
                    break;
                    case 0x07:  //METER_CALIB_FINISH
                        send_cmd_all(METER_CALIB_FINISH, strlen(METER_CALIB_FINISH));
                    break;
                    default:
                        printf("CMD Error[%02X]\n", temp_cmd);
                    break;
                }
            }
            else if ((strncmp(buffer, "HELP", 4) == 0) || (strncmp(buffer, "help", 4) == 0)) {
                printf("==== CMD List ==== \n");
                printf("SEND(send) <socket> : send message to TCP Client. after enter key write message.\n");
                printf("DISCON(discon) <socket> : disconnect socket\n");
                printf("looptest <count> : Send a 5B message to all TCP clients according to the entered number.\n");
                printf("cmd <Type> <parameter>\n");
                printf("\t- A0 <temperature> ex)cmd a0 251 -> METER_CONF _SET 25.1\n");
                printf("\t- AA <qn> <offset> <Value> ex)cmd aa 5 -9999 235 -> METER_FLOW_SET -99.99 23.5\n");
                printf("\t- AB ex)cmd ab ->METER_CALIB_START\n");
                printf("\t- AC <qn> <flowrate> ex)cmd ac 5 1054 -> METER_FLOW_RATE_SET 10.54L/h\n");
                printf("\t- 5B ex)cmd 5b ->METER_VALUE_REQ\n");
                printf("\t- 07 ex)cmd 07 ->METER_CALIB_FINISH\n");
                
            }
            else {
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
                        Recv_Protocol_Process(buffer, bytes_read);
                        if(testCount>0)
                        {                            
                            printf("loop test count : %d\n", testCount);
                            //send_cmd_all(METER_VALUE_REQ, strlen(METER_VALUE_REQ));
                            int sent_bytes = send(sd, METER_VALUE_REQ, strlen(METER_VALUE_REQ), 0);
                            if (sent_bytes > 0)
                                write_log("Sent to %d: %s\n", sd, METER_VALUE_REQ);
                            else
                                write_log("Failed to send to %d. Message: %s\n", sd, METER_VALUE_REQ);
                            testCount--;
                        }
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

    close(server_socket);
    if (log_file) fclose(log_file);
    write_log("TCP Server terminated.\n");
    return 0;
}
