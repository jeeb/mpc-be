/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
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
#include "MpegSplitterFile.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

CMpegSplitterFile::CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, CHdmvClipInfo &ClipInfo, bool bIsBD, bool ForcedSub, int AC3CoreOnly, bool AlternativeDuration, bool SubEmptyPin)
	: CBaseSplitterFileEx(pAsyncReader, hr, false, true)
	, m_type(mpeg_us)
	, m_rate(0)
	, m_rtMin(0), m_rtMax(0)
	, m_posMin(0), m_posMax(0)
	, m_ClipInfo(ClipInfo)
	, m_bIsBD(bIsBD)
	, m_ForcedSub(ForcedSub)
	, m_AC3CoreOnly(AC3CoreOnly)
	, m_AlternativeDuration(AlternativeDuration)
	, m_SubEmptyPin(SubEmptyPin)
	, m_init(FALSE)
	, bIsBadPacked(FALSE)
{
	memset(m_psm, 0, sizeof(m_psm));
	if (SUCCEEDED(hr)) {
		hr = Init(pAsyncReader);
	}
}

HRESULT CMpegSplitterFile::Init(IAsyncReader* pAsyncReader)
{
	if (m_ClipInfo.IsHdmv()) {
		if (CComQIPtr<ISyncReader> pReader = pAsyncReader) {
			// To support seamless BD playback, CMultiFiles should update m_rtPTSOffset variable each time when a new part is opened
			// use this code only if Blu-ray is detected
			pReader->SetPTSOffset(&m_rtPTSOffset);
		}
	}

	WaitAvailable(3000, MEGABYTE);

	// get the type first
	m_type = mpeg_us;

	Seek(0);

	if (m_type == mpeg_us) {
		if (BitRead(32, true) == 'TFrc') {
			Seek(0x67c);
		}
		int cnt = 0, limit = 4;
		for (trhdr h; cnt < limit && (Read(h) != -1); cnt++) {
			Seek(h.next);
		}
		if (cnt >= limit) {
			m_type = mpeg_ts;
		}
	}

	Seek(0);

	if (m_type == mpeg_us) {
		if (BitRead(32, true) == 'TFrc') {
			Seek(0xE80);
		}
		int cnt = 0, limit = 4;
		for (trhdr h; cnt < limit && (Read(h) != -1); cnt++) {
			Seek(h.next);
		}
		if (cnt >= limit) {
			m_type = mpeg_ts;
		}
	}

	Seek(0);

	if (m_type == mpeg_us) {
		int cnt = 0, limit = 4;
		for (pvahdr h; cnt < limit && Read(h); cnt++) {
			Seek(GetPos() + h.length);
		}
		if (cnt >= limit) {
			m_type = mpeg_pva;
		}
	}

	Seek(0);

	if (m_type == mpeg_us) {
		BYTE b;
		for (int i = 0; (i < 4 || GetPos() < (MEGABYTE/2)) && m_type == mpeg_us && NextMpegStartCode(b); i++) {
			if (b == 0xba) {
				pshdr h;
				if (Read(h)) {
					m_type = mpeg_ps;
					m_rate = int(h.bitrate/8);
					break;
				}
			} else if ((b&0xe0) == 0xc0 // audio, 110xxxxx, mpeg1/2/3
					   || (b&0xf0) == 0xe0 // video, 1110xxxx, mpeg1/2
					   // || (b&0xbd) == 0xbd) // private stream 1, 0xbd, ac3/dts/lpcm/subpic
					   || b == 0xbd) { // private stream 1, 0xbd, ac3/dts/lpcm/subpic
				peshdr h;
				if (Read(h, b) && BitRead(24, true) == 0x000001) {
					m_type = mpeg_es;
				}
			}
		}
	}

	Seek(0);

	if (m_type == mpeg_us) {
		return E_FAIL;
	}

	// min/max pts & bitrate
	m_rtMin = m_posMin = _I64_MAX;
	m_rtMax = m_posMax = 0;

	m_init = TRUE;

	if (IsRandomAccess() || IsStreaming()) {
		
		WaitAvailable(5000, MEGABYTE*2);
		SearchPrograms(0, min(GetLength(), IsStreaming() ? MEGABYTE*2 : MEGABYTE*5)); // max 5Mb for search a valid Program Map Table

		__int64 pfp = 0;
		const int k = 20;
		for (int i = 0; i <= k; i++) {
			__int64 fp = i * GetLength() / k;
			fp = min(GetLength() - MEGABYTE/2, fp);
			fp = max(pfp, fp);
			__int64 nfp = fp + (pfp == 0 ? 10*MEGABYTE : MEGABYTE/4);
			SearchStreams(fp, nfp, pAsyncReader);
			pfp = nfp;
		}
	} else {
		SearchStreams(0, MEGABYTE/2, pAsyncReader);
	}

	if (!m_bIsBD) {
		bool bAlternativeDuration = m_AlternativeDuration;
		int step = 1;

		for (;;) {
			if (m_type == mpeg_ts) {
				if (IsRandomAccess() || IsStreaming()) {
					WaitAvailable(3000, MEGABYTE);

					__int64 pfp = 0;
					const int k = 20;
					for (int i = 0; i <= k; i++) {
						__int64 fp = i * GetLength() / k;
						fp = min(GetLength() - MEGABYTE/8, fp);
						fp = max(pfp, fp);
						__int64 nfp = fp + (pfp == 0 ? MEGABYTE : MEGABYTE/16);
						SearchStreams(fp, nfp, pAsyncReader, TRUE);
						pfp = nfp;
					}
				} else {
					SearchStreams(0, MEGABYTE/2, pAsyncReader, TRUE);
				}
			}

			if (m_posMax - m_posMin <= 0 || m_rtMax - m_rtMin <= 0) {
				if (m_type == mpeg_ts && step == 1) {
					step++;
					m_AlternativeDuration = !m_AlternativeDuration;
					continue;
				}

				return E_FAIL;
			}

			break;
		}

		m_AlternativeDuration = bAlternativeDuration;

		int indicated_rate = m_rate;
		int detected_rate = int(m_rtMax > m_rtMin ? 10000000i64 * (m_posMax - m_posMin) / (m_rtMax - m_rtMin) : 0);

		m_rate = detected_rate ? detected_rate : m_rate;
#if (0)
		// normally "detected" should always be less than "indicated", but sometimes it can be a few percent higher (+10% is allowed here)
		// (update: also allowing +/-50k/s)
		if (indicated_rate == 0 || ((float)detected_rate / indicated_rate) < 1.1 || abs(detected_rate - indicated_rate) < 50*1024) {
			m_rate = detected_rate;
		} else {
			;    // TODO: in this case disable seeking, or try doing something less drastical...
		}
#endif
	} else {
		m_rtMin = m_rtMax = 0;

		m_posMin = 0;
		m_posMax = GetLength();
	}

	m_init = FALSE;

	int videoCount = 0;
	int audioCount = 0;
	for (int type = stream_type::video; type <= stream_type::audio; type++) {
		POSITION pos = m_streams[type].GetHeadPosition();
		while (pos) {
			DWORD TrackNum = m_streams[type].GetNext(pos);
			if (streamPTSCount.Lookup(TrackNum)) {
				if (type == stream_type::video) {
					videoCount += streamPTSCount[TrackNum];
				} else {
					audioCount += streamPTSCount[TrackNum];
				}
			}
		}
	}

	if (audioCount && videoCount) {
		bIsBadPacked = (audioCount > videoCount * 5);
	}

	if (m_SubEmptyPin) {
		// Add fake Subtitle stream ...
		if (m_type == mpeg_ts) {
			if (m_streams[stream_type::video].GetCount()) {
				if (!m_ClipInfo.IsHdmv() && m_streams[stream_type::subpic].GetCount()) {
					stream s;
					s.pid = NO_SUBTITLE_PID;
					s.mt.majortype	= m_streams[stream_type::subpic].GetHead().mt.majortype;
					s.mt.subtype	= m_streams[stream_type::subpic].GetHead().mt.subtype;
					s.mt.formattype	= m_streams[stream_type::subpic].GetHead().mt.formattype;
					s.mts.push_back(s.mt);
					m_streams[stream_type::subpic].AddTail(s);
				} else {
					AddHdmvPGStream(NO_SUBTITLE_PID, "---");
				}
			}
		} else {
			if (m_streams[stream_type::video].GetCount()) {
				stream s;
				s.pid = NO_SUBTITLE_PID;
				s.mt.majortype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.majortype		: MEDIATYPE_Video;
				s.mt.subtype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.subtype		: MEDIASUBTYPE_DVD_SUBPICTURE;
				s.mt.formattype	= m_streams[stream_type::subpic].GetCount() ? m_streams[stream_type::subpic].GetHead().mt.formattype	: FORMAT_None;
				s.mts.push_back(s.mt);
				m_streams[stream_type::subpic].AddTail(s);
			}
		}
	}

	Seek(0);

	return S_OK;
}

