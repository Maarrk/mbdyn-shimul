/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2004
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Michele Attolico <attolico@aero.polimi.it>
*/


#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include <netdb.h>

#include <dataman.h>
#include <filedrv.h>
#include <streamdrive.h>
#include "sock.h"

#include <socketstreamdrive.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#define UNIX_PATH_MAX    108
#define DEFAULT_PORT	5500 /* FIXME:da definire meglio */
#define DEFAULT_HOST 	"127.0.0.1"

SocketStreamDrive::SocketStreamDrive(unsigned int uL,
		const DriveHandler* pDH,
		const char* const sFileName,
		integer nd, bool c,
		unsigned short int p,
		const char* const h)
: StreamDrive(uL, pDH, sFileName, nd, c),
type(AF_INET), sock(0), connection_flag(false)
{
	if (h) {
		SAFESTRDUP(host, h);
	} else {
		host = NULL;
	}
	
   	ASSERT(p > 0);
	data.Port = p;
	
	if (create){
		struct sockaddr_in	addr_name;
		int			save_errno;

   		sock = make_inet_socket(&addr_name, host, data.Port, 1, 
				&save_errno);
		
		if (sock == -1) {
			const char	*err_msg = strerror(save_errno);

      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): socket() failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
      			throw ErrGeneric();

   		} else if (sock == -2) {
			const char	*err_msg = strerror(save_errno);

      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): bind() failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
      			throw ErrGeneric();

   		} else if (sock == -3) {
      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): illegal host name \"" << host << "\" "
				"(" << save_errno << ")"
				<< std::endl);
      			throw ErrGeneric();
   		}
		
   		if (listen(sock, 1) < 0) {
			save_errno = errno;
			const char	*err_msg = strerror(save_errno);

      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): listen() failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
      			throw ErrGeneric();
   		}
	}
	 
}

SocketStreamDrive::SocketStreamDrive(unsigned int uL,
		const DriveHandler* pDH,
		const char* const sFileName,
		integer nd, bool c,
		const char* const Path)
: StreamDrive(uL, pDH, sFileName, nd, c),
host(NULL), type(AF_LOCAL), sock(0), connection_flag(false)
{
	//NO_OP;
	
	ASSERT(Path != NULL);

	SAFESTRDUP(data.Path, Path);
	
	if (create){
		int			save_errno;

		sock = make_named_socket(data.Path, 1, &save_errno);
		
		if (sock == -1) {
			const char	*err_msg = strerror(save_errno);

      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): socket() failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
      			throw ErrGeneric();

   		} else if (sock == -2) {
			const char	*err_msg = strerror(save_errno);

      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): bind() failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
      			throw ErrGeneric();
   		}
		
   		if (listen(sock, 1) < 0) {
			save_errno = errno;
			const char	*err_msg = strerror(save_errno);

      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): listen() failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
      			throw ErrGeneric();
   		}
	}

	 
}

SocketStreamDrive::~SocketStreamDrive(void)
{
	shutdown(sock,SHUT_RD);
	switch (type) { //connect
	case AF_LOCAL:
		if (data.Path) {
			SAFEDELETEARR(data.Path);
		}
		break;
	default:
		NO_OP;
		break;
	}
}

FileDrive::Type
SocketStreamDrive::GetFileDriveType(void) const
{
	return FileDrive::SOCKETSTREAM;
}

/* Scrive il contributo del DriveCaller al file di restart */   
std::ostream&
SocketStreamDrive::Restart(std::ostream& out) const
{
   	return out << "0. /* SocketStreamDrive not implemented yet */"
		<< std::endl;
}
   
