/****************************/
/* Http server's parameter  */
/* Author:zhuchao 2015-11-3 */
/****************************/

#ifndef _HTTP_H_
#define _HTTP_H_

typedef enum {GET,PUT,HEAD,POST,NOT_IMPLEMENTED}Method;
typedef enum {HTTP1_0,HTTP1_1,HTTP_UNSUPPORTED}Protocol;

#define CR '\r'
#define LF '\n'
#define CRLF "\r\n"

#endif