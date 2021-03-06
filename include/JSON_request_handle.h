#ifndef JSON_REQUEST_HANDLE_H
	#define JSON_REQUEST_HANDLE_H
	#define _POSIX_C_SOURCE 2

	int Handle_Action(const char * InputBuf, int socket_fd, int * State);
	int Handle_Controller_Alive(const char * InputBuf, int * State);
	int Handle_Register_AP(const char * InputBuf);
	int Handle_Return_AP_Info(const char * InputBuf, int socket_fd);
	int Handle_Change_Config(const char * InputBuf, int socket_fd);
	int Handle_Send_User_Info(const char * InputBuf, int socket_fd);
	int Handle_Upload_Log(const char * InputBuf, int socket_fd);
	int Handle_Download_Update(const char * InputBuf, int socket_fd);
#endif
