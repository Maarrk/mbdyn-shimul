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

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include <ac/sstream>
#include <dofpgin.h>

DofPlugIn::DofPlugIn(MathParser& mp, DataManager *pDM)
: MathParser::PlugIn(mp), pDM(pDM), bPrev(false)
{
	ASSERT(pDM != NULL);
}
	
DofPlugIn::~DofPlugIn(void) 
{
	NO_OP;
}

const char *
DofPlugIn::sName(void) const 
{
	return "dof";
}

int 
DofPlugIn::Read(int argc, char *argv[])
{
	unsigned int uLabel;
	Node *pNode;
	int iOrder = 0;
	int iParams = 2;

	if (argc < 1 || argv[0] == NULL) {
		std::cerr << "DofPlugIn::Read(): "
			"illegal number of parameters " << argc
			<< std::endl;
		throw ErrGeneric();
	}
	uLabel = ReadLabel(argv[0]);

	if (argc < 2 || argv[1] == NULL) {
		std::cerr << "DofPlugIn::Read(" << argv[0] 
			<< "): illegal number of parameters " << argc
			<< std::endl;
		throw ErrGeneric();
	}
	pNode = ReadNode(uLabel, argv[1]);

	unsigned int iMaxIndex = pNode->iGetNumDof();
	switch (iMaxIndex) {
	case 0:
		/* parameter? */
		ASSERT(pNode->GetNodeType() == Node::PARAMETER);
		break;

	case 1:
		iParams++;

		if (argc < 3 || argv[2] == NULL) {
			std::cerr << "DofPlugIn::Read(" << argv[0]
				<< "," << argv[1] 
				<< "): illegal number of parameters " << argc 
				<< std::endl;
			throw ErrGeneric();
		}
		iOrder = ReadDofOrder(pNode, 1, argv[2]);
		break;

	default: {
		iParams += 2;

		if (argc < 3 || argv[2] == NULL) {
			std::cerr << "DofPlugIn::Read(" << argv[0]
				<< "," << argv[1] 
				<< "): illegal number of parameters " << argc 
				<< std::endl;
			throw ErrGeneric();
		}
		unsigned int iIndex = ReadIndex(pNode, iMaxIndex, argv[2]);

		if (argc < 4 || argv[3] == NULL) {
			std::cerr << "DofPlugIn::Read(" << argv[0]
				<< "," << argv[1] 
				<< "," << argv[2] 
				<< "): illegal number of parameters " << argc 
				<< std::endl;
			throw ErrGeneric();
		}
		iOrder = ReadDofOrder(pNode, iIndex, argv[3]);

		NodeDof nd(pNode->GetLabel(), iIndex-1, pNode);
		pNode = NULL;
		/* Chi dealloca questa memoria? ci vorrebbe l'handle */
		SAFENEWWITHCONSTRUCTOR(pNode, Node2Scalar, Node2Scalar(nd));
		pedantic_cerr(psNodeNames[pNode->GetNodeType()] 
				<< "(" << pNode->GetLabel() 
				<< "): possibly allocating a NodeDof "
		 		"that nobody will delete until handles "
				"will be used" << std::endl);
		break;
	}
	}
	dof = ScalarDof((ScalarNode*)pNode, iOrder);

	/* legge i parametri (<param>=<value>, separati da '&') */
	if (argc == iParams + 1) {
		char *parm = NULL, *p, *end;

		SAFESTRDUP(parm, argv[iParams]);

		for (p = parm; p != NULL; p = end) {
			end = strchr(p, '&');
			while (end != NULL && end[-1] == '\\') {
				end = strchr(end+1, '&');
			}

			if (end != NULL) {
				end[0] = '\0';
				end++;
			}

			char *v = strchr(p, '=');
			if (v == NULL) {
				std::cerr << "dof plugin: missing \"=\" "
					"in <param>=<value>" << std::endl;
				throw ErrGeneric();
			}

			v[0] = '\0';
			v++;

			/* prende il valore al passo precedente */
			if (strncasecmp(p, "prev", sizeof("prev") - 1) == 0) {
				if (strcasecmp(v, "true") == 0) {
					bPrev = true;
				} else if (strcasecmp(v, "false") == 0) {
					bPrev = false;
				} else {
					std::cerr << "dof plugin: "
						"unknown mode for parameter "
						"\"" << p << "=" << v << "\""
						<< std::endl;
					throw ErrGeneric();
				}

			} else {
				std::cerr << "dof plugin: unknown parameter "
					"\"" << p << "=" << v << "\""
					<< std::endl;
				throw ErrGeneric();
			}
		}

		SAFEDELETEARR(parm);
	}

	return 0;
}