void CMpegSplitterFile::OnComplete(IAsyncReader* pAsyncReader)
{
	__int64 pos = GetPos();

	{
		SearchStreams(GetLength() - 500*1024, GetLength(), pAsyncReader, TRUE);
		int indicated_rate = m_rate;
		int detected_rate = int(m_rtMax > m_rtMin ? 10000000i64 * (m_posMax - m_posMin) / (m_rtMax - m_rtMin) : 0);

		m_rate = detected_rate ? detected_rate : m_rate;
#if (0)
		// normally "detected" should always be less than "indicated", but sometimes it can be a few percent higher (+10% is allowed here)
		// (update: also allowing +/-50k/s)
		if (indicated_rate == 0 || ((float)detected_rate / indicated_rate) < 1.1 || abs(detected_rate - indicated_rate) < 50*1024) {
			m_rate = detected_rate;
		} else {
			;    // TODO: in this case disable seeking, or try doing something less drastical...
		}
#endif
	}

	Seek(pos);
}

REFERENCE_TIME CMpegSplitterFile::NextPTS(DWORD TrackNum)
{
	REFERENCE_TIME rt	= INVALID_TIME;
	__int64 rtpos		= -1;
	__int64 pos			= GetPos();

	BYTE b;

	while (GetRemaining()) {
		if (m_type == mpeg_ps || m_type == mpeg_es) {
			if (!NextMpegStartCode(b)) {	// continue;
				ASSERT(0);
				break;
			}

			rtpos = GetPos()-4;

			if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) {
				peshdr h;
				if (!Read(h, b) || !h.len) {
					continue;
				}

				__int64 pos = GetPos();

				if (h.fpts && AddStream(0, b, h.id_ext, h.len) == TrackNum) {
					//ASSERT(h.pts >= m_rtMin && h.pts <= m_rtMax);
					rt = h.pts;
					break;
				}

				Seek(pos + h.len);
			}
		} else if (m_type == mpeg_ts) {
			trhdr h;
			__int64 pos = Read(h);
			if (pos == -1) {
				continue;
			}

			if (h.payloadstart && ISVALIDPID(h.pid) && h.pid == TrackNum) {
				peshdr h2;
				if (NextMpegStartCode(b, 4) && Read(h2, b) && h2.fpts) { // pes packet
					rt		= h2.pts;
					rtpos	= pos;
					break;
				}
			}

			Seek(h.next);
		} else if (m_type == mpeg_pva) {
			pvahdr h;
			if (!Read(h)) {
				continue;
			}

			if (h.fpts) {
				rt = h.pts;
				break;
			}
		}
	}

	if (rtpos >= 0) {
		Seek(rtpos);
	}
	if (rt != INVALID_TIME) {
		rt -= m_rtMin;
	} else {
		Seek(pos);
	}

	return rt;
}

