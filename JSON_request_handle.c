#include "JSON_request_handle.h"
#include "config.h"
#include "mjson.h"
#include "config.h"
#include "update.h"
#include "utility.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>

int Handle_Action(const char * InputBuf, int socket_fd, int * State)
{
	//get action and distribute action to each function to handle
	double action = mjson_find_number(InputBuf, strlen(InputBuf), "$.Action", -1);
	char logText[500];
	snprintf(logText, sizeof(logText), "[%s] Action is %d", __FILE__, (int)action);
	RecordLog(logText);
	if(action == 0)
	{
		return Handle_Controller_Alive(InputBuf, State);
	}
	else if(action == 1)
	{
		return Handle_Register_AP(InputBuf);
	}
	else if(action == 2)
	{
		return Handle_Return_AP_Info(InputBuf, socket_fd);
	}
	else if(action == 3)
	{
		return Handle_Change_Config(InputBuf, socket_fd);
	}
	else if(action == 6)
	{
		return Handle_Upload_Log(InputBuf, socket_fd);
	}
	else if(action == 7)
	{
		return Handle_Download_Update(InputBuf, socket_fd);
	}
	else
	{
		return -1;
	}
}
//KMP
void ConstructFailFunc(const char * needle, int * Fail, int len)
{
	Fail[0] = 0;
	for(int q = 1,k = 0;q < len;q++)
	{
		while(k > 0 && needle[q] != needle[k])
		{
			k = Fail[k-1];
		}
		// ensure the needle privious one is the same as needle match one is equal
		if(needle[q] == needle[k])
		{
			k++;
		}
		Fail[q] = k;
	}
}
int MatchFunc(const char * Origin, const char * needle)
{
	int Fail[200];
	int len = strlen(Origin);
	int len_n = strlen(needle);
	ConstructFailFunc(needle, Fail, len_n);
	for(int q = 0,k = 0;q < len;q++)
	{
		while(k > 0 && Origin[q] != needle[k])
		{
			k = Fail[k-1];
		}
		// ensure the needle privious one is the same as needle match one is equal
		if(Origin[q] == '#')
		{
			return -1;
		}
		if(Origin[q] == needle[k])
		{
			k++;
		}
		if(k == len_n)
		{
			return q;
		}
	}
	return -1;
}
int Handle_Controller_Alive(const char * InputBuf, int * State)
{
	//Change Controller state to what it indicate
	double buf = mjson_find_number(InputBuf, strlen(InputBuf), "$.State", -1);
	*State = (int)buf;
	return 1;
}
int Handle_Register_AP(const char * InputBuf)
{
	char logText[500];
	char gw_id[200];
	//get controller distribute gateway id and update it to /etc/wifidog.conf
	mjson_find_string(InputBuf, strlen(InputBuf), "$.Gateway_ID", gw_id, sizeof(gw_id));
	FILE * fp_read = fopen("/etc/wifidog.conf", "r");
	FILE * fp_write = fopen("/etc/wifidog.conf_tmp", "a+");
	int success = 0;
	if(fp_read && fp_write)
	{
		char FileBuf[200];
		while(fgets(FileBuf, sizeof(FileBuf), fp_read))
		{
			if(MatchFunc(FileBuf, "GatewayID") != -1)
			{
				//find GatewayID key in /etc/wifidog.conf
				fprintf(fp_write, "GatewayID %s\n", gw_id);
				success = 1;
				continue;
			}
			else
			{
				fprintf(fp_write, "%s", FileBuf);
			}
		}
		if(!success)
		{
			fprintf(fp_write, "GatewayID %s\n", gw_id);
		}
		fclose(fp_read);
		fclose(fp_write);
		remove("/etc/wifidog.conf");
		rename("/etc/wifidog.conf_tmp", "/etc/wifidog.conf");

		//Change AP_Registered state in connect_config to 1 if successfully registered 
		char CurrentPath[200];
		char ConfigPath[200];
		getcwd(CurrentPath, sizeof(CurrentPath));
		snprintf(ConfigPath, sizeof(ConfigPath), "%s/config/connect_config", CurrentPath);

		if(Save_Config_Character(ConfigPath, "AP_Registered", '1') == 0)
		{
			snprintf(logText, sizeof(logText), "[%s] Register AP failed, save registered state to config failed", __FILE__);
			RecordLog(logText);
			return 0;
		}
		snprintf(logText, sizeof(logText), "[%s] Register AP successed", __FILE__);
		RecordLog(logText);

		//Restart wifidog after updated
		system("/etc/init.d/wifidog stop");
		system("/etc/init.d/wifidog start");
		return 1;
	}
	else
	{
		snprintf(logText, sizeof(logText), "[%s] Register AP failed, can't open /etc/wifidog.conf", __FILE__);
		RecordLog(logText);
		return 0;
	}
}
int Handle_Return_AP_Info(const char * InputBuf, int socket_fd)
{
	uint32_t AP_Traffic_Received, AP_Traffic_Transmit, AP_Gateway_ID;
	double AP_Usage_Percent;
	char logText[500];
	char CurrentPath[200];
	char APRecordPath[200];
	char Command[500];

	//Get cpu usage percentage
	system("mkdir APRecord");
	getcwd(CurrentPath, sizeof(CurrentPath));
	snprintf(APRecordPath, sizeof(APRecordPath), "%s/APRecord/cpu_usage_record", CurrentPath);
	snprintf(Command, sizeof(Command), "cat /proc/stat | grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage}' > %s", APRecordPath);
	system(Command);


	FILE * fp = fopen(APRecordPath, "r");
	if(!fp)
	{
		return -1;
	}
	fscanf(fp, "%lf", &AP_Usage_Percent);
	fclose(fp);

	//Get received and transmit traffic from /proc/net/dev
	char buf[500];
	fp = fopen("/proc/net/dev", "r");
	if(!fp)
	{
		return -1;
	}
	while(fscanf(fp, "%s", buf) == 1)
	{
		if(strncmp(buf, "eth0:", sizeof("eth0:")) == 0)
		{
			fscanf(fp, "%u %*u %*u %*u %*u %*u %*u %*u %u", &AP_Traffic_Received, &AP_Traffic_Transmit);
		}
	}
	fclose(fp);
	const unsigned int KiB = 1024;
	snprintf(logText, sizeof(logText), "[%s] AP Received Traffic is %u(KiB)", __FILE__, AP_Traffic_Transmit/KiB);
	RecordLog(logText);
	snprintf(logText, sizeof(logText), "[%s] AP Transmit Traffic is %u(KiB)", __FILE__, AP_Traffic_Received/KiB);
	RecordLog(logText);
	snprintf(logText, sizeof(logText), "[%s] AP Usage Percent is %.2f", __FILE__, AP_Usage_Percent);
	RecordLog(logText);

	char OutputBuf[1000];
        snprintf(OutputBuf, sizeof(OutputBuf), "{\n  \"Action\" : 2,\n  \"AP CPU Usage Percent\" : %.2f,\n  \"Network Received Traffic\" : %u,\n  \"Network Transmit Traffic\" : %u\n}\n", AP_Usage_Percent, AP_Traffic_Received/KiB, AP_Traffic_Transmit/KiB);

	return send(socket_fd, OutputBuf, strlen(OutputBuf)+1, 0);
}
int Handle_Change_Config(const char * InputBuf, int socket_fd)
{
	//Change every config in system according to JSON using uci
	printf("InputBuf: %s\n", InputBuf);

	char logText[500];
	char OutputBuf[4096];
        snprintf(OutputBuf, sizeof(OutputBuf), "{\n	\"Action\" : 3");

	char Query_Config[100];
	char Command[300];
	char Message[1000];
	char ConCat[1000];
	int ConfigAppend;
	const char * Buf;
	int isOneCommandValid = 0;//if any change config command success, it set to 1
	int len;
	int counter = 1;
	int Find_Category, Find_Keyword;
	char forbidden_symbol[] = "\'\"`$|&;";
	snprintf(Query_Config, sizeof(Query_Config), "$.Config_%d", counter);
	while(mjson_find(InputBuf, strlen(InputBuf), Query_Config, &Buf, &len) == MJSON_TOK_OBJECT)
	{
		snprintf(Query_Config, sizeof(Query_Config), "$.Config_%d.Command", counter);
		mjson_find_string(InputBuf, strlen(InputBuf), Query_Config, Command, sizeof(Command));

		int len = strlen(Command);
		//Check JSON command to avoid malicious attack
		char Keyword[200];
		sscanf(Command, "%[^ ]%*[^\n]", Keyword);
		if(strncmp(Keyword, "uci", strlen("uci")) == 0)
		{
			int len = strlen(Command);
			int in_quatation = 0;
			for(int i = 0;i < len;i++)
			{
				if(Command[i] == '\'' || Command[i] == '\"')
				{
					in_quatation = !in_quatation;
				}
				for(int j = 0;j < sizeof(forbidden_symbol);j++)
				{
					if(Command[i] == forbidden_symbol[j] && !in_quatation)
					{
						Command[i] = 0;
						goto outside;
					}
				}
			}
			outside:;
			snprintf(ConCat, sizeof(ConCat), "%s,\n	\"Config_%d\" : \"", OutputBuf, counter);
			strncpy(OutputBuf, ConCat, sizeof(OutputBuf));

			FILE * Command_exec = popen(Command, "r");
			while(fgets(Message, sizeof(Message), Command_exec))
			{
				snprintf(ConCat, sizeof(ConCat), "%s%s", OutputBuf, Message);
				strncpy(OutputBuf, ConCat, sizeof(OutputBuf));
			}
			snprintf(ConCat, sizeof(ConCat), "%s\"", OutputBuf);
			strncpy(OutputBuf, ConCat, sizeof(OutputBuf));

			pclose(Command_exec);
			isOneCommandValid = 1;

			snprintf(logText, sizeof(logText), "[%s] counter is %d, command: %s", __FILE__, counter, Command);
			RecordLog(logText);

		}
		else
		{
			snprintf(logText, sizeof(logText), "[%s] Malicious command: %s", __FILE__, Command);
			RecordLog(logText);
		}
		counter++;
		snprintf(Query_Config, sizeof(Query_Config), "$.Config_%d", counter);
	}
	snprintf(ConCat, sizeof(ConCat), "%s\n}\n", OutputBuf);
	strncpy(OutputBuf, ConCat, sizeof(OutputBuf));
	printf("[OutputBuf]: %s\n", OutputBuf);
	if(isOneCommandValid)
	{
		system("wifi");
	}
	return send(socket_fd, OutputBuf, strlen(OutputBuf)+1, 0);
}
int Handle_Upload_Log(const char * InputBuf, int socket_fd)
{
	//Upload log to FTP server
	char Filename[100];
	char month[10];
	char logText[500];
	char CurrentPath[200];
	char ConfigPath[200];
	char OutputBuf[1000];
	getcwd(CurrentPath, sizeof(CurrentPath));
	snprintf(ConfigPath, sizeof(ConfigPath), "%s/config/connect_config", CurrentPath);

	int year, day, hour, minute, sec;
	//Get Current Time in below format
	//Sun Nov 25 01:27:25 2018
	time_t cur_time = time(NULL);
	sscanf(ctime(&cur_time), "%*s %s %d %d:%d:%d %d", month, &day, &hour, &minute, &sec, &year);
	
	snprintf(Filename, sizeof(Filename), "Record.log");

	char FTPFile[500];
	char FTPServerAddress[100];
	char UserName[50];
	char Password[50];
	ConfigQuery ConfigBuf;
	ConfigBuf = Load_Config_String(ConfigPath, "FTP_Server_Address", FTPServerAddress, sizeof(FTPServerAddress));
	snprintf(logText, sizeof(logText), "[%s] Upload Filename: %s", __FILE__, Filename);
	RecordLog(logText);

	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}
	//Upload file name is RecordLog_gatewayid_year_month_day_hour_minute_second.log
	snprintf(FTPFile, sizeof(FTPFile), "ftp://%s/RecordLog_%d_%d_%s_%d_%d_%d_%d.log", FTPServerAddress, GetGatewayID(), year, month, day, hour, minute, sec);

	ConfigBuf = Load_Config_String(ConfigPath, "FTP_Username", UserName, sizeof(UserName));
	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}

	ConfigBuf = Load_Config_String(ConfigPath, "FTP_Password", Password, sizeof(Password));
	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}

	UploadFile(FTPFile, UserName, Password, Filename);
        int ret_value = snprintf(OutputBuf, sizeof(OutputBuf), "{\n  \"Action\" : 6,\n  \"Log File Name\" : \"RecordLog_%d_%d_%s_%d_%d_%d_%d.log\"\n}\n", GetGatewayID(), year, month, day, hour, minute, sec);
	return send(socket_fd, OutputBuf, strlen(OutputBuf)+1, 0);
}

