// Implementation of the Socket class.


#include "Socket.h"
#include "SocketException.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>

#ifndef SO_NOSIGPIPE
#define SO_NOSIGPIPE MSG_NOSIGNAL
#endif


Socket::Socket() :
m_sock ( -1 )
{

	memset ( &m_addr,
		0,
		sizeof ( m_addr ) );

}

Socket::~Socket()
{
	if ( is_valid() )
		::close ( m_sock );
}

bool Socket::create()
{
	m_sock = socket ( AF_INET,
		SOCK_STREAM,
		0 );

	if ( ! is_valid() )
		return false;


// TIME_WAIT - argh
	int on = 1;
	if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
		return false;


	return true;

}



bool Socket::bind ( const int port )
{

	if ( ! is_valid() )
	{
		return false;
	}



	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = htons ( port );

	int bind_return = ::bind ( m_sock,
		( struct sockaddr * ) &m_addr,
		sizeof ( m_addr ) );


	if ( bind_return == -1 )
	{
		return false;
	}

	return true;
}


bool Socket::listen() const
{
	if ( ! is_valid() )
	{
		return false;
	}

	int listen_return = ::listen ( m_sock, MAXCONNECTIONS );


	if ( listen_return == -1 )
	{
		return false;
	}

	return true;
}


bool Socket::accept ( Socket& new_socket ) const
{
	int addr_length = sizeof ( m_addr );
	new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

	if ( new_socket.m_sock <= 0 )
		return false;
	else
		return true;
}


bool Socket::send ( const std::string s ) const
{
	int status = ::send ( m_sock, s.c_str(), s.size(), SO_NOSIGPIPE );
	if ( status == -1 )
	{
		return false;
	}
	else
	{
		return true;
	}
}


int Socket::recv ( std::string& s ) const
{
	char buf [ MAXRECV + 1 ];

	s = "";

	memset ( buf, 0, MAXRECV + 1 );

	int status = ::recv ( m_sock, buf, MAXRECV, 0 );

	if ( status == -1 )
	{
		std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
		return 0;
	}
	else if ( status == 0 )
	{
		return 0;
	}
	else
	{
		s = buf;
		return status;
	}
}

int Socket::send (char * buffer, int size) const{
	int status = ::send ( m_sock, buffer, size, SO_NOSIGPIPE );
	return status;
}


int Socket::recv(char * buffer, int size) const {
	memset ( buffer, 0, size );
	int status = ::recv ( m_sock, buffer, size - 1, 0 );
	if ( status == -1 and errno != EAGAIN){
		//std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
		throw SocketException ("Error while receiving");
	}
	return status;
}

//Reads from the socket until it finds a \n and returns
//a string with everything read, including the \n.
std::string Socket::readLine() const {
	char buf[MAXRECV + 1];
	std::string answer = "";
	
	while (true){
		//Peek at the socket, to see if we have a nearby end of line.
		memset(buf, 0, MAXRECV + 1);
		int length = ::recv(m_sock, buf, MAXRECV, MSG_PEEK);
		if (length <= 0){
	      throw SocketException ( "Could not read from socket." );
		}
		char * end = (char *) memchr(buf, '\n', length); //Try to find a \n
		if (end == NULL){
			//No nearby end of line. Let's add everything read to the answer and try again.
			memset(buf, 0, MAXRECV + 1);
			length = ::recv(m_sock, buf, MAXRECV, 0);
			if (length <= 0){
		      throw SocketException ( "Could not read from socket." );
			}			
			answer += buf;
			continue;
		}else{
			//There's a nearby end of line. Read just until there.
			int must_read = end - buf + 1; //The number of bytes that must be read to include the \n
			memset(buf, 0, MAXRECV + 1);
			length = ::recv(m_sock, buf, must_read, 0);
			if (length <= 0){
		      throw SocketException ( "Could not read from socket." );
			}			
			answer += buf;
			break;
		}
	}
	return answer;
}


bool Socket::connect ( const std::string host, const int port )
{
	if ( ! is_valid() ) return false;

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons ( port );

	int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

	if ( errno == EAFNOSUPPORT ) return false;

	status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

	if ( status == 0 )
		return true;
	else
		return false;
}

void Socket::set_non_blocking ( const bool b )
{

	int opts;

	opts = fcntl ( m_sock,
		F_GETFL );

	if ( opts < 0 )
	{
		return;
	}

	if ( b )
		opts = ( opts | O_NONBLOCK );
	else
		opts = ( opts & ~O_NONBLOCK );

	fcntl ( m_sock,
		F_SETFL,opts );

}
