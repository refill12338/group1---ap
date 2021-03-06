#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "JSON_action.h"
#include "config.h"
#include "update.h"
#include "CheckFile.h"
#include "utility.h"

int Request_Controller_Alive(char * OutputBuf, int length)
{
	int ret_value = snprintf(OutputBuf, length, "{\n  \"Action\" : 0\n}\n");
	return ret_value>=0&&ret_value<length;
}
int Request_Register_AP(char * OutputBuf, int length)
{
	int ret_value = snprintf(OutputBuf, length, "{\n  \"Action\" : 1\n}\n");
	return ret_value>=0&&ret_value<length;
}
int Request_Connect_To_Controller(char * OutputBuf, int length)
{
	int ret_value = snprintf(OutputBuf, length, "{\n  \"Action\" : 4,\n	\"Gateway_ID\" : %d\n}\n", GetGatewayID());
	return ret_value>=0&&ret_value<length;
}