int Handle_Download_Update(const char * InputBuf, int socket_fd)
{
	//Controller indicate ap to update
	//Download new ap client from FTP server
	char FileName[50];
	mjson_find_string(InputBuf, strlen(InputBuf), "$.Update Archive File Name", FileName, sizeof(FileName));

	char FTPServerAddress[100];
	char UserName[50];
	char Password[50];
	char FTPFile[500];
	char Update_Extract_Loc[100];
	char logText[500];
	char CurrentPath[200];
	char ConfigPath[200];
	getcwd(CurrentPath, sizeof(CurrentPath));
	snprintf(ConfigPath, sizeof(ConfigPath), "%s/config/connect_config", CurrentPath);
	ConfigQuery ConfigBuf;
	ConfigBuf = Load_Config_String(ConfigPath, "FTP_Server_Address", FTPServerAddress, sizeof(FTPServerAddress));

	snprintf(logText, sizeof(logText), "[%s] Downloaded Filename: %s", __FILE__, FileName);
	RecordLog(logText);
	//Get Every needed information from connect_config
	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}
	snprintf(FTPFile, sizeof(FTPFile), "ftp://%s/%s", FTPServerAddress, FileName);

	ConfigBuf = Load_Config_String(ConfigPath, "FTP_Username", UserName, sizeof(UserName));
	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}

	ConfigBuf = Load_Config_String(ConfigPath, "FTP_Password", Password, sizeof(Password));
	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}

	DownloadFile(FTPFile, UserName, Password, FileName);

	ConfigBuf = Load_Config_String(ConfigPath, "Update_Extract_Location", Update_Extract_Loc, sizeof(Update_Extract_Loc));
	if(ConfigBuf.DataType != 2)
	{
		snprintf(logText, sizeof(logText), "[%s] No proper config", __FILE__);
		RecordLog(logText);
		return -1;
	}

	char Command_To_delete_previous[200];//to avoid busy file
	snprintf(Command_To_delete_previous, sizeof(Command_To_delete_previous), "tar -ztvf %s | awk '{print $6}' | xargs rm -rf", FileName);

	snprintf(logText, sizeof(logText), "[%s] Command: %s", __FILE__, Command_To_delete_previous);
	RecordLog(logText);

	char Command_To_Extract[200];
	snprintf(Command_To_Extract, sizeof(Command_To_Extract), "tar -xvzf %s -C %s", FileName, Update_Extract_Loc);

	snprintf(logText, sizeof(logText), "[%s] Command: %s", __FILE__, Command_To_Extract);
	RecordLog(logText);

	system(Command_To_delete_previous);
	system(Command_To_Extract);

	remove(FileName);

	int Applied = 1;

	char OutputBuf[1000];
        snprintf(OutputBuf, sizeof(OutputBuf), "{\n  \"Action\" : 7,\n  \"Applied\" : %d\n}\n", Applied);
	send(socket_fd, OutputBuf, strlen(OutputBuf)+1, 0);
	execv("/root/ap_client", NULL);
	return 1;
}
