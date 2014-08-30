
#pragma once

#include <atlcoll.h>

class CAudioNormalizer
{
protected:
	int m_level;
	bool m_boost;
	int m_stepping;
	int m_rising;
	int m_drangehead;
	int m_vol_level[64];
	int m_vol_head;

	CAtlArray<short> m_bufLQ;
	CAtlArray<int> m_smpLQ;

	CAtlArray<double> m_bufHQ;
	CAtlArray<double> m_smpHQ;
	double m_drange[64];
	double m_SMP;

	BYTE m_prediction[4096];
	DWORD m_predictor;
	DWORD m_pred_good;
	DWORD m_pred_bad;
	int m_vol;

	typedef struct smooth_struct
	{
		double * data;
		double max;
		int size;
		int used;
		int current;
	} smooth_t;

	smooth_t *m_smooth[10];
	double m_normalize_level;
	double m_silence_level;
	double m_max_mult;
	bool m_do_compress;
	double m_cutoff;
	double m_degree;

	void calc_power_level(short *samples, int numsamples, int nch);
	void adjust_gain(short *samples, int numsamples, int nch, double gain);
	smooth_t *SmoothNew(int size);
	void SmoothDelete(smooth_t * del);
	void SmoothAddSample(smooth_t * sm, double sample);
	double SmoothGetMax(smooth_t * sm);

	int MSteadyLQ(void *samples, int numsamples, int nch, bool IsFloat);
	int MSteadyHQ(void *samples, int numsamples, int nch, bool IsFloat);

public:
	void SetParam(int Level, bool Boost, int Steping);
	int MSteadyLQ16(short *samples, int numsamples, int nch);
	int MSteadyLQ32(float *samples, int numsamples, int nch);
	int MSteadyHQ16(short *samples, int numsamples, int nch);
	int MSteadyHQ32(float *samples, int numsamples, int nch);

	int AutoVolume(short *samples, int numsamples, int nch);

public:
	CAudioNormalizer(void);
	virtual ~CAudioNormalizer(void);
};

