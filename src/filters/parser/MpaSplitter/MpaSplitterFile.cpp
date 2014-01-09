/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include <MMReg.h>
#include "MpaSplitterFile.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#define FRAMES_FLAG     0x0001
#define MPA_HEADER_SIZE 4	// MPEG-Audio Header Size

//

static const LPCTSTR s_genre[] = {
	_T("Blues"), _T("Classic Rock"), _T("Country"), _T("Dance"),
	_T("Disco"), _T("Funk"), _T("Grunge"), _T("Hip-Hop"),
	_T("Jazz"), _T("Metal"), _T("New Age"), _T("Oldies"),
	_T("Other"), _T("Pop"), _T("R&B"), _T("Rap"),
	_T("Reggae"), _T("Rock"), _T("Techno"), _T("Industrial"),
	_T("Alternative"), _T("Ska"), _T("Death Metal"), _T("Pranks"),
	_T("Soundtrack"), _T("Euro-Techno"), _T("Ambient"), _T("Trip-Hop"),
	_T("Vocal"), _T("Jazz+Funk"), _T("Fusion"), _T("Trance"),
	_T("Classical"), _T("Instrumental"), _T("Acid"), _T("House"),
	_T("Game"), _T("Sound Clip"), _T("Gospel"), _T("Noise"),
	_T("Alternative Rock"), _T("Bass"), _T("Soul"), _T("Punk"),
	_T("Space"), _T("Meditative"), _T("Instrumental Pop"), _T("Instrumental Rock"),
	_T("Ethnic"), _T("Gothic"), _T("Darkwave"), _T("Techno-Industrial"),
	_T("Electronic"), _T("Pop-Folk"), _T("Eurodance"), _T("Dream"),
	_T("Southern Rock"), _T("Comedy"), _T("Cult"), _T("Gangsta"),
	_T("Top 40"), _T("Christian Rap"), _T("Pop/Funk"), _T("Jungle"),
	_T("Native US"), _T("Cabaret"), _T("New Wave"), _T("Psychadelic"),
	_T("Rave"), _T("Showtunes"), _T("Trailer"), _T("Lo-Fi"),
	_T("Tribal"), _T("Acid Punk"), _T("Acid Jazz"), _T("Polka"),
	_T("Retro"), _T("Musical"), _T("Rock & Roll"), _T("Hard Rock"),
	_T("Folk"), _T("Folk-Rock"), _T("National Folk"), _T("Swing"),
	_T("Fast Fusion"), _T("Bebob"), _T("Latin"), _T("Revival"),
	_T("Celtic"), _T("Bluegrass"), _T("Avantgarde"), _T("Gothic Rock"),
	_T("Progressive Rock"), _T("Psychedelic Rock"), _T("Symphonic Rock"), _T("Slow Rock"),
	_T("Big Band"), _T("Chorus"), _T("Easy Listening"), _T("Acoustic"),
	_T("Humour"), _T("Speech"), _T("Chanson"), _T("Opera"),
	_T("Chamber Music"), _T("Sonata"), _T("Symphony"), _T("Booty Bass"),
	_T("Primus"), _T("Porn Groove"), _T("Satire"), _T("Slow Jam"),
	_T("Club"), _T("Tango"), _T("Samba"), _T("Folklore"),
	_T("Ballad"), _T("Power Ballad"), _T("Rhytmic Soul"), _T("Freestyle"),
	_T("Duet"), _T("Punk Rock"), _T("Drum Solo"), _T("Acapella"),
	_T("Euro-House"), _T("Dance Hall"), _T("Goa"), _T("Drum & Bass"),
	_T("Club-House"), _T("Hardcore"), _T("Terror"), _T("Indie"),
	_T("BritPop"), _T("Negerpunk"), _T("Polsk Punk"), _T("Beat"),
	_T("Christian Gangsta"), _T("Heavy Metal"), _T("Black Metal"),
	_T("Crossover"), _T("Contemporary C"), _T("Christian Rock"), _T("Merengue"), _T("Salsa"),
	_T("Thrash Metal"), _T("Anime"), _T("JPop"), _T("SynthPop"),
};

//

CMpaSplitterFile::CMpaSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFileEx(pAsyncReader, hr, false, true, true)
	, m_mode(none)
	, m_rtDuration(0)
	, m_startpos(0)
	, m_totalbps(0)
	, m_bIsVBR(false)
{
	if (SUCCEEDED(hr)) {
		hr = Init();
	}
}

// text encoding:
// 0 - ISO-8859-1
// 1 - UCS-2 (UTF-16 encoded Unicode with BOM)
// 2 - UTF-16BE encoded Unicode without BOM
// 3 - UTF-8 encoded Unicode

