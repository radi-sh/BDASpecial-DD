#pragma once

#include "IBdaSpecials2.h"
#include <map>

#include <Ks.h>
#include <ksproxy.h>

/*
//
// Broadcast Driver Architecture で定義されている Property set / Method set
//
// KSNODE_BDA_RF_TUNER Input pin, id:0
static constexpr GUID KSPROPSETID_BdaFrequencyFilter = { 0x71985f47, 0x1ca1, 0x11d3,{ 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0 } };

enum KSPROPERTY_BDA_FREQUENCY_FILTER {					// MinProperty=32
	KSPROPERTY_BDA_RF_TUNER_FREQUENCY = 0,				// get/set		MinData=4
	KSPROPERTY_BDA_RF_TUNER_POLARITY,					// get/set		MinData=4
	KSPROPERTY_BDA_RF_TUNER_RANGE,						// get/set		MinData=4
	KSPROPERTY_BDA_RF_TUNER_TRANSPONDER,				// get/set		MinData=4
	KSPROPERTY_BDA_RF_TUNER_BANDWIDTH,					// get/set		MinData=4
	KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER,		// get/set		MinData=4
	KSPROPERTY_BDA_RF_TUNER_CAPS,						// not supported
	KSPROPERTY_BDA_RF_TUNER_SCAN_STATUS,				// not supported
	KSPROPERTY_BDA_RF_TUNER_STANDARD,					// not supported
	KSPROPERTY_BDA_RF_TUNER_STANDARD_MODE,				// not supported
};

// KSNODE_BDA_RF_TUNER Input pin, id:0
static constexpr GUID KSPROPSETID_BdaSignalStats = { 0x1347d106, 0xcf3a, 0x428a,{ 0xa5, 0xcb, 0xac, 0xd, 0x9a, 0x2a, 0x43, 0x38 } };

enum KSPROPERTY_BDA_SIGNAL_STATS {						// MinProperty=32
	KSPROPERTY_BDA_SIGNAL_STRENGTH = 0,					// get only		MinData=4
	KSPROPERTY_BDA_SIGNAL_QUALITY,						// get only		MinData=4
	KSPROPERTY_BDA_SIGNAL_PRESENT,						// get only		MinData=4
	KSPROPERTY_BDA_SIGNAL_LOCKED,						// get only		MinData=4
	KSPROPERTY_BDA_SAMPLE_TIME,							// get only		MinData=4
	KSPROPERTY_BDA_SIGNAL_LOCK_CAPS,					// get only		MinData=4
	KSPROPERTY_BDA_SIGNAL_LOCK_TYPE,					// get only		MinData=4
};

// KSNODE_BDA_RF_TUNER Input pin, id:0
static constexpr GUID KSPROPSETID_BdaLNBInfo = { 0x992cf102, 0x49f9, 0x4719,{ 0xa6, 0x64, 0xc4, 0xf2, 0x3e, 0x24, 0x8, 0xf4 } };

enum KSPROPERTY_BDA_LNB_INFO {							// MinProperty=32
	KSPROPERTY_BDA_LNB_LOF_LOW_BAND = 0,				// get/set		MinData=4
	KSPROPERTY_BDA_LNB_LOF_HIGH_BAND,					// get/set		MinData=4
	KSPROPERTY_BDA_LNB_SWITCH_FREQUENCY,				// get/set		MinData=4
};

// KSNODE_BDA_RF_TUNER Input pin, id:0
static constexpr GUID KSPROPSETID_BdaDiseqCommand = { 0xf84e2ab0, 0x3c6b, 0x45e3,{ 0xa0, 0xfc, 0x86, 0x69, 0xd4, 0xb8, 0x1f, 0x11 } };
enum KSPROPERTY_BDA_DISEQC_COMMAND {					// MinProperty=32
	KSPROPERTY_BDA_DISEQC_ENABLE = 0,					// get/set		MinData=4
	KSPROPERTY_BDA_DISEQC_LNB_SOURCE,					// get/set		MinData=4
	KSPROPERTY_BDA_DISEQC_USETONEBURST,					// not supported
	KSPROPERTY_BDA_DISEQC_REPEATS,						// not supported
	KSPROPERTY_BDA_DISEQC_SEND,							// set only		MinData=4
	KSPROPERTY_BDA_DISEQC_RESPONSE,						// not supported
};

// KSNODE_BDA_*_DEMODULATOR Output pin, id:1
static constexpr GUID KSPROPSETID_BdaDigitalDemodulator = { 0xef30f379, 0x985b, 0x4d10,{ 0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x4, 0xe1, 0xe0 } };

enum KSPROPERTY_BDA_DIGITAL_DEMODULATOR {				// MinProperty=32
	KSPROPERTY_BDA_MODULATION_TYPE = 0,					// get/set		MinData=4
	KSPROPERTY_BDA_INNER_FEC_TYPE,						// get/set		MinData=4
	KSPROPERTY_BDA_INNER_FEC_RATE,						// get/set		MinData=4
	KSPROPERTY_BDA_OUTER_FEC_TYPE,						// get/set		MinData=4
	KSPROPERTY_BDA_OUTER_FEC_RATE,						// get/set		MinData=4
	KSPROPERTY_BDA_SYMBOL_RATE,							// get/set		MinData=4
	KSPROPERTY_BDA_SPECTRAL_INVERSION,					// get/set		MinData=4
	KSPROPERTY_BDA_GUARD_INTERVAL,						// get/set		MinData=4
	KSPROPERTY_BDA_TRANSMISSION_MODE,					// get/set		MinData=4
	KSPROPERTY_BDA_ROLL_OFF,							// get/set		MinData=4
	KSPROPERTY_BDA_PILOT,								// get/set		MinData=4
	KSPROPERTY_BDA_SIGNALTIMEOUTS,						// not supported
	KSPROPERTY_BDA_PLP_NUMBER,							// get/set		MinData=4
};

// KSNODE_BDA_TS_SELECTOR Output pin, id:2? (driver ver.216で追加・未確認)
static constexpr GUID KSMETHODSETID_BdaTSSelector = { 0x1dcfafe9, 0xb45e, 0x41b3,{ 0xbb, 0x2a, 0x56, 0x1e, 0xb1, 0x29, 0xae, 0x98 } };

enum KSMETHOD_BDA_TS_SELECTOR {							// MinMethod=40
	KSMETHOD_BDA_TS_SELECTOR_SETTSID = 0,				//				MinData=4
	KSMETHOD_BDA_TS_SELECTOR_GETTSINFORMATION,			// not supported
};

// KSCATEGORY_BDA_NETWORK_TUNER Tuner filter
static constexpr GUID KSMETHODSETID_BdaChangeSync = { 0xfd0a5af3, 0xb41d, 0x11d2,{ 0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0 } };

enum KSMETHOD_BDA_CHANGE_SYNC {							// MinMethod=24
	KSMETHOD_BDA_START_CHANGES = 0,						//				MinData=0
	KSMETHOD_BDA_CHECK_CHANGES,							//				MinData=0
	KSMETHOD_BDA_COMMIT_CHANGES,						//				MinData=0
	KSMETHOD_BDA_GET_CHANGE_STATE,						//				MinData=0
};
*/

