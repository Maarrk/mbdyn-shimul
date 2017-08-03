/* $Header$ */
/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2017
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

#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */

#include <cstring>

#ifdef USE_SOCKET
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
#endif // USE_SOCKET

#include "dataman.h"
#include "socketstream_out_elem.h"
#include "socketstreammotionelem.h"
#include "sock.h"

#ifdef USE_RTAI
#include "rtai_out_elem.h"
#endif // USE_RTAI

#ifdef USE_SOCKET

#include "geomdata.h"

/* SocketStreamElem - begin */

SocketStreamElem::SocketStreamElem(unsigned int uL,
	const std::string& name,
	unsigned int oe,
	UseSocket *pUS,
	StreamContent *pSC,
	int flags, bool bSendFirst, bool bAbortIfBroken,
	StreamOutEcho *pSOE)
: Elem(uL, flag(0)),
StreamOutElem(uL, name, oe),
pUS(pUS), pSC(pSC), send_flags(flags),
bSendFirst(bSendFirst), bAbortIfBroken(bAbortIfBroken),
pSOE(pSOE)
{
	if (pSOE) {
		pSOE->Init("SocketStreamElem", uLabel, pSC->GetNumChannels());
	}
}

SocketStreamElem::~SocketStreamElem(void)
{
	if (pUS != 0) {
		SAFEDELETE(pUS);
	}

	if (pSC != 0) {
		SAFEDELETE(pSC);
	}

	if (pSOE != 0) {
		delete pSOE;
	}
}

std::ostream&
SocketStreamElem::Restart(std::ostream& out) const
{   	
	return out << "# SocketStreamElem(" << GetLabel() << "): "
		"not implemented yet" << std::endl;
}	

void
SocketStreamElem::SetValue(DataManager *pDM,
		VectorHandler& X, VectorHandler& XP,
		SimulationEntity::Hints *ph)
{
	if (bSendFirst) {
		// output imposed values (before "derivatives")
		OutputCounter = OutputEvery - 1;

		AfterConvergence(X, XP);
	}

	// do not send "derivatives"
	OutputCounter = -1;
}

void
SocketStreamElem::AfterConvergence(const VectorHandler& X, 
		const VectorHandler& XP)
{
	/* by now, an abandoned element does not write any more;
	 * should we retry or what? */
	if (pUS->Abandoned()) {
		return;
	}

	ASSERT(pUS->Connected());

	/* output only every OutputEvery steps */
	OutputCounter++;
	if (OutputCounter != OutputEvery) {
		return;
	}
	OutputCounter = 0;

	// prepare the output buffer
	pSC->Prepare();

	// check whether echo is needed
	if (pSOE) {
		pSOE->Echo((doublereal *)pSC->GetBuf(), pSC->GetNumChannels());
	}

	// int rc = send(pUS->GetSock(), pSC->GetOutBuf(), pSC->GetOutSize(), send_flags);
	ssize_t rc = pUS->send(pSC->GetOutBuf(), pSC->GetOutSize(), send_flags);
	if (rc == -1 || rc != pSC->GetOutSize()) {
		int save_errno = errno;

		if (save_errno == EAGAIN && (send_flags & MSG_DONTWAIT)) {
			// would block; continue (and discard...)
			return;
		}
		
		char *msg = strerror(save_errno);
		silent_cerr("SocketStreamElem(" << name << "): send() failed "
				"(" << save_errno << ": " << msg << ")"
				<< std::endl);

		if (bAbortIfBroken) {
			throw NoErr(MBDYN_EXCEPT_ARGS);
		}

		pUS->Abandon();
	}
}

void
SocketStreamElem::AfterConvergence(const VectorHandler& X, 
		const VectorHandler& XP, const VectorHandler& XPP)
{
	AfterConvergence(X, XP);
}

#endif // USE_SOCKET