CString CMpaSplitterFile::ReadText(DWORD &size, BYTE encoding)
{
	WORD bom = (WORD)BitRead(16, true);

	CStringA	str;
	CString		wstr;

	if (encoding > 0 && size >= 2 && bom == 0xfffe) {
		BitRead(16);
		size = (size - 2) / 2;
		ByteRead((BYTE*)wstr.GetBufferSetLength(size), size*2);
		return wstr.Trim();
	} else if (encoding > 0 && size >= 2 && bom == 0xfeff) {
		BitRead(16);
		size = (size - 2) / 2;
		ByteRead((BYTE*)wstr.GetBufferSetLength(size), size*2);
		for (int i = 0, j = wstr.GetLength(); i < j; i++) {
			wstr.SetAt(i, (wstr[i]<<8)|(wstr[i]>>8));
		}
		return wstr.Trim();
	} else {
		ByteRead((BYTE*)str.GetBufferSetLength(size), size);
		return (encoding > 0 ? UTF8ToString(str) : CStringW(str)).Trim();
	}
}

CString CMpaSplitterFile::ReadField(DWORD &size, BYTE encoding)
{
	DWORD fieldSize	= 0;
	DWORD pos		= GetPos();

	CString wstr;
	
	if (encoding > 0) {
		while (size -= 2) {
			fieldSize += 2;

			WORD w = (WORD)BitRead(16);
			if (w == 0) {
				break;
			}
		}
	} else {
		while (size--) {
			fieldSize++;

			if (BitRead(8) == 0) {
				break;
			}
		}
	}

	if (fieldSize) {
		Seek(pos);
		wstr = ReadText(fieldSize, encoding);
	};

	return wstr;
}

