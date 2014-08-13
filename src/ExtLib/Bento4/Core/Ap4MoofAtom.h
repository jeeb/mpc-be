/*****************************************************************
|
|    AP4 - moof Atom
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_MOOF_ATOM_H_
#define _AP4_MOOF_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4ContainerAtom.h"

/*----------------------------------------------------------------------
|       AP4_MoofAtom
+---------------------------------------------------------------------*/
class AP4_MoofAtom : public AP4_ContainerAtom
{
public:
	// methods
	AP4_MoofAtom(AP4_Size         size,
                 AP4_ByteStream&  stream,
                 AP4_AtomFactory& atom_factory);

	virtual AP4_Result InspectFields(AP4_AtomInspector& inspector) { return AP4_FAILURE; }
	virtual AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

	AP4_UI64 GetOffset() { return m_MoofOffset; }

private:
	// members
	AP4_UI64 m_MoofOffset;
};

#endif // _AP4_MOOF_ATOM_H_

