
// Source code is provided by Deng James from Strongene Ltd.

class NALBitstream
{
public:
	NALBitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) {};
	NALBitstream(void * data, int len) { Init(data, len); };
	void Init(void * data, int len) { m_data = (LPBYTE)data; m_len = len; m_idx = 0; m_bits = 0; m_byte = 0; m_zeros = 0; };

	BYTE GetBYTE() {
		if ( m_idx >= m_len )
			return 0;
		BYTE b = m_data[m_idx++];
		// to avoid start-code emulation, a byte 0x03 is inserted
		// after any 00 00 pair. Discard that here.
		if ( b == 0 ) {
			m_zeros++;
			if ( (m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03) ) {
				m_idx++;
				m_zeros=0;
			}
		} else {
			m_zeros = 0;
		}
		return b;
	};
	UINT32 GetBit() {
		if (m_bits == 0) {
			m_byte = GetBYTE();
			m_bits = 8;
		}
		m_bits--;
		return (m_byte >> m_bits) & 0x1;
	};
	UINT32 GetWord(int bits) {
		UINT32 u = 0;
		while ( bits > 0 ) {
			u <<= 1;
			u |= GetBit();
			bits--;
		}
		return u;
	};
	UINT32 GetUE() {
		// Exp-Golomb entropy coding: leading zeros, then a one, then
		// the data bits. The number of leading zeros is the number of
		// data bits, counting up from that number of 1s as the base.
		// That is, if you see
		//      0001010
		// You have three leading zeros, so there are three data bits (010)
		// counting up from a base of 111: thus 111 + 010 = 1001 = 9
		int zeros = 0;
		while (m_idx < m_len && GetBit() == 0 ) zeros++;
		return GetWord(zeros) + ((1 << zeros) - 1);
	};
	INT32 GetSE() {
		// same as UE but signed.
		// basically the unsigned numbers are used as codes to indicate signed numbers in pairs
		// in increasing value. Thus the encoded values
		//      0, 1, 2, 3, 4
		// mean
		//      0, 1, -1, 2, -2 etc
		UINT32 UE = GetUE();
		bool positive = UE & 1;
		INT32 SE = (UE + 1) >> 1;
		if ( !positive )
		{
			SE = -SE;
		}
		return SE;
	};

private:
	LPBYTE m_data;
	int m_len;
	int m_idx;
	int m_bits;
	BYTE m_byte;
	int m_zeros;
};