void
SocketStreamDrive::ServePending(const doublereal& t)
{
	
	if (connection_flag) {//non esegue la lettura nel primo passo
		if ((recv(sock, buf, size, 0))) {
			
			doublereal *rbuf = (doublereal *)buf;
			for (int i = 1; i <= iNumDrives; i++){
				pdVal[i] = rbuf[i-1];
			}
	
		} else {	
			silent_cout("SocketStreamDrive(" << sFileName << ") "
					<< "Communication closed by host" << std::endl);
			// stop simulation?
		}
	}
	
	
	if (!connection_flag) {
		if (create) {
			int tmp_sock = sock;
#ifdef HAVE_SOCKLEN_T
	   		socklen_t socklen;
#else /* !HAVE_SOCKLEN_T */
			int socklen;
#endif /* !HAVE_SOCKLEN_T */

			switch (type) {
			case AF_LOCAL:
				{
					struct sockaddr_un client_addr;
					socklen = sizeof(struct sockaddr_un);

					pedantic_cout("accepting connection on local socket \""
							<< sFileName << "\" (" << data.Path << ") ..." 
							<< std::endl);

					socklen = sizeof(struct sockaddr_un);
					client_addr.sun_path[0] = '\0';
					sock = accept(tmp_sock,
							(struct sockaddr *)&client_addr, &socklen);
					if (sock == -1) {
						int		save_errno = errno;
						const char	*err_msg = strerror(save_errno);

                				silent_cerr("SocketStreamElem(" << sFileName << ") "
							"accept(" << tmp_sock << ") failed ("
							<< save_errno << ": " << err_msg << ")"
							<< std::endl);
						throw ErrGeneric();
     					}
					if (client_addr.sun_path[0] == '\0') {
						strncpy(client_addr.sun_path, data.Path, sizeof(client_addr.sun_path));
					}
					silent_cout("SocketStreamDrive(" << GetLabel()
  							<< "): connect from " << client_addr.sun_path
							<< std::endl);
					unlink(data.Path);
					
					break;
				}

			case AF_INET:
				{
   					struct sockaddr_in client_addr;
					socklen = sizeof(struct sockaddr_in);
	
					pedantic_cout("accepting connection on inet socket \""
							<< sFileName << "\" (" 
							<< inet_ntoa(client_addr.sin_addr) 
							<< ":" << ntohs(client_addr.sin_port)
							<< ") ..." << std::endl);
						
					socklen = sizeof(struct sockaddr_in);
					sock = accept(tmp_sock,
							(struct sockaddr *)&client_addr, &socklen);
					if (sock == -1) {
						int		save_errno = errno;
						const char	*err_msg = strerror(save_errno);

                				silent_cerr("SocketStreamElem(" << sFileName << ") "
							"accept(" << tmp_sock << ") failed ("
							<< save_errno << ": " << err_msg << ")"
							<< std::endl);
						throw ErrGeneric();
        				}
      					silent_cout("SocketStreamDrive(" << GetLabel()
	  					<< "): connect from " << inet_ntoa(client_addr.sin_addr)
		  				<< ":" << ntohs(client_addr.sin_port) << std::endl);
					shutdown(tmp_sock,SHUT_RDWR);
					break;
				}

			default:
				NO_OP;
				break;
			}
		} else {
				//connect
			switch (type) {
			case AF_LOCAL:
				{
					struct sockaddr_un addr;
						
					sock = socket(PF_LOCAL, SOCK_STREAM, 0);
					if (sock < 0){
						silent_cerr("SocketStreamDrive(" << sFileName << ") "
							"socket() failed " << host << std::endl);
						throw ErrGeneric();
					}
					addr.sun_family = AF_UNIX;
					memcpy(addr.sun_path, data.Path, UNIX_PATH_MAX);

					pedantic_cout("connecting to local socket \""
							<< sFileName << "\" (" << data.Path << ") ..." 
							<< std::endl);

					if (connect(sock,(struct sockaddr *) &addr, sizeof (addr)) < 0){
						silent_cerr("SocketStreamDrive(" << sFileName << ") "
							"connect() failed " << std::endl);
						throw ErrGeneric();									
					}					
					break;
				}
			
			case AF_INET:
					{
					struct sockaddr_in addr;
					
					sock = socket(PF_INET, SOCK_STREAM, 0);
					if (sock < 0){
						silent_cerr("SocketStreamDrive(" << sFileName << ") "
							"socket() failed " << host << std::endl);
						throw ErrGeneric();
						}				
					addr.sin_family = AF_INET;
					addr.sin_port = htons (data.Port);
					if (inet_aton(host, &(addr.sin_addr)) == 0) {
						silent_cerr("SocketStreamDrive(" << sFileName << ") "
							"unknow host \"" << host << "\"" << std::endl);
						throw ErrGeneric();	
					}

						pedantic_cout("connecting to inet socket \""
							<< sFileName << "\" (" 
							<< inet_ntoa(addr.sin_addr) 
							<< ":" << ntohs(addr.sin_port)
							<< ") ..." << std::endl);
							
					if (connect(sock,(struct sockaddr *) &addr, sizeof (addr)) < 0){
						silent_cerr("SocketStreamDrive(" << sFileName << ") "
							"connect() failed " << std::endl);
						throw ErrGeneric();					
						}
					break;
				}
				
			default:
				NO_OP;
				break;
			}
	
		} /*create*/
		
		struct linger lin;
		lin.l_onoff = 1;
		lin.l_linger = 0;
		
		if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin))){
      			silent_cerr("SocketStreamDrive(" << sFileName
				<< "): setsockopt() failed" << std::endl);
      			throw ErrGeneric();
			
		}
		connection_flag = true;
	} /*connection_flag == false*/
}