void CMpegSplitterFile::SearchPrograms(__int64 start, __int64 stop)
{
	if (m_type != mpeg_ts) {
		return;
	}

	Seek(start);
	stop = min(stop, GetLength());

	while (GetPos() < stop) {
		trhdr h;
		if (Read(h) == -1) {
			continue;
		}

		UpdatePrograms(h);
		Seek(h.next);
	}
}

void CMpegSplitterFile::SearchStreams(__int64 start, __int64 stop, IAsyncReader* pAsyncReader, BOOL CalcDuration)
{
	Seek(start);
	stop = min(stop, GetLength());

	while (GetPos() < stop) {
		BYTE b;

		if (m_type == mpeg_ps || m_type == mpeg_es) {
			if (!NextMpegStartCode(b)) {
				continue;
			}

			if (b == 0xba) { // program stream header
				pshdr h;
				if (!Read(h)) {
					continue;
				}
				m_rate = int(h.bitrate/8);
			} else if (b == 0xbc) { // program stream map
				UpdatePSM();
			} else if (b == 0xbb) { // program stream system header
				pssyshdr h;
				if (!Read(h)) {
					continue;
				}
			}
			else if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) { // pes packet
				peshdr h;
				if (!Read(h, b) || !h.len) {
					continue;
				}

				if (h.type == mpeg2 && h.scrambling) {
					Seek(GetPos() + h.len);
				}

				if (h.fpts) {
					if (m_rtMin == _I64_MAX) {
						m_rtMin = h.pts;
						m_posMin = GetPos();
						DbgLog((LOG_TRACE, 3, L"CMpegSplitterFile::SearchStreams() : m_rtMin = %s [%10I64d], pesID = %d", ReftimeToString(m_rtMin), m_rtMin, b));
					}
					if (m_rtMin < h.pts && m_rtMax < h.pts) {
						m_rtMax = h.pts;
						m_posMax = GetPos();
						DbgLog((LOG_TRACE, 3, L"CMpegSplitterFile::SearchStreams() : m_rtMax = %s [%10I64d], pesID = %d", ReftimeToString(m_rtMax), m_rtMax, b));
					}
				}

				__int64 pos = GetPos();
				DWORD TrackNum = AddStream(0, b, h.id_ext, h.len);
				if (h.fpts) {
					if (!streamPTSCount.Lookup(TrackNum)) {
						streamPTSCount[TrackNum] = 0;
					}
					streamPTSCount[TrackNum]++;
				}
				if (h.len) {
					Seek(pos + h.len);
				}
			}
		} else if (m_type == mpeg_ts) {
			trhdr h;
			if (Read(h) == -1) {
				continue;
			}

			__int64 pos = GetPos();

			if (h.payload && ISVALIDPID(h.pid)) {
				peshdr h2;
				if (h.payloadstart && NextMpegStartCode(b, 4) && Read(h2, b)) { // pes packet
					if (h2.type == mpeg2 && h2.scrambling) {
						Seek(h.next);
					}

					if (h2.fpts && CalcDuration && (m_AlternativeDuration || (GetMasterStream() && GetMasterStream()->GetHead() == h.pid))) {
						if ((m_rtMin == _I64_MAX)/* || (m_rtMin > h2.pts)*/) {
							m_rtMin = h2.pts;
							m_posMin = GetPos();
							DbgLog((LOG_TRACE, 3, L"CMpegSplitterFile::SearchStreams() : m_rtMin = %s [%10I64d], pID = %d", ReftimeToString(m_rtMin), m_rtMin, h.pid));
						}

						if (m_rtMin < h2.pts && m_rtMax < h2.pts) {
							m_rtMax = h2.pts;
							m_posMax = GetPos();
							DbgLog((LOG_TRACE, 3, L"CMpegSplitterFile::SearchStreams() : m_rtMax = %s [%10I64d], pID = %d", ReftimeToString(m_rtMax), m_rtMax, h.pid));
						}
					}
				} else {
					b = 0;
				}

				if (!CalcDuration) {
					AddStream(h.pid, b, 0, DWORD(h.bytes - (GetPos() - pos)));
				}
			}

			Seek(h.next);
		} else if (m_type == mpeg_pva) {
			pvahdr h;
			if (!Read(h)) {
				continue;
			}

			if (h.fpts) {
				if (m_rtMin == _I64_MAX) {
					m_rtMin = h.pts;
					m_posMin = GetPos();
				}

				if (m_rtMin < h.pts && m_rtMax < h.pts) {
					m_rtMax = h.pts;
					m_posMax = GetPos();
				}
			}

			__int64 pos = GetPos();

			if (h.streamid == 1) {
				AddStream(h.streamid, 0xe0, 0, h.length);
			} else if (h.streamid == 2) {
				AddStream(h.streamid, 0xc0, 0, h.length);
			}

			if (h.length) {
				Seek(pos + h.length);
			}
		}
	}

	return;
}

