/*****************************************************************
|
|    AP4 - moof Atom
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/
/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4MoofAtom.h"

/*----------------------------------------------------------------------
|       AP4_MoofAtom::AP4_MoofAtom
+---------------------------------------------------------------------*/
AP4_MoofAtom::AP4_MoofAtom(AP4_Size         size,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory)
	: AP4_ContainerAtom(AP4_ATOM_TYPE_MOOF, size, false, stream, atom_factory)
	, m_MoofOffset(0)
{
	if (AP4_SUCCEEDED(stream.Tell(m_MoofOffset))) {
		m_MoofOffset -= size;
	}
}
