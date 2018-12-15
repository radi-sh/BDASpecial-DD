#pragma once

#include "IBdaSpecials2.h"
#include <map>

#include <Ks.h>
#include <ksproxy.h>

class CDDSpecials : public IBdaSpecials2a3
{
public:
	CDDSpecials(HMODULE hMySelf, CComPtr<IBaseFilter> pTunerDevice);
	virtual ~CDDSpecials(void);

	const HRESULT InitializeHook(void);
	const HRESULT Set22KHz(bool bActive);
	const HRESULT FinalizeHook(void);

	const HRESULT GetSignalState(int *pnStrength, int *pnQuality, int *pnLock);
	const HRESULT LockChannel(BYTE bySatellite, BOOL bHorizontal, unsigned long ulFrequency, BOOL bDvbS2);
	const HRESULT SetLNBPower(bool bActive);

	const HRESULT Set22KHz(long nTone);
	const HRESULT LockChannel(const TuningParam *pTuningParm);
	const HRESULT ReadIniFile(const WCHAR *szIniFilePath);
	const HRESULT IsDecodingNeeded(BOOL *pbAns);
	const HRESULT Decode(BYTE *pBuf, DWORD dwSize);
	const HRESULT GetSignalStrength(float *fVal);
	const HRESULT PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest);
	const HRESULT PostLockChannel(const TuningParam *pTuningParm);

	virtual void Release(void);