/* legge i drivers tipo stream */

Drive*
ReadSocketStreamDrive(DataManager* pDM,
		MBDynParser& HP,
		unsigned int uLabel)
{
	
	bool create = false;
	unsigned short int port = DEFAULT_PORT;
	const char *name = NULL;
	const char *host = NULL;
	const char *path = NULL;


	if (HP.IsKeyWord("name") || HP.IsKeyWord("stream" "drive" "name")) {
		const char *m = HP.GetStringWithDelims();
		if (m == NULL) {
			silent_cerr("unable to read stream drive name "
				"for SocketStreamDrive(" << uLabel 
				<< ") at line "
				<< HP.GetLineData() << std::endl);
			throw ErrGeneric();

		} 

		SAFESTRDUP(name, m);

	} else {
		silent_cerr("missing stream drive name "
			"for SocketStreamDrive(" << uLabel
			<< ") at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric();
	}

	if (HP.IsKeyWord("create")) {
		if (HP.IsKeyWord("yes")) {
			create = true;
		} else if (HP.IsKeyWord("no")) {
			create = false;
		} else {
			silent_cerr("\"create\" must be \"yes\" or \"no\" "
				"for stream drive \"" << name << "\" "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric();
		}
	}
	
	if (HP.IsKeyWord("local") || HP.IsKeyWord("path")) {
		const char *m = HP.GetStringWithDelims();
		
		if (m == NULL) {
			silent_cerr("unable to read local path for "
				<< "SocketStreamDrive("
			 	<< uLabel << ") at line "
				<< HP.GetLineData() << std::endl);
			throw ErrGeneric();
		}
		
		SAFESTRDUP(path, m);	
	}
	
	if (HP.IsKeyWord("port")){
		if (path != NULL){
			silent_cerr("cannot specify a port "
					"for a local socket in "
				<< "SocketStreamDrive("
			 	<< uLabel << ") at line "
				<< HP.GetLineData() << std::endl);
			throw ErrGeneric();		
		}
		int p = HP.GetInt();
		/*Da sistemare da qui*/
		
		if (p <= IPPORT_USERRESERVED) {
			silent_cerr("SocketStreamDrive(" << uLabel << "): "
					"cannot listen on port " << port
					<< ": less than IPPORT_USERRESERVED=" 
					<< IPPORT_USERRESERVED
					<< " at line " << HP.GetLineData()
					<< std::endl);
			throw ErrGeneric();
		}
		port = p;
	}

	
	if (HP.IsKeyWord("host")) {
		if (path != NULL){
			silent_cerr("cannot specify an allowed host "
					"for a local socket in "
				<< "SocketStreamDrive("
			 	<< uLabel << ") at line "
				<< HP.GetLineData() << std::endl);
			throw ErrGeneric();		
		}

		const char *h;
		
		h = HP.GetStringWithDelims();
		if (h == NULL) {
			silent_cerr("unable to read host for "
				<< "SocketStreamDrive("
			 	<< uLabel << ") at line "
				<< HP.GetLineData() << std::endl);
			throw ErrGeneric();
		}

		SAFESTRDUP(host, h);

	} else if (!path && !create) {
		silent_cerr("host undefined for "
				<< "SocketStreamDrive("
			 	<< uLabel << ") at line "
			<< HP.GetLineData() << std::endl);
		silent_cerr("using default host: "
			<< DEFAULT_HOST << std::endl);
		SAFESTRDUP(host, DEFAULT_HOST);
	}

	int idrives = HP.GetInt();
	if (idrives <= 0) {
		silent_cerr("illegal number of channels for "
				<< "SocketStreamDrive("
			 	<< uLabel << ") at line "
			<< std::endl);
		throw ErrGeneric();
	}

	Drive* pDr = NULL;
	if (path == NULL){
		SAFENEWWITHCONSTRUCTOR(pDr, SocketStreamDrive,
				SocketStreamDrive(uLabel, 
				pDM->pGetDrvHdl(),
				name, idrives, create, port, host));

	} else {
		SAFENEWWITHCONSTRUCTOR(pDr, SocketStreamDrive,
				SocketStreamDrive(uLabel, 
				pDM->pGetDrvHdl(),
				name, idrives, create, path));	
	}

	return pDr;
} /* End of ReadStreamDrive */

