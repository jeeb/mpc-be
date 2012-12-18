/*****************************************************************
|
|    AP4 - chap Atom
|
|    Copyright 2012 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_CHAP_ATOM_H_
#define _AP4_CHAP_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Types.h"
#include "Ap4Array.h"
#include "Ap4DataBuffer.h"

/*----------------------------------------------------------------------
|       AP4_ChapAtom
+---------------------------------------------------------------------*/

class AP4_ChapAtom : public AP4_Atom
{
public:
	AP4_ChapAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

	AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

	AP4_UI32 GetChapterTrackId() const { return m_ChapterTrackId; }

private:
	AP4_UI32 m_ChapterTrackId;

};

#endif // _AP4_CHAP_ATOM_H_