#define MPEG_AUDIO					(1ULL << 0)
#define AAC_AUDIO					(1ULL << 1)
#define AC3_AUDIO					(1ULL << 2)
#define DTS_AUDIO					(1ULL << 3)
#define LPCM_AUDIO					(1ULL << 4)
#define MPEG2_VIDEO					(1ULL << 5)
#define H264_VIDEO					(1ULL << 6)
#define VC1_VIDEO					(1ULL << 7)
#define DIRAC_VIDEO					(1ULL << 8)
#define HEVC_VIDEO					(1ULL << 9)
#define PGS_SUB						(1ULL << 10)
#define DVB_SUB						(1ULL << 11)

#define PES_STREAM_TYPE_ANY			(MPEG_AUDIO | AAC_AUDIO | AC3_AUDIO | DTS_AUDIO/* | LPCM_AUDIO */| MPEG2_VIDEO | H264_VIDEO | DIRAC_VIDEO | HEVC_VIDEO/* | PGS_SUB*/ | DVB_SUB)

struct StreamType {
	PES_STREAM_TYPE pes_stream_type;
	ULONGLONG		stream_type;
};

static const StreamType PES_types[] = {
	// MPEG Audio
	{ AUDIO_STREAM_MPEG1,				MPEG_AUDIO	},
	{ AUDIO_STREAM_MPEG2,				MPEG_AUDIO	},
	// AAC Audio
	{ AUDIO_STREAM_AAC,					AAC_AUDIO	},
	{ AUDIO_STREAM_AAC_LATM,			AAC_AUDIO	},
	// AC3/TrueHD Audio
	{ AUDIO_STREAM_AC3,					AC3_AUDIO	},
	{ AUDIO_STREAM_AC3_PLUS,			AC3_AUDIO	},
	{ AUDIO_STREAM_AC3_TRUE_HD,			AC3_AUDIO	},
	{ SECONDARY_AUDIO_AC3_PLUS,			AC3_AUDIO	},
	{ PES_PRIVATE,						AC3_AUDIO	},
	// DTS Audio
	{ AUDIO_STREAM_DTS,					DTS_AUDIO	},
	{ AUDIO_STREAM_DTS_HD,				DTS_AUDIO	},
	{ AUDIO_STREAM_DTS_HD_MASTER_AUDIO,	DTS_AUDIO	},
	{ SECONDARY_AUDIO_DTS_HD,			DTS_AUDIO	},
	// LPCM Audio
	{ AUDIO_STREAM_LPCM,				LPCM_AUDIO	},
	// MPEG2 Video
	{ VIDEO_STREAM_MPEG2,				MPEG2_VIDEO	},
	// H.264/AVC1 Video
	{ VIDEO_STREAM_H264,				H264_VIDEO	},
	// VC-1 Video
	{ VIDEO_STREAM_VC1,					VC1_VIDEO	},
	// Dirac Video
	{ VIDEO_STREAM_DIRAC,				DIRAC_VIDEO	},
	// H.265/HEVC Video
	{ VIDEO_STREAM_HEVC,				HEVC_VIDEO	},
	{ PES_PRIVATE,						HEVC_VIDEO	},
	// PGS Subtitle
	{ PRESENTATION_GRAPHICS_STREAM,		PGS_SUB		},
	// DVB Subtitle
	{ PES_PRIVATE,						DVB_SUB		}
};