HRESULT CMpaSplitterFile::Init()
{
	m_startpos = 0;
	__int64 endpos = GetLength();

	Seek(0);

	// some files can be determined as Mpeg Audio
	if ((BitRead(24, true) == 0x000001) || // ?
		(BitRead(32, true) == 'RIFF')   || // skip AVI and WAV files
		(BitRead(24, true) == 'AMV')) {    // skip MTV files (.amv .mtv)
		return E_FAIL;
	}

	if (endpos > 128 && IsRandomAccess()) {
		Seek(endpos - 128);

		if (BitRead(24) == 'TAG') {
			endpos -= 128;

			CStringA str;

			// title
			ByteRead((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['TIT2'] = CStringW(str).Trim();

			// artist
			ByteRead((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['TPE1'] = CStringW(str).Trim();

			// album
			ByteRead((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['TALB'] = CStringW(str).Trim();

			// year
			ByteRead((BYTE*)str.GetBufferSetLength(4), 4);
			m_tags['TYER'] = CStringW(str).Trim();

			// comment
			ByteRead((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['COMM'] = CStringW(str).Trim();

			// track
			LPCSTR s = str;
			if (s[28] == 0 && s[29] != 0) {
				m_tags['TRCK'].Format(L"%d", s[29]);
			}

			// genre
			BYTE genre = (BYTE)BitRead(8);
			if (genre < _countof(s_genre)) {
				m_tags['TCON'] = CStringW(s_genre[genre]);
			}
		}
	}

	Seek(0);

	bool MP3_find = false;

	while (BitRead(24, true) == 'ID3') {
		MP3_find = true;

		BitRead(24);

		BYTE major = (BYTE)BitRead(8);
		BYTE revision = (BYTE)BitRead(8);
		UNREFERENCED_PARAMETER(revision);

		BYTE flags = (BYTE)BitRead(8);
		UNREFERENCED_PARAMETER(flags);

		DWORD size = BitRead(32);
		size = (((size & 0x7F000000) >> 0x03) |
				((size & 0x007F0000) >> 0x02) |
				((size & 0x00007F00) >> 0x01) |
				((size & 0x0000007F)		));

		m_startpos = GetPos() + size;

		// TODO: read extended header
		if (major <= 4) {
			__int64 pos = GetPos();

			while (pos < m_startpos) {
				Seek(pos);
				
				DWORD tag	= 0;
				DWORD size	= 0;
				if (major == 2) {
					tag		= (DWORD)BitRead(24);
					size	= (DWORD)BitRead(24);
				} else {
					tag		= (DWORD)BitRead(32);
					size	|= BitRead(8) << 24;
					size	|= BitRead(8) << 16;
					size	|= BitRead(8) << 8;
					size	|= BitRead(8);
					WORD flags = (WORD)BitRead(16);
					UNREFERENCED_PARAMETER(flags);
				}

				pos += ((major == 2) ? 3+3 : 4+4+2) + size;

				if (pos > m_startpos || tag == 0) {
					break;
				}

				if (!size) {
					continue;
				}

				if (tag == 'TIT2'
						|| tag == 'TPE1'
						|| tag == 'TALB' || tag == '\0TAL'
						|| tag == 'TYER'
						|| tag == 'COMM'
						|| tag == 'TRCK'
						|| tag == 'TCOP'
						|| tag == '\0TP1'
						|| tag == '\0TT2'
						|| tag == '\0PIC' || tag == 'APIC'
						|| tag == '\0ULT' || tag == 'USLT'
						) {

					if (tag == 'APIC' || tag == '\0PIC') {
						if (!m_Cover.GetCount()) {

							TCHAR mime[64];
							memset(&mime, 0 ,64);
							BYTE encoding = (BYTE)BitRead(8);
							size--;

							int mime_len = 0;
							while (size-- && (mime[mime_len++] = BitRead(8)) != 0) {
								;
							}

							BYTE pic_type = (BYTE)BitRead(8);
							size--;

							if (tag == 'APIC') {
								CString Desc = ReadField(size, encoding);
								UNREFERENCED_PARAMETER(Desc);
							}

							m_CoverMime = mime;

							if (tag == '\0PIC') {
								if (CString(m_CoverMime).MakeUpper() == _T("JPG")) {
									m_CoverMime = _T("image/jpeg");
								} else if (CString(m_CoverMime).MakeUpper() == _T("PNG")) {
									m_CoverMime = _T("image/png");
								} else {
									m_CoverMime.Format(_T("image/%ws"), mime);
								}
							}

							m_Cover.SetCount(size);
							ByteRead(m_Cover.GetData(), size);
						}
					} else if (tag == '\0ULT' || tag == 'USLT') {
						// Text encoding
						BYTE encoding = (BYTE)BitRead(8);
						size--;
						
						// Language
						CHAR lang[3];
						memset(&lang, 0 ,3);
						ByteRead((BYTE*)lang, 3);
						UNREFERENCED_PARAMETER(lang);
						size -= 3;

						CString Desc = ReadField(size, encoding);
						UNREFERENCED_PARAMETER(Desc);

						m_tags[tag] = ReadText(size, encoding);
					} else {

						// Text encoding
						BYTE encoding = (BYTE)BitRead(8);
						size--;

						if (tag == 'COMM') {
							// Language
							CHAR lang[3];
							memset(&lang, 0 ,3);
							ByteRead((BYTE*)lang, 3);
							UNREFERENCED_PARAMETER(lang);
							size -= 3;

							if (BitRead(8, true) == 0) {
								BitRead(8);
								size--;
							}
	
							DWORD bom_big = (DWORD)BitRead(32, true);
							if (bom_big == 0xfffe0000 || bom_big == 0xfeff0000) {
								BitRead(32);
								size -= 4;
							}
						}

						m_tags[tag] = ReadText(size, encoding);
					}
				}
			}
		}

		Seek(m_startpos);

		for (int i = 0; i < (1<<20) && m_startpos < endpos && BitRead(8, true) == 0; i++) {
			BitRead(8), m_startpos++;
		}
	}

	int searchlen		= 0;
	__int64 startpos	= 0;
	__int64 syncpos		= 0;

	__int64 startpos_mp3 = m_startpos;

	__int64 len = min(GetLength(), 5 * MEGABYTE);
	while (m_mode == none) {
		if (!MP3_find && GetPos() >= 2048) {
			break;
		}

		if ((len - GetPos()) < 512) {
			break;
		}

		searchlen = (int)min(endpos - startpos_mp3, 512);
		Seek(startpos_mp3);

		// If we fail to see sync bytes, we reposition here and search again
		syncpos = startpos_mp3 + searchlen;

		// Check for a valid MPA header
		if (Read(m_mpahdr, searchlen, &m_mt, true)) {
			m_mode = mpa;

			// check multiple frame to ensure that the data is correct
			__int64 savepos = GetPos() - 4;
			for (int i = 0; i < 5; i++) {
				syncpos = GetPos();
				startpos = i ? syncpos : (syncpos - 4);
				Seek(startpos + m_mpahdr.FrameSize);
				if (!Sync(4)) {
					m_mode = none;
					break;
				}
			}

			if (m_mode == mpa) {
				startpos = savepos;
				break;
			}
		}

		// If we have enough room to search for a valid header, then skip ahead and try again
		if (endpos - syncpos >= 8) {
			startpos_mp3 = syncpos;
		} else {
			break;
		}
	}

	searchlen = (int)min(endpos - m_startpos, 512);
	Seek(m_startpos);

	if (m_mode == none && Read(m_aachdr, searchlen, &m_mt)) {
		m_mode = mp4a;

		startpos = GetPos() - (m_aachdr.fcrc?7:9);

		// make sure the first frame is followed by another of the same kind (validates m_aachdr basically)
		Seek(startpos + m_aachdr.aac_frame_length);
		if (!Sync(9)) {
			m_mode = none;
		}
	}

	if (m_mode == none) {
		return E_FAIL;
	}

	WaitAvailable(1500, 64 * KILOBYTE);

	m_startpos = startpos;

	if (m_mode == mpa) {
		DWORD m_dwFrames = 0;		// total number of frames
		Seek(m_startpos + MPA_HEADER_SIZE + 32);
		if (BitRead(32, true) == 'Xing' || BitRead(32, true) == 'Info') {
			BitRead(32); // Skip ID tag
			DWORD dwFlags = (DWORD)BitRead(32);
			// extract total number of frames in file
			if (dwFlags & FRAMES_FLAG) {
				m_dwFrames = (DWORD)BitRead(32);
			}
		} else if (BitRead(32, true) == 'VBRI') {
			BitRead(32); // Skip ID tag
			// extract all fields from header (all mandatory)
			BitRead(16); // version
			BitRead(16); // delay
			BitRead(16); // quality
			BitRead(32); // bytes
			m_dwFrames = (DWORD)BitRead(32); // extract total number of frames in file
		}

		if (m_dwFrames) {
			bool l3ext = m_mpahdr.layer == 3 && !(m_mpahdr.version&1);
			DWORD m_dwSamplesPerFrame = m_mpahdr.layer == 1 ? 384 : l3ext ? 576 : 1152;

			m_bIsVBR = true;
			m_rtDuration = 10000000i64 * (m_dwFrames * m_dwSamplesPerFrame / m_mpahdr.nSamplesPerSec);
		}
	}

	Seek(m_startpos);

	int FrameSize;
	REFERENCE_TIME rtFrameDur, rtPrevDur = -1;
	clock_t start = clock();
	int i = 0;
	while (Sync(FrameSize, rtFrameDur) && (clock() - start) < CLOCKS_PER_SEC) {
		TRACE(_T("%I64d\n"), m_rtDuration);
		Seek(GetPos() + FrameSize);
		i = rtPrevDur == m_rtDuration ? i+1 : 0;
		if (i == 10) {
			break;
		}
		rtPrevDur = m_rtDuration;
	}

	return S_OK;
}

bool CMpaSplitterFile::Sync(int limit)
{
	int FrameSize;
	REFERENCE_TIME rtDuration;
	return Sync(FrameSize, rtDuration, limit);
}

bool CMpaSplitterFile::Sync(int& FrameSize, REFERENCE_TIME& rtDuration, int limit)
{
	__int64 endpos = min(GetLength(), GetPos() + limit);

	if (m_mode == mpa) {
		while (GetPos() <= endpos - 4) {
			mpahdr h;

			if (Read(h, (int)(endpos - GetPos()), NULL, true)) {
				if (m_mpahdr == h) {
					Seek(GetPos() - 4);
					AdjustDuration(h.nBytesPerSec);

					FrameSize	= h.FrameSize;
					rtDuration	= h.rtDuration;

					memcpy(&m_mpahdr, &h, sizeof(mpahdr));

					return true;
				}
			} else {
				break;
			}
		}
	} else if (m_mode == mp4a) {
		while (GetPos() <= endpos - 9) {
			aachdr h;

			if (Read(h, (int)(endpos - GetPos()))) {
				if (m_aachdr == h) {
					Seek(GetPos() - (h.fcrc?7:9));
					AdjustDuration(h.nBytesPerSec);
					Seek(GetPos() + (h.fcrc?7:9));

					FrameSize	= h.FrameSize;
					rtDuration	= h.rtDuration;

					return true;
				}
			} else {
				break;
			}
		}
	}

	return false;
}

void CMpaSplitterFile::AdjustDuration(int nBytesPerSec)
{
	ASSERT(nBytesPerSec);

	if (!m_bIsVBR) {
		int rValue;
		if (!m_pos2bps.Lookup(GetPos(), rValue)) {
			m_totalbps += nBytesPerSec;
			if (!m_totalbps) {
				return;
			}
			m_pos2bps.SetAt(GetPos(), nBytesPerSec);
			__int64 avgbps = m_totalbps / m_pos2bps.GetCount();
			m_rtDuration = 10000000i64 * (GetTotal() - m_startpos) / avgbps;
		}
	}
}