Elem *
ReadSocketStreamElem(DataManager *pDM, MBDynParser& HP, unsigned int uLabel, StreamContent::Type type)
{
	bool bIsRTAI(false);
#ifdef USE_RTAI
	if (::rtmbdyn_rtai_task != 0) {
		bIsRTAI = true;
	}
#endif // USE_RTAI
#ifndef USE_SOCKET
	if (!bIsRTAI) {
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"not allowed because apparently the current "
			"architecture does not support sockets "
			"at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}
#endif // ! USE_SOCKET

	std::string name;
	std::string path;
	std::string host;
	unsigned short int port = (unsigned short int)(-1);
	bool bGotCreate(false);
	bool bCreate(false);

	if (HP.IsKeyWord("name") || HP.IsKeyWord("stream" "name")) {
		const char *m = HP.GetStringWithDelims();
		if (m == 0) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"unable to read stream name "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);

		} else if (bIsRTAI && strlen(m) != 6) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"illegal stream name \"" << m << "\" "
				"(must be exactly 6 chars) "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		} 
		
		name = m;

	} else {
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"missing stream name "
			"at line " << HP.GetLineData() << std::endl);
		if (bIsRTAI) {
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}

	if (HP.IsKeyWord("create")) {
		bGotCreate = true;
		if (!HP.GetYesNo(bCreate)) {
			silent_cerr("SocketStreamElem(" << uLabel << "):"
				"\"create\" must be either "
				"\"yes\" or \"no\" "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}
	
	if (HP.IsKeyWord("local") || HP.IsKeyWord("path")) {
		const char *m = HP.GetFileName();
		
		if (m == 0) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"unable to read local path "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		
		path = m;
	}

	if (HP.IsKeyWord("port")) {
		if (!path.empty()) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"cannot specify a port for a local socket "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		int p = HP.GetInt();

		if (p <= 0) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"illegal port " << p << " "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		port = p;
	}

	if (HP.IsKeyWord("host")) {
		if (!path.empty()) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"cannot specify an allowed host "
				"for a local socket "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		const char *h;
		
		h = HP.GetStringWithDelims();
		if (h == 0) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"unable to read host "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

		host = h;

	} else if (path.empty() && !bCreate) {
		/* INET sockets (!path) must be created if host is missing */
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"host undefined "
			"at line " << HP.GetLineData() << std::endl);
		host = DEFAULT_HOST;
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"using default host: "
			<< host << ":"
			<< (port == (unsigned short int)(-1) ? DEFAULT_PORT : port)
			<< std::endl);
	}

#ifdef USE_SOCKET
	const int sock_stream = SOCK_STREAM;
	const int sock_dgram = SOCK_DGRAM;
#else // ! USE_SOCKET
	const int sock_stream = 1;
	const int sock_dgram = 2;