DWORD CMpegSplitterFile::AddStream(WORD pid, BYTE pesid, BYTE ps1id, DWORD len)
{
	if (pid) {
		if (pesid) {
			m_pid2pes[pid] = pesid;
		} else {
			m_pid2pes.Lookup(pid, pesid);
		}
	}

	stream s;
	s.pid	= pid;
	s.pesid	= pesid;
	s.ps1id	= ps1id;

	BYTE nPid = pid ? pid : pesid;

	const __int64 start		= GetPos();
	enum stream_type type	= stream_type::unknown;

	ULONGLONG stream_type			= PES_STREAM_TYPE_ANY;
	PES_STREAM_TYPE pes_stream_type	= INVALID;
	if (GetStreamType(s.pid, pes_stream_type)) {
		stream_type = 0ULL;

		for (size_t i = 0; i < _countof(PES_types); i++) {
			if (PES_types[i].pes_stream_type == pes_stream_type) {
				stream_type |= PES_types[i].stream_type;
			}
		}
	}

	if (pesid >= 0xe0 && pesid < 0xf0) { // mpeg video
		// MPEG2
		if (type == stream_type::unknown && (stream_type & MPEG2_VIDEO)) {
			// Sequence/extension header can be split into multiple packets
			if (!m_streams[stream_type::video].Find(s)) {
				if (!seqh.Lookup(nPid)) {
					seqh[nPid].Init();
				}

				if (seqh[nPid].data.GetCount()) {
					if (seqh[nPid].data.GetCount() < 512) {
						size_t size = seqh[nPid].data.GetCount();
						seqh[nPid].data.SetCount(size + (size_t)len);
						ByteRead(seqh[nPid].data.GetData() + size, len);
					} else {
						seqhdr h;
						if (Read(h, seqh[nPid].data, &s.mt)) {
							type = stream_type::video;
						}
					}
				} else {
					CMediaType mt;
					if (Read(seqh[nPid], len, &mt)) {
						Seek(start);
						seqh[nPid].data.SetCount((size_t)len);
						ByteRead(seqh[nPid].data.GetData(), len);
					}
				}
			}
		}

		// AVC/H.264
		if (type == stream_type::unknown && (stream_type & H264_VIDEO)) {
			Seek(start);
			// PPS and SPS can be present on differents packets
			// and can also be split into multiple packets
			if (!avch.Lookup(nPid)) {
				memset(&avch[nPid], 0, sizeof(avchdr));
			}

			if (!m_streams[stream_type::video].Find(s) && !m_streams[stream_type::stereo].Find(s) && Read(avch[nPid], len, &s.mt)) {
				if (avch[nPid].spspps[index_subsetsps].complete) {
					type = stream_type::stereo;
				} else {
					type = stream_type::video;
				}
			}
		}

		// HEVC/H.265
		if (type == stream_type::unknown && (stream_type & HEVC_VIDEO)) {
			Seek(start);
			hevchdr h;
			if (!m_streams[stream_type::video].Find(s) && Read(h, len, &s.mt)) {
				type = stream_type::video;
			}
		}
	} else if (pesid >= 0xc0 && pesid < 0xe0) { // mpeg audio

		// AAC_LATM
		if (type == stream_type::unknown && (stream_type & AAC_AUDIO)) {
			Seek(start);
			latm_aachdr h = { 0 };

			if (!m_streams[stream_type::audio].Find(s)) {
				if (Read(h, len, &s.mt) && m_type == mpeg_ts) {
					if (m_aaclatmValid[nPid].IsValid()) {
						type = stream_type::audio;
					} else {
						m_aaclatmValid[nPid].Handle(h);
					}
				}
			}
		}

		// AAC
		if (type == stream_type::unknown && (stream_type & AAC_AUDIO)) {
			Seek(start);
			aachdr h = { 0 };

			if (!m_streams[stream_type::audio].Find(s)) {
				if (Read(h, len, &s.mt)) {
					if (m_aacValid[nPid].IsValid()) {
						type = stream_type::audio;
					} else {
						m_aacValid[nPid].Handle(h);
					}
				}
			}
		}

		// MPEG Audio
		if (type == stream_type::unknown && (stream_type & MPEG_AUDIO)) {
			Seek(start);
			mpahdr h = { 0 };

			if (!m_streams[stream_type::audio].Find(s)) {
				if (Read(h, len, &s.mt)) {
					if (m_mpaValid[nPid].IsValid()) {
						type = stream_type::audio;
					} else {
						m_mpaValid[nPid].Handle(h);
					}
				}
			}
		}

		// AC3
		if (type == stream_type::unknown && (stream_type & AC3_AUDIO)) {
			Seek(start);
			ac3hdr h = { 0 };

			if (!m_streams[stream_type::audio].Find(s)) {
				if (Read(h, len, &s.mt)) {
					if (m_ac3Valid[nPid].IsValid()) {
						type = stream_type::audio;
					} else {
						m_ac3Valid[nPid].Handle(h);
					}
				}
			}
		}
	} else if (pesid == 0xbd || pesid == 0xfd) { // private stream 1
		if (s.pid) {
			if (!m_streams[stream_type::audio].Find(s) && !m_streams[stream_type::video].Find(s) && !m_streams[stream_type::subpic].Find(s)) {

				// AC3, E-AC3, TrueHD
				if (type == stream_type::unknown && (stream_type & AC3_AUDIO)) {
					ac3hdr h;
					if (Read(h, len, &s.mt, true, (m_AC3CoreOnly == 1))) {
						type = stream_type::audio;
					}
				}

				// DTS, DTS HD, DTS HD MA
				if (type == stream_type::unknown && (stream_type & DTS_AUDIO)) {
					Seek(start);
					dtshdr h;
					if (Read(h, len, &s.mt, false)) {
						type = stream_type::audio;
					}
				}

				// VC1
				if (type == stream_type::unknown && (stream_type & VC1_VIDEO)) {
					Seek(start);
					vc1hdr h;
					if (Read(h, len, &s.mt)) {
						type = stream_type::video;
					}
				}

				// DIRAC
				if (type == stream_type::unknown && (stream_type & DIRAC_VIDEO)) {
					Seek(start);
					dirachdr h;
					if (Read(h, len, &s.mt)) {
						type = stream_type::video;
					}
				}

				// DVB subtitles
				if (type == stream_type::unknown && (stream_type & DVB_SUB)) {
					Seek(start);
					dvbsub h;
					if (Read(h, len, &s.mt)) {
						type = stream_type::subpic;
					}
				}

				// LPCM Audio
				if (type == stream_type::unknown && (stream_type & LPCM_AUDIO)) {
					Seek(start);
					hdmvlpcmhdr h;
					if (Read(h, &s.mt)) {
						type = stream_type::audio;
					}
				}

				// PGS subtitles
				if (type == stream_type::unknown && (stream_type & PGS_SUB)) {
					Seek(start);

					int iProgram;
					const CHdmvClipInfo::Stream *pClipInfo;
					const program* pProgram = FindProgram(s.pid, iProgram, pClipInfo);

					hdmvsubhdr h;
					if (!m_ClipInfo.IsHdmv() && Read(h, &s.mt, pClipInfo ? pClipInfo->m_LanguageCode : NULL)) {
						type = stream_type::subpic;
					}
				}
			} else if ((m_AC3CoreOnly != 1) && m_init) {
				int iProgram;
				const CHdmvClipInfo::Stream *pClipInfo;
				const program* pProgram = FindProgram(s.pid, iProgram, pClipInfo);
				if ((type == stream_type::unknown) && (pProgram != NULL) && AUDIO_STREAM_AC3_TRUE_HD == pProgram->streams[iProgram].type) {
					const stream* source = m_streams[stream_type::audio].FindStream(s.pid);
					if (source && source->mt.subtype == MEDIASUBTYPE_DOLBY_AC3) {
						ac3hdr h;
						if (Read(h, len, &s.mt, false, (m_AC3CoreOnly == 1)) && s.mt.subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
							m_streams[stream_type::audio].Replace((stream&)*source, s);
						}
					}
				}
			}
		} else if (pesid == 0xfd) {
			vc1hdr h;
			if (!m_streams[stream_type::video].Find(s) && Read(h, len, &s.mt)) {
				type = stream_type::video;
			}
		} else {
			BYTE b		= (BYTE)BitRead(8, true);
			WORD w		= (WORD)BitRead(16, true);
			DWORD dw	= (DWORD)BitRead(32, true);

			if (b >= 0x80 && b < 0x88 || w == 0x0b77) { // ac3
				s.ps1id = (b >= 0x80 && b < 0x88) ? (BYTE)(BitRead(32) >> 24) : 0x80;

				ac3hdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (b >= 0x88 && b < 0x90 || dw == 0x7ffe8001) { // dts
				s.ps1id = (b >= 0x88 && b < 0x90) ? (BYTE)(BitRead(32) >> 24) : 0x88;

				dtshdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (b >= 0xa0 && b < 0xa8) { // lpcm
				s.ps1id = (BYTE)BitRead(8);
				
				do {
					// DVD-Audio LPCM
					if (b == 0xa0) {
						BitRead(8); // Continuity Counter - counts from 0x00 to 0x1f and then wraps to 0x00.
						DWORD headersize = (DWORD)BitRead(16); // LPCM_header_length
						if (headersize >= 8 && headersize+4 < len) {
							dvdalpcmhdr h;
							if (Read(h, len-4, &s.mt)) {
								Seek(start + 4 + headersize);
								type = stream_type::audio;
								break;
							}
						}
					}
					// DVD-Audio MLP
					else if (b == 0xa1 && len > 10) {
						BYTE counter = (BYTE)BitRead(8); // Continuity Counter: 0x00..0x1f or 0x20..0x3f or 0x40..0x5f
						BitRead(8); // some unknown data
						DWORD headersize = (DWORD)BitRead(8); // MLP_header_length (always equal 6?)
						BitRead(32); // some unknown data
						WORD unknown1 = (WORD)BitRead(16); // 0x0000 or 0x0400
						if (counter <= 0x5f && headersize == 6 && (unknown1 == 0x0000 || unknown1 == 0x0400)) { // Maybe it's MLP?
							// MLP header may be missing in the first package
							mlphdr h;
							if (!m_streams[stream_type::audio].Find(s) && Read(h, len-10, &s.mt, true)) {
								// This is exactly the MLP.
								Seek(start + 10);
								type = stream_type::audio;
							}
							Seek(start + 10);
							break; 
						}
					}

					// DVD LPCM
					if (m_streams[stream_type::audio].Find(s)) {
						Seek(start + 7);
					} else {
						Seek(start + 4);
						lpcmhdr h;
						if (Read(h, &s.mt)) {
							type = stream_type::audio;
						}
					}
				} while (false);
			} else if (b >= 0x20 && b < 0x40) { // DVD subpic
				s.ps1id = (BYTE)BitRead(8);

				dvdspuhdr h;
				if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
					type = stream_type::subpic;
				}
			} else if (b >= 0x70 && b < 0x80) { // SVCD subpic
				s.ps1id = (BYTE)BitRead(8);

				svcdspuhdr h;
				if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
					type = stream_type::subpic;
				}
			} else if (b >= 0x00 && b < 0x10) { // CVD subpic
				s.ps1id = (BYTE)BitRead(8);

				cvdspuhdr h;
				if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
					type = stream_type::subpic;
				}
			} else if (w == 0xffa0 || w == 0xffa1) { // ps2-mpg audio
				s.ps1id = (BYTE)BitRead(8);
				s.pid = (WORD)((BitRead(8) << 8) | BitRead(16)); // pid = 0xa000 | track id

				ps2audhdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (w == 0xff90) { // ps2-mpg ac3 or subtitles
				s.ps1id = (BYTE)BitRead(8);
				s.pid = (WORD)((BitRead(8) << 8) | BitRead(16)); // pid = 0x9000 | track id

				w = (WORD)BitRead(16, true);

				if (w == 0x0b77) {
					ac3hdr h;
					if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt)) {
						type = stream_type::audio;
					}
				} else if (w == 0x0000) { // usually zero...
					ps2subhdr h;
					if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt)) {
						type = stream_type::subpic;
					}
				}
			} else if (b >= 0xc0 && b < 0xcf) { // dolby digital/dolby digital +
				s.ps1id = (BYTE)BitRead(8);
				// skip audio header - 3-byte
				BitRead(24);
				ac3hdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt)) {
					type = stream_type::audio;
				}
			} else if (b >= 0xb0 && b < 0xbf) { // truehd
				s.ps1id = (BYTE)BitRead(8);
				// TrueHD audio has a 4-byte header
				BitRead(32);
				ac3hdr h;
				if (!m_streams[stream_type::audio].Find(s) && Read(h, len, &s.mt, false, false)) {
					type = stream_type::audio;
				}
			}
		}
	} else if (pesid == 0xbe) { // padding
	} else if (pesid == 0xbf) { // private stream 2
	}

	if (type != stream_type::unknown && !m_streams[type].Find(s)) {
		if (s.pid) {
			if (!s.lang_set) {
				CStringA lang_name;
				m_pPMT_Lang.Lookup(s.pid, lang_name);
				if (!lang_name.IsEmpty()) {
					memcpy(s.lang, lang_name, 4);
					s.lang_set = true;
				}
			}

			for (int t = stream_type::video; t < stream_type::unknown; t++) {
				if (m_streams[t].Find(s)) {
					return s;
				}
			}
		}

		if (type == stream_type::video) {
			REFERENCE_TIME rtAvgTimePerFrame = 1;
			ExtractAvgTimePerFrame(&s.mt, rtAvgTimePerFrame);

			if (rtAvgTimePerFrame == 1) {
				__int64 _pos = GetPos();
				REFERENCE_TIME rt_start = NextPTS(s.pid);
				if (rt_start != INVALID_TIME) {
					Seek(GetPos() + 188);
					REFERENCE_TIME rt_end = rt_start;
					int count = 0;
					while (count < 30) {
						REFERENCE_TIME rt = NextPTS(s.pid);
						if (rt == INVALID_TIME) {
							break;
						}
						if (rt > rt_start) {
							rt_end = rt;
							count++;
						}
						Seek(GetPos() + 188);
					}

					if (count && (rt_start < rt_end)) {
						rtAvgTimePerFrame = (rt_end - rt_start) / (count - 1);
					}

					if (rtAvgTimePerFrame) {
						if (s.mt.formattype == FORMAT_MPEG2_VIDEO) {
							((MPEG2VIDEOINFO*)s.mt.pbFormat)->hdr.AvgTimePerFrame	= rtAvgTimePerFrame;
						} else if (s.mt.formattype == FORMAT_VIDEOINFO2) {
							((VIDEOINFOHEADER2*)s.mt.pbFormat)->AvgTimePerFrame		= rtAvgTimePerFrame;
						} else if (s.mt.formattype == FORMAT_VideoInfo) {
							((VIDEOINFOHEADER*)s.mt.pbFormat)->AvgTimePerFrame		= rtAvgTimePerFrame;
						}
					}
				}

				Seek(_pos);
			}
		}

		s.mts.push_back(s.mt);
		m_streams[type].Insert(s, type);
	}

	return s;
}

