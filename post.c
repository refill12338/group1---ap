#include "post.h"
void WrapToPOST(char * dest, const char * src)
{
	sprintf(dest, "POST %s HTTP1.1\r\nAccept: */*\r\nReferer: <REFERER HERE>\r\nAccept-Language: en-us\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept-Encoding: gzip,deflate\r\nUser-Agent: Mozilla/4.0\r\nContent-Length: %d\r\nPragma: no-cache\r\nConnection: keep-alive\r\n\r\n%s", 
}
