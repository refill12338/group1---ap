#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "JSON_action.h"
#include "JSON_request_handle.h"
#include "config.h"
#include "update.h"
#include "help.h"
int main()
{
	char ip_address[100] = {};
	char DNS_address[200] = {};
	ConfigQuery ConfigBuf;
	ConfigBuf = Load_Config_String("./config/connect_config", "IP_Address", ip_address, sizeof(ip_address));
	if(ConfigBuf.DataType == -1)
	{
		ConfigBuf = Load_Config_String("./config/connect_config", "DNS_Address", DNS_address, sizeof(DNS_address));
		if(ConfigBuf.DataType == -1)
		{
			puts("[main.c] No proper config");
			return -3;
		}
		printf("[main.c] Load DNS: %s\n", DNS_address);
		struct hostent* HostInfo;

		/* get IP address from name */
		HostInfo = gethostbyname(DNS_address);

		if(!HostInfo)
		{
		    printf("[main.c] Could not resolve host name\n");
		    return -10;
		}
		sprintf(ip_address, "%hhu.%hhu.%hhu.%hhu", HostInfo->h_addr_list[0][0], HostInfo->h_addr_list[0][1], HostInfo->h_addr_list[0][2], HostInfo->h_addr_list[0][3]);
	}
	else
	{
		printf("[main.c] Load IP: %s\n", ip_address);
	}
	ConfigBuf = Load_Config_IntData("./config/connect_config", "Port");
	int port = ConfigBuf.IntData;
	printf("[main.c] Load Port: %d\n", port);


	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = PF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ip_address);
	serveraddr.sin_port = htons(port);

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1)
	{
		puts("[main.c] Socket Creation Failure");
		return -1;
	}
	int connect_fail = connect(socket_fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if(connect_fail == -1)
	{
		printf("[main.c] Connect to %s fail\n", ip_address);
		return -2;
	}
	printf("[main.c] Connected to %s\n", ip_address);

	char send_buf[1000];
	char send_buf_buf[1000];
	char command[1000];
	char recv_buf[1000];
	int send_fail;
	fd_set read_fd;
	int action;
	int Controller_State = -1;

	int Message_pop = 0;
	while(1)
	{
		if(!Message_pop)
		{
			printf("Enter Command (h for help)...>");
			fflush(stdout);
		}
		Message_pop = 1;

		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_ZERO(&read_fd);		
		FD_SET(socket_fd, &read_fd);
		FD_SET(STDIN_FILENO, &read_fd);
		if(select(socket_fd+1, &read_fd, NULL, NULL, &tv) <= 0)
		{
		}
		if(FD_ISSET(STDIN_FILENO, &read_fd))
		{
			scanf("%s", command);
			if(strncmp(command, "h", strlen("h")) == 0)
			{
				Helper();
			}
			else if(strncmp(command, "send", strlen("send")) == 0)
			{
				scanf("%d", &action);
				if(action == 0)
				{
					if(Request_Controller_Alive(send_buf, 1000) == 1)
					{
						sprintf(send_buf_buf, "%s", send_buf);
						printf("[main.c] Send Action 0 to server:\n");				
						printf("%s", send_buf_buf);
					}
				}
				else
				{
					printf("[main.c] Invalid action\n");
				}
				send_fail = send(socket_fd, send_buf_buf, strlen(send_buf_buf)+1, 0);
				if(send_fail < 0)
				{
					printf("[main.c] Send JSON Failed\n");
					break;
				}

			}
			else
			{
				printf("[main.c] Invalid command\n");
			}
			Message_pop = 0;
		}
		if(FD_ISSET(socket_fd, &read_fd))
		{
			if(recv(socket_fd, recv_buf, 1000, 0) > 0)
			{
				puts("");
				printf("[main.c] Received JSON from server:\n%s\n", recv_buf);
				Handle_Action(recv_buf, socket_fd, &Controller_State);
			}
			else
			{
				printf("\n[main.c] Server Disconnect\n");
				break;
			}
			Message_pop = 0;
		}
	}
	close(socket_fd);
}