void CMpegSplitterFile::AddHdmvPGStream(WORD pid, const char* language_code)
{
	stream s;

	s.pid		= pid;
	s.pesid		= 0xbd;

	hdmvsubhdr h;
	if (!m_streams[stream_type::subpic].Find(s) && Read(h, &s.mt, language_code)) {
		s.mts.push_back(s.mt);
		m_streams[stream_type::subpic].Insert(s, subpic);
	}
}

CAtlList<CMpegSplitterFile::stream>* CMpegSplitterFile::GetMasterStream()
{
	return
		!m_streams[stream_type::video].IsEmpty()	? &m_streams[stream_type::video]  :
		!m_streams[stream_type::audio].IsEmpty()	? &m_streams[stream_type::audio]  :
		!m_streams[stream_type::subpic].IsEmpty()	? &m_streams[stream_type::subpic] :
		!m_streams[stream_type::stereo].IsEmpty()	? &m_streams[stream_type::stereo] :
		NULL;
}

void CMpegSplitterFile::UpdatePrograms(const trhdr& h, bool UpdateLang)
{
	CAutoLock cAutoLock(&m_csProps);

	if (h.payload && h.payloadstart && h.pid == 0) {
		trsechdr h2;
		if (Read(h2) && h2.table_id == 0) {
			CAtlMap<WORD, bool> newprograms;

			int len = h2.section_length;
			len -= 5+4;

			for (int i = len/4; i > 0; i--) {
				WORD program_number = (WORD)BitRead(16);
				BYTE reserved = (BYTE)BitRead(3);
				WORD pid = (WORD)BitRead(13);
				UNREFERENCED_PARAMETER(reserved);
				if (program_number != 0) {
					m_programs[pid].program_number = program_number;
					newprograms[program_number] = true;
				}
			}

			POSITION pos = m_programs.GetStartPosition();
			while (pos) {
				const CAtlMap<WORD, program>::CPair* pPair = m_programs.GetNext(pos);

				if (!newprograms.Lookup(pPair->m_value.program_number)) {
					m_programs.RemoveKey(pPair->m_key);
				}
			}
		}
	} else if (CAtlMap<WORD, program>::CPair* pPair = m_programs.Lookup(h.pid)) {
		if (h.payload && h.payloadstart) {
			trsechdr h2;
			if (Read(h2) && h2.table_id == 2) {
				int len = h2.section_length;
				len -= 5+4;

				if (len) {
					BYTE* buffer = DNew BYTE[len];
					ByteRead(buffer, len);

					int max_len = h.bytes - 9;

					if (len > max_len) {
						memset(pPair->m_value.ts_buffer, 0, sizeof(pPair->m_value.ts_buffer));
						pPair->m_value.ts_len_cur = max_len;
						pPair->m_value.ts_len_packet = len;
						memcpy(pPair->m_value.ts_buffer, buffer, max_len);
					} else {
						CGolombBuffer gb(buffer, len);
						UpdatePrograms(gb, h.pid, UpdateLang);
					}

					delete [] buffer;
				}
			}
		} else {
			if (pPair->m_value.ts_len_cur > 0) {
				int len = pPair->m_value.ts_len_packet - pPair->m_value.ts_len_cur;
				if (len > h.bytes) {
					ByteRead(pPair->m_value.ts_buffer + pPair->m_value.ts_len_cur, h.bytes);
					pPair->m_value.ts_len_cur += h.bytes;
				} else {
					ByteRead(pPair->m_value.ts_buffer + pPair->m_value.ts_len_cur, pPair->m_value.ts_len_packet - pPair->m_value.ts_len_cur);
					CGolombBuffer gb(pPair->m_value.ts_buffer, pPair->m_value.ts_len_packet);
					UpdatePrograms(gb, h.pid, UpdateLang);
				}
			}
		}
	}
}