#endif // ! USE_SOCKET

	int socket_type = sock_stream;
	if (HP.IsKeyWord("socket" "type")) {
		if (HP.IsKeyWord("udp")) {
			socket_type = sock_dgram;

			if (!bGotCreate) {
				bCreate = true;
			}

		} else if (!HP.IsKeyWord("tcp")) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"invalid socket type "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}

	if ((socket_type == sock_dgram) && bCreate) {
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"socket type=udp incompatible with create=yes "
			"at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	bool bNonBlocking = false;
	bool bNoSignal = false;
	bool bSendFirst = true;
	bool bAbortIfBroken = false;
	while (HP.IsArg()) {
		if (HP.IsKeyWord("no" "signal")) {
			bNoSignal = true;

		} else if (HP.IsKeyWord("signal")) {
			bNoSignal = false;

		} else if (HP.IsKeyWord("blocking")) {
			bNonBlocking = false;

		} else if (HP.IsKeyWord("non" "blocking")) {
			bNonBlocking = true;

		} else if (HP.IsKeyWord("no" "send" "first")) {
			bSendFirst = false;

		} else if (HP.IsKeyWord("send" "first")) {
			bSendFirst = true;

		} else if (HP.IsKeyWord("abort" "if" "broken")) {
			bAbortIfBroken = true;

		} else if (HP.IsKeyWord("do" "not" "abort" "if" "broken")) {
			bAbortIfBroken = false;

		} else {
			break;
		}
	}

	unsigned int OutputEvery = 1;
	if (HP.IsKeyWord("output" "every")) {
		int i = HP.GetInt();
		if (i <= 0) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"invalid output every value " << i << " "
		 		"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		OutputEvery = (unsigned int)i;
	}

	StreamOutEcho *pSOE = ReadStreamOutEcho(HP);
	StreamContent *pSC = ReadStreamContent(pDM, HP, type);

	/* Se non c'e' il punto e virgola finale */
	if (HP.IsArg()) {
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"semicolon expected "
			"at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	Elem *pEl = 0;
	
	// .log file output
	std::ostream& out = pDM->GetLogFile();
	out 
		<< "outputelement: " << uLabel 
		<< " stream";

	if (bIsRTAI) {
#ifdef USE_RTAI
		if (pSOE != 0) {
			silent_cerr("SocketStreamElem(" << uLabel << "): "
				"echo ignored in RTAI mode"
				<< std::endl);
		}

		unsigned long node = (unsigned long)-1;
#if defined(HAVE_GETADDRINFO)
		struct addrinfo hints = { 0 }, *res = NULL;
		int rc;

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM; // FIXME: SOCK_DGRAM?
		rc = getaddrinfo(host.c_str(), NULL, &hints, &res);
		if (rc == 0) {
			node = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
			freeaddrinfo(res);
		}
#elif defined(HAVE_GETHOSTBYNAME)
		struct hostent *he = gethostbyname(host.c_str());
		if (he != NULL) {
			node = ((unsigned long *)he->h_addr_list[0])[0];
		} 
#elif defined(HAVE_INET_ATON)
		struct in_addr addr;
		if (inet_aton(host.c_str(), &addr)) {
			node = addr.s_addr;
		}
#else // ! HAVE_GETADDRINFO && ! HAVE_GETHOSTBYNAME && ! HAVE_INET_ATON
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"host (RTAI RPC) not supported "
			"at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
#endif // ! HAVE_GETADDRINFO && ! HAVE_GETHOSTBYNAME && ! HAVE_INET_ATON

		if (node == (unsigned long)-1) {
			silent_cerr("RTMBDynInDrive(" << uLabel << "): "
				"unable to convert host \"" << host << "\" to node" << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

		silent_cerr("starting RTMBDynOutputElement(" << uLabel << ")..."
			<< std::endl);
		SAFENEWWITHCONSTRUCTOR(pEl, RTMBDynOutElem,
			RTMBDynOutElem(uLabel,
       				name, host, node, bCreate, pSC, bNonBlocking));

		out
			<< " " << "RTAI"
			<< " " << name
			<< " " << host
			<< " " << node
			<< " " << bCreate;

		WriteStreamContentLogOutput(pSC, out);

#endif // USE_RTAI

	} else {
		int flags = 0;

#ifdef USE_SOCKET
		/* costruzione del nodo */
		UseSocket *pUS = 0;
		if (path.empty()) {
			if (port == (unsigned short int)(-1)) {
				port = DEFAULT_PORT;
				silent_cerr("SocketStreamElem(" << uLabel << "): "
					"port undefined; using default port "
					<< port << " at line "
					<< HP.GetLineData() << std::endl);
			}
      
			SAFENEWWITHCONSTRUCTOR(pUS, UseInetSocket, UseInetSocket(host.c_str(), port, socket_type, bCreate));
		
			// .log file output
			out 
				<< " " << "INET"
				<< " " << name
				<< " " << host
				<< " " << port;
			if (socket_type == sock_dgram)	{
				out << " udp";
			} else {
				out << " tcp";
			}
				out << " " << bCreate;


		} else {
			SAFENEWWITHCONSTRUCTOR(pUS, UseLocalSocket, UseLocalSocket(path.c_str(), socket_type, bCreate));
			
			// .log file output
			out 
				<< " " << "UNIX"
				<< " " << name
				<< " " << path;
			if (socket_type == sock_dgram) {
				out << " udp";
			} else {
				out << " tcp";
			}
				out << " " << bCreate;	
		}

		if (bCreate) {
			pDM->RegisterSocketUser(pUS);

		} else {
			pUS->Connect();
		}

#ifdef MSG_NOSIGNAL
		if (bNoSignal) {
			// NOTE: we assume MSG_NOSIGNAL is a macro...
			flags |= MSG_NOSIGNAL;

		} else {
			flags &= ~MSG_NOSIGNAL;
		}
#else // !MSG_NOSIGNAL
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"MSG_NOSIGNAL undefined; "
			"your mileage may vary" << std::endl);
#endif // !MSG_NOSIGNAL

#ifdef MSG_DONTWAIT
		// NOTE: we assume MSG_DONTWAIT is a macro...
		if (bNonBlocking) {
			flags |= MSG_DONTWAIT;

		} else {
			flags &= ~MSG_DONTWAIT;
		}
#else // !MSG_DONTWAIT
		silent_cerr("SocketStreamElem(" << uLabel << "): "
			"MSG_DONTWAIT undefined; "
			"your mileage may vary" << std::endl);
#endif // !MSG_DONTWAIT

		silent_cerr("starting SocketStreamElem(" << uLabel << ")..."
			<< std::endl);
		SAFENEWWITHCONSTRUCTOR(pEl, SocketStreamElem,
			SocketStreamElem(uLabel, name, OutputEvery,
				pUS, pSC, flags, bSendFirst, bAbortIfBroken,
				pSOE));

		out 
			<< " " << !bNoSignal
			<< " " << !bNonBlocking
			<< " " << bSendFirst
			<< " " << bAbortIfBroken
			<< " " << OutputEvery;
		
		WriteStreamContentLogOutput(pSC, out);

#endif // USE_SOCKET
	}

	return pEl;
}

void
WriteStreamContentLogOutput(const StreamContent* pSC, 
	std::ostream& out)
{
	const StreamContentMotion* pSCM = NULL;
	pSCM = dynamic_cast<const StreamContentMotion*>(pSC);

	if (pSCM == NULL) 
	{
		// pSC is of type StreamContent::VALUES
		out 
			<< " values"
			<< " " << pSC->GetNumChannels()
			<< std::endl;
	} else {
		// pSC is of type StreamContent::MOTION
		const unsigned uFlags = pSCM->uGetFlags();
		out 
			<< " motion"
			<< " " << bool(uFlags & GeometryData::X)
			<< " " << bool(uFlags & GeometryData::R)
			<< " " << bool(uFlags & GeometryData::RT)
			<< " " << bool(uFlags & GeometryData::V)
			<< " " << bool(uFlags & GeometryData::W);

		for (std::vector<const StructNode*>::const_iterator i = pSCM->nodes_begin(); 
				i != pSCM->nodes_end(); ++i) 
		{
			out << " " << (*i)->GetLabel();
		}

		out << std::endl;
	}
}