TypedValue::Type
DofPlugIn::GetType(void) const 
{
	return TypedValue::VAR_REAL;
}

TypedValue 
DofPlugIn::GetVal(void) const 
{
	if (bPrev) {
		return TypedValue(dof.dGetValuePrev());
	}
	return TypedValue(dof.dGetValue());
}

unsigned int 
DofPlugIn::ReadLabel(const char* s) 
{
	unsigned int rc;
	char *stmp = NULL;

	/*
	 * deve essere terminato da ';' per essere letto da math parser :(
	 */
	int l = strlen(s)+2;
	SAFENEWARR(stmp, char, l);
	strcpy(stmp, s);
	strcat(stmp, ";");
#if defined(HAVE_SSTREAM)
	std::istringstream in(stmp);
#else /* HAVE_STRSTREAM_H */
	istrstream in(stmp);
#endif /* HAVE_STRSTREAM_H */
	InputStream In(in);
	rc = (unsigned int)mp.Get(In);
	SAFEDELETEARR(stmp);

	return rc;
}

Node *
DofPlugIn::ReadNode(unsigned int uLabel, const char *s) 
{
	Node *pNode = NULL;
	unsigned int i;
	for (i = 0; i < Node::LASTNODETYPE; i++) {
		if (strcasecmp(s, psReadNodesNodes[i]) == 0) {
			break;
		}
	}
	if (i == Node::LASTNODETYPE) {
		std::cerr << "unknown node type '" << s << "'" << std::endl;
		throw ErrGeneric();
	}
	if ((pNode = pDM->pFindNode(Node::Type(i), uLabel)) == NULL) {
		std::cerr << psNodeNames[Node::Type(i)] 
			<< "(" << uLabel << ") not defined" << std::endl;
		throw ErrGeneric();
	}
	return pNode;
}

unsigned int 
DofPlugIn::ReadIndex(Node *pNode, unsigned int iMaxIndex, const char *s) 
{
	unsigned int i = ReadLabel(s);
	if (i == 0 || i > iMaxIndex) {
		std::cerr << "illegal index " << i << " for "
			<< psNodeNames[pNode->GetNodeType()]
			<< "(" << pNode->GetLabel() << ")" << std::endl;
		throw ErrGeneric();
	}
	return i;
}

int 
DofPlugIn::ReadDofOrder(Node *pNode, unsigned int iIndex, const char *s) 
{
	if (strcasecmp(s, "differential") == 0) {
		if (pNode->GetDofType(iIndex-1) != DofOrder::DIFFERENTIAL) {
			std::cerr << "cannot take differential value of "
				<< psNodeNames[pNode->GetNodeType()] 
				<< "(" << pNode->GetLabel() << ")[" 
				<< iIndex << "]" << std::endl;
			throw ErrGeneric();
		}
		return 1;
	}
	
	if (strcasecmp(s, "algebraic") == 0) {
		return 0;
	} 

	std::cerr << "unknown dof order '" << s << "'" << std::endl;
	throw ErrGeneric();
}

MathParser::PlugIn *
dof_plugin(MathParser& mp, void *arg)
{
	MathParser::PlugIn *p = NULL;
	SAFENEWWITHCONSTRUCTOR(p, DofPlugIn, DofPlugIn(mp, (DataManager *)arg));
	return p;
}