void CMpegSplitterFile::UpdatePrograms(CGolombBuffer gb, WORD pid, bool UpdateLang)
{
	if (CAtlMap<WORD, program>::CPair* pPair = m_programs.Lookup(pid))
	{
		memset(pPair->m_value.streams, 0, sizeof(pPair->m_value.streams));

		int len = gb.GetSize();

		BYTE reserved1				= (BYTE)gb.BitRead(3);
		WORD PCR_PID				= (WORD)gb.BitRead(13);
		BYTE reserved2				= (BYTE)gb.BitRead(4);
		WORD program_info_length	= (WORD)gb.BitRead(12);
		UNREFERENCED_PARAMETER(reserved1);
		UNREFERENCED_PARAMETER(PCR_PID);
		UNREFERENCED_PARAMETER(reserved2);

		len -= (4 + program_info_length);
		if (len <= 0)
			return;

		while (program_info_length-- > 0) {
			gb.BitRead(8);
		}

		for (int i = 0; i < _countof(pPair->m_value.streams) && len >= 5; i++) {
			BYTE stream_type	= (BYTE)gb.BitRead(8);
			BYTE nreserved1		= (BYTE)gb.BitRead(3);
			WORD pid			= (WORD)gb.BitRead(13);
			BYTE nreserved2		= (BYTE)gb.BitRead(4);
			WORD ES_info_length	= (WORD)gb.BitRead(12);
			UNREFERENCED_PARAMETER(nreserved1);
			UNREFERENCED_PARAMETER(nreserved2);

			pPair->m_value.streams[i].pid	= pid;
			pPair->m_value.streams[i].type	= (PES_STREAM_TYPE)stream_type;

			len -= (5 + ES_info_length);
			if (len < 0)
				break;
			if (ES_info_length<=2)
				continue;

			if (UpdateLang) {
				int	info_length = ES_info_length;
				for (;;) {
					BYTE descriptor_tag		= (BYTE)gb.BitRead(8);
					BYTE descriptor_length	= (BYTE)gb.BitRead(8);
					info_length -= (2 + descriptor_length);
					if (info_length < 0)
						break;
					char ch[4];
					switch (descriptor_tag) {
						case 0x0a: // ISO 639 language descriptor
						case 0x56: // Teletext descriptor
						case 0x59: // Subtitling descriptor
							gb.ReadBuffer((BYTE *)ch, 3);
							ch[3] = 0;
							for (int i = 3; i < descriptor_length; i++) {
								gb.BitRead(8);
							}
							if (!(ch[0] == 'u' && ch[1] == 'n' && ch[2] == 'd')) {
								m_pPMT_Lang[pid] = CString(ch);
							}
							break;
						default:
							for (int i = 0; i < descriptor_length; i++) {
								gb.BitRead(8);
							}
							break;
					}
					if (info_length<=2) break;
				}
			} else {
				while (ES_info_length-- > 0) {
					gb.BitRead(8);
				}
			}

			if (m_ForcedSub) {
				if (stream_type == PRESENTATION_GRAPHICS_STREAM) {
					stream s;
					s.pid = pid;
					
					CStringA lang_name;
					m_pPMT_Lang.Lookup(s.pid, lang_name);
					if (!lang_name.IsEmpty()) {
						memcpy(s.lang, lang_name, 4);
						s.lang_set = true;
					}

					CMpegSplitterFile::hdmvsubhdr hdr;
					if (Read(hdr, &s.mt, NULL)) {
						if (!m_streams[stream_type::subpic].Find(s)) {
							s.mts.push_back(s.mt);
							m_streams[stream_type::subpic].Insert(s, subpic);
						}
					}
				}
			}
		}
		pPair->m_value.ts_len_cur		= 0;
		pPair->m_value.ts_len_packet	= 0;
	}
}

