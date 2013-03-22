/*
 * $Id$
 *
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

#pragma once

#include <d3dx9.h>
#include <videoacc.h>	// DXVA1
#include <dxva.h>
#include <dxva2api.h>	// DXVA2
#include "../BaseVideoFilter/BaseVideoFilter.h"
#include "IMPCVideoDecFilter.h"
#include "MPCVideoDecSettingsWnd.h"
#include "DXVADecoder.h"
#include "H264RandomAccess.h"
#include <atlpath.h>
#include "../../../apps/mplayerc/FilterEnum.h"

#define MPCVideoDecName L"MPC Video Decoder"

#define CHECK_HR(x)			hr = ##x; if (FAILED(hr)) { TRACE("Error : 0x%08x, %s:%i\n", hr, __FILE__, __LINE__); return hr; }
#define CHECK_HR_FALSE(x)	hr = ##x; if (FAILED(hr)) { TRACE("Error : 0x%08x, %s:%i\n", hr, __FILE__, __LINE__); return S_FALSE; }

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
struct AVFrame;
struct SwsContext;

class CCpuId;

typedef enum {
	MODE_SOFTWARE,
	MODE_DXVA1,
	MODE_DXVA2
} DXVA_MODE;

typedef struct {
	REFERENCE_TIME	rtStart;
	REFERENCE_TIME	rtStop;
} B_FRAME;

typedef struct {
	bool	video_after_seek;
	__int32	kf_pts;		///< timestamp of next video keyframe
	__int64	kf_base;	///< timestamp of the prev. video keyframe
} RMDemuxContext;

class __declspec(uuid("008BAC12-FBAF-497b-9670-BC6F6FBAE2C4"))
	CMPCVideoDecFilter
	: public CBaseVideoFilter
	, public ISpecifyPropertyPages2
	, public IMPCVideoDecFilter
	, public IMPCVideoDecFilter2
	, public IMPCVideoDecFilterCodec
{
protected:

	friend class CVideoDecDXVAAllocator;

	CCpuId*									m_pCpuId;
	CCritSec								m_csProps;

	bool									m_FFmpegFilters[FFM_LAST + !FFM_LAST];
	bool									m_DXVAFilters[TRA_DXVA_LAST + !TRA_DXVA_LAST];

	// === Persistants parameters (registry)
	int										m_nThreadNumber;
	int										m_nDiscardMode;
	int										m_nErrorRecognition;
	int										m_nIDCTAlgo;
	MPC_DEINTERLACING_FLAGS					m_nDeinterlacing;
	bool									m_bDXVACompatible;
	DWORD									m_nActiveCodecs;
	int										m_nARMode;
	int										m_nDXVACheckCompatibility;
	int										m_nDXVA_SD;

	// === New swscaler options
	int										m_nSwRefresh;
	int										m_nSwOutputFormats;
	int										m_nSwChromaToRGB;
	int										m_nSwResizeMethodBE;
	int										m_nSwColorspace;
	int										m_nSwInputLevels;
	int										m_nSwOutputLevels;
	//

	enum AVPixelFormat						m_PixFmt;

	FF_FIELD_TYPE							m_nFrameType;

	// === FFMpeg variables
	AVCodec*								m_pAVCodec;
	AVCodecContext*							m_pAVCtx;
	AVCodecParserContext*					m_pParser;
	AVFrame*								m_pFrame;
	int										m_nCodecNb;
	enum AVCodecID							m_nCodecId;
	int										m_nWorkaroundBug;
	int										m_nErrorConcealment;
	REFERENCE_TIME							m_rtAvrTimePerFrame;
	bool									m_bReorderBFrame;
	B_FRAME									m_BFrames[2];
	int										m_nPosB;
	int										m_nOutputWidth;
	int										m_nOutputHeight;

	bool									m_bIsEVO;

	// Buffer management for truncated stream (store stream chunks & reference time sent by splitter)
	BYTE*									m_pFFBuffer;
	int										m_nFFBufferSize;
	BYTE*									m_pFFBuffer2;
	int										m_nFFBufferSize2;
	BYTE*									m_pAlignedFFBuffer;
	int										m_nAlignedFFBufferSize;

	REFERENCE_TIME							m_rtLastStop;			// rtStop for last delivered frame
	double									m_dRate;
	REFERENCE_TIME							m_rtPrevStop;

	bool									m_bUseDXVA;
	bool									m_bUseFFmpeg;
	SwsContext*								m_pSwsContext;
	unsigned __int64						m_nOutCsp;
	CSize									m_pOutSize;				// Picture size on output pin
	int										m_nSwOutBpp;

	// === DXVA common variables
	VIDEO_OUTPUT_FORMATS*					m_pVideoOutputFormat;
	int										m_nVideoOutputCount;
	DXVA_MODE								m_nDXVAMode;
	CDXVADecoder*							m_pDXVADecoder;
	GUID									m_DXVADecoderGUID;

	DWORD									m_nPCIVendor;
	DWORD									m_nPCIDevice;
	LARGE_INTEGER							m_VideoDriverVersion;
	CString									m_strDeviceDescription;

	// === DXVA1 variables
	DDPIXELFORMAT							m_PixelFormat;

	// === DXVA2 variables
	CComPtr<IDirect3DDeviceManager9>		m_pDeviceManager;
	CComPtr<IDirectXVideoDecoderService>	m_pDecoderService;
	CComPtr<IDirect3DSurface9>				m_pDecoderRenderTarget;
	DXVA2_ConfigPictureDecode				m_DXVA2Config;
	HANDLE									m_hDevice;
	DXVA2_VideoDesc							m_VideoDesc;

	CH264RandomAccess						m_h264RandomAccess;

	BOOL									m_bWaitingForKeyFrame;

	RMDemuxContext							rm;
	REFERENCE_TIME							m_rtStart;

	HWND									m_nDialogHWND;

	REFERENCE_TIME							m_rtStartCache;

	// === Private functions
	void				Cleanup();
	void				ffmpegCleanup();
	int					FindCodec(const CMediaType* mtIn);
	void				AllocExtradata(AVCodecContext* pAVCtx, const CMediaType* mt);
	void				GetOutputFormats (int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats);
	void				DetectVideoCard(HWND hWnd);
	unsigned __int64	GetCspFromMediaType(GUID& subtype);
	void				InitSwscale();

	void				SetTypeSpecificFlags(IMediaSample* pMS);
	HRESULT				SoftwareDecode(IMediaSample* pIn, BYTE* pDataIn, int nSize, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);

	HRESULT				ReopenVideo();

public:

	const static AMOVIESETUP_MEDIATYPE		sudPinTypesIn[];
	const static int						sudPinTypesInCount;
	const static AMOVIESETUP_MEDIATYPE		sudPinTypesOut[];
	const static int						sudPinTypesOutCount;

	CMPCVideoDecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMPCVideoDecFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP			NonDelegatingQueryInterface(REFIID riid, void** ppv);
	virtual bool			IsVideoInterlaced();
	virtual void			GetOutputSize(int& w, int& h, int& arx, int& ary, int& RealWidth, int& RealHeight);
	CTransformOutputPin*	GetOutputPin() {
		return m_pOutput;
	}

	REFERENCE_TIME	GetDuration();
	void			UpdateFrameTime (REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop, bool pulldown_flag = false);
	bool			IsAVI();

	// === Overriden DirectShow functions
	HRESULT			SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);
	HRESULT			CheckInputType(const CMediaType* mtIn);
	HRESULT			Transform(IMediaSample* pIn);
	HRESULT			CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	HRESULT			DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT			BeginFlush();
	HRESULT			EndFlush();
	HRESULT			NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate);
	HRESULT			EndOfStream();

	HRESULT			BreakConnect(PIN_DIRECTION dir);


	// === ISpecifyPropertyPages2

	STDMETHODIMP	GetPages(CAUUID* pPages);
	STDMETHODIMP	CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// === IMPCVideoDecFilter
	STDMETHODIMP Apply();
	STDMETHODIMP SetThreadNumber(int nValue);
	STDMETHODIMP_(int) GetThreadNumber();
	STDMETHOD(SetDiscardMode(int nValue));
	STDMETHOD_(int, GetDiscardMode());
	STDMETHOD(SetErrorRecognition(int nValue));
	STDMETHOD_(int, GetErrorRecognition());
	STDMETHOD(SetIDCTAlgo(int nValue));
	STDMETHOD_(int, GetIDCTAlgo());
	STDMETHOD(SetDeinterlacing(MPC_DEINTERLACING_FLAGS nValue));
	STDMETHOD_(MPC_DEINTERLACING_FLAGS, GetDeinterlacing());
	STDMETHOD_(GUID*, GetDXVADecoderGuid());
	STDMETHOD(SetActiveCodecs(DWORD nValue));
	STDMETHOD_(DWORD, GetActiveCodecs());
	STDMETHODIMP_(LPCTSTR) GetVideoCardDescription();

	STDMETHOD(SetARMode(int nValue));
	STDMETHOD_(int, GetARMode());

	STDMETHOD(SetDXVACheckCompatibility(int nValue));
	STDMETHOD_(int, GetDXVACheckCompatibility());

	STDMETHOD(SetDXVA_SD(int nValue));
	STDMETHOD_(int, GetDXVA_SD());

	// === New swscaler options
	STDMETHOD(SetSwRefresh(int nValue));
	STDMETHOD(SetSwOutputFormats(int nValue));
	STDMETHOD_(int, GetSwOutputFormats());
	STDMETHOD(SetSwChromaToRGB(int nValue));
	STDMETHOD_(int, GetSwChromaToRGB());
	STDMETHOD(SetSwResizeMethodBE(int nValue));
	STDMETHOD_(int, GetSwResizeMethodBE());
	STDMETHOD(SetSwColorspace(int nValue));
	STDMETHOD_(int, GetSwColorspace());
	STDMETHOD(SetSwInputLevels(int nValue));
	STDMETHOD_(int, GetSwInputLevels());
	STDMETHOD(SetSwOutputLevels(int nValue));
	STDMETHOD_(int, GetSwOutputLevels());
	//

	STDMETHOD(SetDialogHWND(HWND nValue));
	STDMETHOD_(unsigned __int64, GetOutputFormat());

	// === IMPCVideoDecFilter2
	STDMETHOD_(int, GetFrameType());

	// === IMPCVideoDecFilterCodec
	STDMETHOD(SetFFMpegCodec(int nCodec, bool bEnabled));
	STDMETHOD(SetDXVACodec(int nCodec, bool bEnabled));

	// === DXVA common functions
	BOOL						IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered);
	BOOL						IsSupportedDecoderMode(const GUID& mode);
	void						BuildDXVAOutputFormat();
	int							GetPicEntryNumber();
	int							PictWidth();
	int							PictHeight();
	int							PictWidthRounded();
	int							PictHeightRounded();

	inline bool					UseDXVA2()				{ return (m_nDXVAMode == MODE_DXVA2); };
	inline AVCodecContext*		GetAVCtx()				{ return m_pAVCtx; };
	inline AVFrame*				GetFrame()				{ return m_pFrame; };
	inline enum AVCodecID			GetCodec()				{ return m_nCodecId; };
	inline bool					IsReorderBFrame()		{ return m_bReorderBFrame; };
	inline bool					IsEvo()					{ return m_bIsEVO; };
	inline DWORD				GetPCIVendor()			{ return m_nPCIVendor; };
	inline double				GetRate()				{ return m_dRate; };
	bool						IsDXVASupported();
	void						UpdateAspectRatio();
	void						ReorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	void						FlushDXVADecoder()	{
		if (m_pDXVADecoder) {
			m_pDXVADecoder->Flush();
		}
	}

	void						SetFrameType(FF_FIELD_TYPE nFrameType);

	// === DXVA1 functions
	DDPIXELFORMAT*				GetPixelFormat()		{ return &m_PixelFormat; }
	HRESULT						FindDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat);
	HRESULT						CheckDXVA1Decoder(const GUID *pGuid);
	void						SetDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat);
	WORD						GetDXVA1RestrictedMode();
	HRESULT						CreateDXVA1Decoder(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount);


	// === DXVA2 functions
	void						FillInVideoDescription(DXVA2_VideoDesc *pDesc);
	DXVA2_ConfigPictureDecode*	GetDXVA2Config()		{ return &m_DXVA2Config; };
	HRESULT						ConfigureDXVA2(IPin *pPin);
	HRESULT						SetEVRForDXVA2(IPin *pPin);
	HRESULT						FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
															  const GUID& guidDecoder,
															  DXVA2_ConfigPictureDecode *pSelectedConfig,
															  BOOL *pbFoundDXVA2Configuration);
	HRESULT						CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets);

	// === EVR functions
	HRESULT						DetectVideoCard_EVR(IPin *pPin);
};
