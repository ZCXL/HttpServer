#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <cstring>

#include "HttpRequest.h"
HttpRequest::HttpRequest():requestBody(""),data("")
{
	
}
HttpRequest::~HttpRequest()
{
	
}
void HttpRequest::printRequest(void)
{
	cout<<"---Request Begin---"<<endl<<data<<"---Request End---"<<endl;
}
void HttpRequest::addData(const char* buf,const int& len)
{
	data.append(buf,len);
}
void HttpRequest::addRequestBody(const string&str)
{
	requestBody+=str;
}

int HttpRequest::setMethod(Method m)
{
	method=m;
	return 0;
}
Method HttpRequest::getMethod()
{
	return method;
}

int HttpRequest::setURL(string u)
{
	url=u;
	return 0;
}
string HttpRequest::getURL()
{
	return url;
}

int HttpRequest::setProtocol(Protocol p)
{
	protocol=p;
	return 0;
}
Protocol HttpRequest::getProtocol()
{
	return protocol;
}

int HttpRequest::setUserAgent(string u)
{
	userAgent=u;
	return 0;
}
string HttpRequest::getUserAgent()
{
	return userAgent;
}

int HttpRequest::setHttpHeader(string headerName,string headerContent)
{
	http_headers.push_back(make_pair(headerName,headerContent));
	return 0;
}
string HttpRequest::getHttpHeader(string headerName)
{
	for(vector<pair<string,string> >::iterator it=http_headers.begin();it!=http_headers.end();it++)
	{
		if((*it).first==headerName)
			return (*it).second;
	}
	return "";
}

int HttpRequest::setHttpHeaderVector(vector<pair<string,string> >* const h)
{
	for(vector<pair<string,string> >::iterator it=h->begin();it!=h->end();it++)
		setHttpHeader((*it).first,(*it).second);
	return 0;
}
vector<pair<string,string> >* HttpRequest::getHttpHeaderVector()
{
	return &http_headers;
}

int HttpRequest::setRequestBody(const string* r)
{
	requestBody=*r;
	return 0;
}
string* HttpRequest::getRequestBodyPtr()
{
	return &requestBody;
}

int HttpRequest::parseRequest()
{
	/*
	   Request = Request-Line CRLF
	   (Request-Header CRLF)*
	   CRLF
	   Message-Body
	   Request-Line = Method-Name <space> Request-URI <space> HTTP/1.0 CRLF
	   Request-Header = Header-Name ":" <space> Header-Content CRLF
	 */
	size_t parseCursorOld = 0, parseCursorNew = 0;
	size_t headerParseCursorOld, headerParseCursorNew;
	string httpMethod, httpProtocol, requestHeader;
	string requestHeaderName, requestHeaderContent;
	
	/* Parse Request-Line */
	/* HTTP Method */
	parseCursorNew = data.find_first_of(" ", parseCursorOld);
	httpMethod = data.substr(parseCursorOld, parseCursorNew - parseCursorOld);
	parseCursorOld = parseCursorNew+1;
	
	if(httpMethod == "GET"){
		method = GET;
	}else if(httpMethod == "PUT"){
		method = PUT;
	}else{
		method = NOT_IMPLEMENTED;
		return 0;
	}

	/* URL */
	parseCursorNew = data.find_first_of(" ", parseCursorOld);
	url = data.substr(parseCursorOld, parseCursorNew - parseCursorOld);
	parseCursorOld = parseCursorNew+1;

	/* HTTP Protocol */
	parseCursorNew = data.find_first_of(CRLF, parseCursorOld);
	httpProtocol = data.substr(parseCursorOld, parseCursorNew - parseCursorOld);
	parseCursorOld = parseCursorNew+1;

	if(httpProtocol == "HTTP/1.0"){
		protocol = HTTP1_0;
	}else if(httpProtocol == "HTTP/1.1"){
		protocol = HTTP1_1;
	}else{
		protocol = HTTP_UNSUPPORTED;
		return 0;
	}

	/* Skip the CRLF */
	parseCursorOld++;

	/* Request Headers start here */
	while(1){
		parseCursorNew = data.find_first_of(CRLF, parseCursorOld);
		requestHeader = data.substr(parseCursorOld, parseCursorNew - parseCursorOld);
		parseCursorOld = parseCursorNew+1;

		headerParseCursorOld = headerParseCursorNew = 0;
		/* Further parse the request header */
		/* Header Name */
		headerParseCursorNew = requestHeader.find_first_of(":", headerParseCursorOld);
		requestHeaderName = requestHeader.substr(headerParseCursorOld, headerParseCursorNew - headerParseCursorOld);
		headerParseCursorOld = headerParseCursorNew+2;
	
		/* Header Content */
		headerParseCursorNew = requestHeader.find_first_of(CRLF, headerParseCursorOld);
		requestHeaderContent = requestHeader.substr(headerParseCursorOld, headerParseCursorNew - headerParseCursorOld);
		headerParseCursorOld = headerParseCursorNew;

		setHttpHeader(requestHeaderName, requestHeaderContent);
	
		/* Skip the CRLF */
		parseCursorOld++;
	
		/* Is there another CRLF? */
		if(data.substr(parseCursorOld, 2) == CRLF)
			break;
	}

	parseCursorOld+=2;
	requestBody = data.substr(parseCursorOld);
	
	return 0;
}

int HttpRequest::prepareRequest(){
	string httpMethod,httpProtocol;
	
	switch(method){
		case GET:
			httpMethod="GET";
			break;
		case PUT:
			httpMethod="PUT";
			break;
		default:
			return -1;
			break;
	}
	switch(protocol){
		case HTTP1_0:
			httpProtocol="HTTP/1.0";
			break;
		case HTTP1_1:
			httpProtocol="HTTP/1.1";
			break;
		default:
			return -1;
			break;
	}
	data+=httpMethod+" "+url+" "+httpProtocol+CRLF;
	for(vector<pair<string, string> >::iterator  it = http_headers.begin(); it!=http_headers.end(); it++){
		data += (*it).first + ": " + (*it).second + CRLF;
	}
	data += CRLF;

	data += requestBody;
	return 0;
}

size_t HttpRequest::getRequestSize()
{
	return data.length();
}
string* HttpRequest::getRequestDataPtr()
{
	return &data;
}

int HttpRequest::copyToFile(ofstream& ofs){
	size_t contentLength=atoi(getHttpHeader("Content-Length").c_str());
	
	if(ofs.good()){
		ofs.write(requestBody.c_str(),contentLength);
	}
	if(ofs.bad())
		return -1;
	return 0;
}
int HttpRequest::copyFromFile(ifstream& ifs, size_t contentLength)
{
	char* fileBuf = new char[contentLength];
	memset(fileBuf, '\0', contentLength);

	if(ifs.good()){
		ifs.read(fileBuf, contentLength);
	}
	requestBody.append(fileBuf, contentLength);

	if(ifs.bad())
		return -1;

	return 0;
}