const CMpegSplitterFile::program* CMpegSplitterFile::FindProgram(WORD pid, int &iStream, const CHdmvClipInfo::Stream * &_pClipInfo)
{
	iStream		= -1;
	_pClipInfo	= NULL;

	if (m_type == mpeg_ts) {
		_pClipInfo = m_ClipInfo.FindStream(pid);

		POSITION pos = m_programs.GetStartPosition();
		while (pos) {
			program* p = &m_programs.GetNextValue(pos);

			for (int i = 0; i < _countof(p->streams); i++) {
				if (p->streams[i].pid == pid) {
					iStream = i;
					return p;
				}
			}
		}
	}

	return NULL;
}

bool CMpegSplitterFile::GetStreamType(WORD pid, PES_STREAM_TYPE &stream_type)
{
	if (ISVALIDPID(pid)) {
		int iProgram;
		const CHdmvClipInfo::Stream *pClipInfo;
		const program* pProgram = FindProgram(pid, iProgram, pClipInfo);
		if (pProgram) {
			stream_type = pProgram->streams[iProgram].type;

			if (stream_type != INVALID) {
				return true;
			}
		}
	}

	return false;
}

void CMpegSplitterFile::UpdatePSM()
{
	WORD psm_length		= (WORD)BitRead(16);
	BitRead(8);
	BitRead(8);
	WORD ps_info_length	= (WORD)BitRead(16);
	while (ps_info_length-- > 0) {
		BitRead(1);
	}
	WORD es_map_length	= (WORD)BitRead(16);

	while (es_map_length > 4) {
		BYTE type			= (BYTE)BitRead(8);
		BYTE es_id			= (BYTE)BitRead(8);
		WORD es_info_length	= (WORD)BitRead(16);

		m_psm[es_id]		= (PES_STREAM_TYPE)type;
        
		es_map_length		-= 4 + min(es_info_length, es_map_length - 4);
		while (es_info_length-- > 0) {
			BitRead(1);
		}
	}

	BitRead(32);
}