private:
	static constexpr GUID KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR = { 0x0aa8a605, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

	enum KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR {
		KSPROPERTY_DD_BDA_SELECT_STANDARD = 0,	// Select signal standard
		KSPROPERTY_DD_BDA_SELECT_STREAM = 1,	// DVB-S2: input stream id
												// DVB-T2: 0 = Base Profile, 1 = Lite Profile
												// DVB-C2: Data Slice ID
												// ISDB-C: High Word = ONID, Low Word = TSID.  ONID == 0 -> TSID is relative, TSID == 0xFFFF -> disable TSMF
												// ISDB-S: High Word = Flag, Low Word = TSID.  Flag == 0 -> TSID is relative
		KSPROPERTY_DD_BDA_S2_ISI = KSPROPERTY_DD_BDA_SELECT_STREAM,
		KSPROPERTY_DD_BDA_S2_PLSSI = 2,			// Sets DVB-S2 physical layer scrambling sequence index (Gold code)
		KSPROPERTY_DD_BDA_S2_MODMASK = 3,		// Sets DVB-S2/S2X Modulation Mask 
		KSPROPERTY_DD_BDA_IQ_MODE = 5,			// Enables IQ output on cards which supports it (0 = normal, 1 = IQ Symbols, 2 = IQ Samples)
		KSPROPERTY_DD_BDA_BBFRAME_MODE = 6,		// Enables BBFRAMES output on cards which supports it
	};

	enum DD_SIGNAL_STANDARD {
		DD_SIGNAL_STANDARD_UNDEFINED = (ULONG)-1,
		DD_SIGNAL_STANDARD_DVBT = 0,
		DD_SIGNAL_STANDARD_DVBT2 = 1,
		DD_SIGNAL_STANDARD_DVBC = 2,
		DD_SIGNAL_STANDARD_DVBC2 = 3,
		DD_SIGNAL_STANDARD_DVBS = 4,
		DD_SIGNAL_STANDARD_DVBS2 = 5,
		DD_SIGNAL_STANDARD_DVBS2X = 6,
		DD_SIGNAL_STANDARD_ISDBT = 8,
		DD_SIGNAL_STANDARD_ISDBC = 9,
		DD_SIGNAL_STANDARD_ISDBS = 10,
		DD_SIGNAL_STANDARD_J83B = 16,
	};

	static constexpr ULONG NODE_ID_DD_BDA_DIGITAL_DEMODULATOR = 1;

	struct KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S {
		KSP_NODE ExtensionProp;
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S(ULONG Id)
		{
			ExtensionProp.Property = { KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR, Id, 0 };
			ExtensionProp.NodeId = NODE_ID_DD_BDA_DIGITAL_DEMODULATOR;
			ExtensionProp.Reserved = 0;
		}
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S(ULONG Id, ULONG flags)
			: KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S(Id)
		{
			SetFlags(flags);
		}
		void SetFlags(ULONG flags) {
			ExtensionProp.Property.Flags = flags;
		}
	};

	struct SelectStream {
		union {
			struct {
				USHORT StreamID;
			} DVBS2;
			struct {
				USHORT Profile;
			} DVBT2;
			struct {
				USHORT DataSliceID;
			} DVBC2;
			struct {
				USHORT TSID;
				USHORT ONID;
			} ISDBC;
			struct {
				USHORT TSID;
				USHORT Flag;
			} ISDBS;
			ULONG alignment;
		};
		SelectStream(void)
			: alignment(0)
		{
		}
	};

	static constexpr GUID KSPROPERTYSET_DD_BDA_SIGNAL_INFO = { 0x0aa8a602, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

	enum KSPROPERTY_DD_BDA_SIGNAL_INFO {
		KSPROPERTY_DD_BDA_SIGNAL_IQ,			// ARRAY OF SHORT (only supported on a subset of the hardware)
		KSPROPERTY_DD_BDA_SIGNAL_SNR,			// LONG  ( dB * 10 )
		KSPROPERTY_DD_BDA_SIGNAL_BER,			// ULONG*2 ( Numerator,Denominator )  Pre RS/BCH
		KSPROPERTY_DD_BDA_SIGNAL_BITRATE,		// ULONG ( calculated Bitrate )
		KSPROPERTY_DD_BDA_SIGNAL_OFFSET,		// LONG  ( frequency offset , returns 0 if not supported )
		KSPROPERTY_DD_BDA_SIGNAL_LOSTCOUNT,		// ULONG ( signal lost count )
		KSPROPERTY_DD_BDA_SIGNAL_QUALITY,		// LONG  ( Quality as defined by Nordig 2.4)
		KSPROPERTY_DD_BDA_SIGNAL_STRENGTH,		// LONG  ( dBｵV * 10 )
		KSPROPERTY_DD_BDA_SIGNAL_STANDARD,		// ULONG ( see enumeration below )
		KSPROPERTY_DD_BDA_SIGNAL_SYMBOLRATE,	// ULONG ( current symbolrate, returns 0 if not supported )
	};

	static constexpr ULONG NODE_ID_DD_BDA_SIGNAL_INFO = 1;

	struct KSPROPERTY_DD_BDA_SIGNAL_INFO_S {
		KSP_NODE ExtensionProp;
		KSPROPERTY_DD_BDA_SIGNAL_INFO_S(ULONG Id)
		{
			ExtensionProp.Property = { KSPROPERTYSET_DD_BDA_SIGNAL_INFO, Id, 0 };
			ExtensionProp.NodeId = NODE_ID_DD_BDA_SIGNAL_INFO;
			ExtensionProp.Reserved = 0;
		}
		KSPROPERTY_DD_BDA_SIGNAL_INFO_S(ULONG Id, ULONG flags)
			: KSPROPERTY_DD_BDA_SIGNAL_INFO_S(Id)
		{
			SetFlags(flags);
		}
		void SetFlags(ULONG flags) {
			ExtensionProp.Property.Flags = flags;
		}
	};

	struct SignalInfo {
		union {
			LONG SNR;
			struct {
				ULONG Numerator;
				ULONG Denominator;
			} BER;
			ULONG Bitrate;
			LONG Quality;
			LONG Strength;
			LONG Offset;
			ULONG LostCount;
			ULONG Standard;
			ULONG SymbolRate;
		};
	};

	HMODULE m_hMySelf;
	CComPtr<IKsControl> m_pControlTunerOutputPin;
	CComPtr<IBDA_DeviceControl> m_pIBDA_DeviceControl;
	CComPtr<IBDA_LNBInfo> m_pIBDA_LNBInfo;
	CComPtr<IBDA_DigitalDemodulator> m_pIBDA_DigitalDemodulator;
	CComPtr<IBDA_FrequencyFilter> m_pIBDA_FrequencyFilter;
	CComPtr<IBDA_DiseqCommand> m_pIBDA_DiseqCommand;
	CComPtr<IBDA_SignalStatistics> m_pIBDA_SignalStatistics;
	CComPtr<IBaseFilter> m_pTunerDevice;

	// チューニングスペース毎のデータ
	struct TuningSpaceData {
		DD_SIGNAL_STANDARD SignalStandard;
		TuningSpaceData(void)
			: SignalStandard(DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED)
		{
		}

		~TuningSpaceData(void)
		{
		}
	};

	struct TuningData {
		std::map<unsigned int, TuningSpaceData*> Spaces;	// チューニングスペース番号とチューニングスペース毎のデータ
		DWORD dwNumSpace;									// チューニングスペース数
		TuningData(void)
			: dwNumSpace(0)
		{
		}

		~TuningData(void)
		{
			for (auto it = Spaces.begin(); it != Spaces.end(); it++) {
				SAFE_DELETE(it->second);
			}
			Spaces.clear();
		}

		void Regist(unsigned int space, DD_SIGNAL_STANDARD signalStandard)
		{
			std::map<unsigned int, TuningSpaceData*>::iterator itSpace = Spaces.find(space);
			if (itSpace == Spaces.end()) {
				TuningSpaceData *tuningSpaceData = new TuningSpaceData();
				itSpace = Spaces.insert(Spaces.end(), std::pair<unsigned int, TuningSpaceData*>(space, tuningSpaceData));
			}
			itSpace->second->SignalStandard = signalStandard;
		}

		DD_SIGNAL_STANDARD GetSignalStandard(unsigned int space)
		{
			std::map<unsigned int, TuningSpaceData*>::iterator itSpace = Spaces.find(space);
			if (itSpace == Spaces.end()) {
				return DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED;
			}
			return itSpace->second->SignalStandard;
		}
	};
	TuningData m_TuningData;

	BOOL m_bSelectStreamRelative;
	BOOL m_bDisableTSMF;
	KSPROPERTY_DD_BDA_SIGNAL_INFO m_nGetSignalStrengthFunction;
};