//
// Digital Devices BDA tuner filter driver 固有の Property set
//

// KSNODE_BDA_RF_TUNER Input pin, id:0
static constexpr GUID KSPROPERTYSET_DD_BDA_TUNER_CONFIG = { 0x0aa8a601, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

enum KSPROPERTY_DD_BDA_TUNER_CONFIG {					// MinProperty=32
	KSPROPERTY_DD_BDA_SCIF_SETTINGS = 0,				// get/set		MinData=16
	KSPROPERTY_DD_BDA_SCIF_SETTINGS_PERSISTANT = 1,		// get/set		MinData=16
	KSPROPERTY_DD_BDA_LMS_CONFIG = 2,					// get/set		MinData=8
	KSPROPERTY_DD_BDA_LMS_CONFIG_PERSISTANT = 3,		// get/set		MinData=8
	KSPROPERTY_DD_BDA_TUNER_CONFIG_UNKNOWN1000 = 1000,	// set only		MinData=20
};

struct DD_BDA_SCIF_SETTINGS {
	ULONG UBSlot;
	ULONG UBFrequency;
	ULONG UBPin;
	ULONG Flags;
};

static constexpr ULONG DD_BDA_SCIF_SETTINGS_FLAGS_MODE(BOOL EN50607)
{
	return (EN50607) ? 1 : 0;
}

static constexpr ULONG DD_BDA_SCIF_SETTINGS_FLAGS_POWERUPDELAY(ULONG Delay)
{
	return (((Delay + 199) / 200) << 4) & 0xF0;
}

struct DD_BDA_LMS_CONFIG {
	ULONG Mode;
	ULONG Input;
};

enum DD_BDA_LMS_CONFIG_MODE {
	DD_BDA_LMS_CONFIG_MODE_NOT_SET = -1,
	DD_BDA_LMS_CONFIG_MODE_QUAD = 0,
	DD_BDA_LMS_CONFIG_MODE_QUATTRO = 1,
	DD_BDA_LMS_CONFIG_MODE_SCIF = 2,
};

// KSNODE_BDA_*_DEMODULATOR Output pin, id:1
static constexpr GUID KSPROPERTYSET_DD_BDA_SIGNAL_INFO = { 0x0aa8a602, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

enum KSPROPERTY_DD_BDA_SIGNAL_INFO {					// MinProperty=32
	KSPROPERTY_DD_BDA_SIGNAL_IQ,						// get only		MinData=4	ARRAY OF SHORT (only supported on a subset of the hardware)
	KSPROPERTY_DD_BDA_SIGNAL_SNR,						// get only		MinData=4	LONG  ( dB * 10 )
	KSPROPERTY_DD_BDA_SIGNAL_BER,						// get only		MinData=8	ULONG*2 ( Numerator,Denominator )  Pre RS/BCH
	KSPROPERTY_DD_BDA_SIGNAL_BITRATE,					// get only		MinData=4	ULONG ( calculated Bitrate )
	KSPROPERTY_DD_BDA_SIGNAL_OFFSET,					// get only		MinData=4	LONG  ( frequency offset , returns 0 if not supported )
	KSPROPERTY_DD_BDA_SIGNAL_LOSTCOUNT,					// get only		MinData=4	ULONG ( signal lost count )
	KSPROPERTY_DD_BDA_SIGNAL_QUALITY,					// get only		MinData=4	LONG  ( Quality as defined by Nordig 2.4)
	KSPROPERTY_DD_BDA_SIGNAL_STRENGTH,					// get only		MinData=4	LONG  ( dBuV * 10 )
	KSPROPERTY_DD_BDA_SIGNAL_STANDARD,					// get only		MinData=4	ULONG ( see enumeration below )
	KSPROPERTY_DD_BDA_SIGNAL_SYMBOLRATE,				// get only		MinData=4	ULONG ( current symbolrate, returns 0 if not supported )
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

// KSNODE_BDA_*_DEMODULATOR Output pin, id:1
static constexpr GUID STATIC_KSPROPERTYSET_DD_BDA_SIGNAL_INFO2 = { 0x0aa8a603, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

enum KSPROPERTY_DD_BDA_SIGNAL_INFO2 {					// MinProperty=32
	KSPROPERTY_DD_BDA_SIGNAL_TYPE,						// get only		MinData=4	ULONG (DD_SIGNAL_TYPE_xxx)
	KSPROPERTY_DD_BDA_SIGNAL_T2_PLPIDS,					// get only		MinData=284	DD_T2_PLPIDS
};

enum DD_SIGNAL_TYPE {
	DD_SIGNAL_TYPE_UNK = 0,
	DD_SIGNAL_TYPE_X1 = 1,			// Signal is T,C,...
	DD_SIGNAL_TYPE_X2 = 2,			// Signal is T2,C2,..
};

struct DD_T2_PLPIDS {
	ULONG Length;					// Structure Length
	ULONG Version;					// 1
	ULONG PLPID;					// 
	ULONG CommonPLPID;				// (0xFFFFFFFF = No Common PLP)
	ULONG Flags;					// Bit 0 = 1: FEF Exists (scan also for Lite)
	ULONG Reserved;
	ULONG NumPLPS;
	BYTE PLPList[256];
};

enum DD_T2_PLPIDS_FLAGS {
	DD_T2_PLPIDS_FEF = 0x00000001,	// 1: FEF Exists (scan also for Lite)
	DD_T2_PLPIDS_LITE = 0x00000002,	// 1: Is Lite
};

// KSNODE_BDA_*_DEMODULATOR Output pin, id:1
static constexpr GUID KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR = { 0x0aa8a605, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

enum KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR {			// MinProperty=32
	KSPROPERTY_DD_BDA_SELECT_STANDARD = 0,				// get/set		MinData=4	Select signal standard
	KSPROPERTY_DD_BDA_SELECT_STREAM = 1,				// get/set		MinData=4	DVB-S2: input stream id
														//							DVB-T2: 0 = Base Profile, 1 = Lite Profile
														//							DVB-C2: Data Slice ID
														//							ISDB-C: High Word = ONID, Low Word = TSID.  ONID == 0 -> TSID is relative, TSID == 0xFFFF -> disable TSMF
														//							ISDB-S: High Word = Flag, Low Word = TSID.  Flag == 0 -> TSID is relative
	KSPROPERTY_DD_BDA_S2_ISI = KSPROPERTY_DD_BDA_SELECT_STREAM,
	KSPROPERTY_DD_BDA_S2_PLSSI = 2,						// get/set		MinData=4	Sets DVB-S2 physical layer scrambling sequence index (Gold code)
	KSPROPERTY_DD_BDA_S2_MODMASK = 3,					// get/set		MinData=4	Sets DVB-S2/S2X Modulation Mask 
	KSPROPERTY_DD_BDA_IQ_MODE = 5,						// get/set		MinData=4	Enables IQ output on cards which supports it (0 = normal, 1 = IQ Symbols, 2 = IQ Samples)
	KSPROPERTY_DD_BDA_BBFRAME_MODE = 6,					// get/set		MinData=4	Enables BBFRAMES output on cards which supports it
};

static constexpr ULONG DD_BDA_PLSSI_DEFAULT = 0;

enum DD_BDA_IQ_MODE {
	DD_BDA_IQ_MODE_OFF = 0,			// Outputs regular transport stream 
	DD_BDA_IQ_MODE_SYMBOLS = 1,		// Ouputs IQ symbols at symbol rate (bits/sample depends on card) embedded in TS packets
	DD_BDA_IQ_MODE_SAMPLES = 2,		// Ouputs IQ samples at ADC rate (bits/sample depends on card) embedded in TS packets
	DD_BDA_IQ_MODE_VTM = 3,			// Ouputs IQ samples at symbolrate embedded in TS packets
};

enum DD_BDA_BBFRAME_MODE {
	DD_BDA_BBFRAME_MODE_OFF = 0,	// Outputs regular transport stream for MATYPE = TS. (default)
	DD_BDA_BBFRAME_MODE_EMBED = 1,	// Embed BBFRames in TS (even if MATYPE = TS)
};

enum DD_BDA_S2_MODMASK {
	DD_BDA_S2_MODMASK_QPSK = 1,
	DD_BDA_S2_MODMASK_8PSK = 2,
	DD_BDA_S2_MODMASK_16APSK = 4,
	DD_BDA_S2_MODMASK_32APSK = 8,
	DD_BDA_S2_MODMASK_64APSK = 16,
	DD_BDA_S2_MODMASK_128APSK = 32,
	DD_BDA_S2_MODMASK_256APSK = 64,
};

// KSCATEGORY_BDA_NETWORK_TUNER Tuner filter?・KSPROPERTYSET_DD_BDA_SIGNAL_INFO と同じ? (未確認)
static constexpr GUID KSPROPERTYSET_DD_BDA_UNKNOWN_A606 = { 0x0aa8a606, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

// KSCATEGORY_BDA_NETWORK_TUNER Tuner filter? (未確認)
static constexpr GUID KSPROPERTYSET_DD_BDA_UNKNOWN_A502 = { 0x0aa8a502, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

enum KSPROPERTY_DD_BDA_UNKNOWN_A {						// MinProperty=24
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_0 = 0,			// get only		MinData=4
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_3 = 3,			// get only		MinData=60
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_4 = 4,			// get/set		MinData=4
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_7 = 7,			// get/set		MinData=16
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_8 = 8,			// get/set		MinData=8
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_9 = 9,			// get/set		MinData=4
	KSPROPERTYSET_DD_BDA_UNKNOWN_A502_10 = 10,			// get/set		MinData=4
};

class CDDSpecials : public IBdaSpecials2b2
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
	const HRESULT LockChannel(const TuningParam *pTuningParam);
	const HRESULT ReadIniFile(const WCHAR *szIniFilePath);
	const HRESULT IsDecodingNeeded(BOOL *pbAns);
	const HRESULT Decode(BYTE *pBuf, DWORD dwSize);
	const HRESULT GetSignalStrength(float *fVal);
	const HRESULT PreLockChannel(TuningParam *pTuningParam);
	const HRESULT PreTuneRequest(const TuningParam *pTuningParam, ITuneRequest *pITuneRequest);
	const HRESULT PostTuneRequest(const TuningParam *pTuningParam);
	const HRESULT PostLockChannel(const TuningParam *pTuningParam);

	virtual void Release(void);

private:
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
	CRITICAL_SECTION m_CriticalSection;

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
		std::map<unsigned int, TuningSpaceData> Spaces;		// チューニングスペース番号とチューニングスペース毎のデータ
		DWORD dwNumSpace;									// チューニングスペース数
		TuningData(void)
			: dwNumSpace(0)
		{
		}

		~TuningData(void)
		{
			Spaces.clear();
		}

		void Regist(unsigned int space, DD_SIGNAL_STANDARD signalStandard)
		{
			auto itSpace = Spaces.find(space);
			if (itSpace == Spaces.end()) {
				itSpace = Spaces.emplace(space, TuningSpaceData()).first;
			}
			itSpace->second.SignalStandard = signalStandard;
		}

		DD_SIGNAL_STANDARD GetSignalStandard(unsigned int space)
		{
			auto itSpace = Spaces.find(space);
			if (itSpace == Spaces.end()) {
				return DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED;
			}
			return itSpace->second.SignalStandard;
		}
	};
	TuningData m_TuningData;

	BOOL m_bSelectStreamRelative;
	BOOL m_bDisableTSMF;
	BOOL m_bLNBPowerOff;
	KSPROPERTY_DD_BDA_SIGNAL_INFO m_nGetSignalStrengthFunction;
};
