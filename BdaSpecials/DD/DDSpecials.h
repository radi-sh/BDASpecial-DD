#pragma once

#include "IBdaSpecials2.h"
#include <map>

#include <Ks.h>
#include <ksproxy.h>

static const GUID KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR = { 0x0aa8a605, 0xa240, 0x11de, {0xb1, 0x30, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x56} };

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
	enum KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR {
		KSPROPERTY_DD_BDA_SELECT_STANDARD = 0,	// Select signal standard
		KSPROPERTY_DD_BDA_SELECT_STREAM = 1,	// DVB-S2: input stream id
												// DVB-T2: 0 = Base Profile, 1 = Lite Profile
												// DVB-C2: Data Slice ID
												// ISDB-C: High Word = ONID, Low Word = TSID.  ONID == 0 -> TSID is relative, TSID == 0xFFFF -> disable TSMF
												// ISDB-S: High Word = Flag, Low Word = TSID.  Flag == 0 -> TSID is relative
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

#define NODE_ID_DD_BDA_DIGITAL_DEMODULATOR 1

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

	HMODULE m_hMySelf;
	CComPtr<IKsControl> m_pControlTunerOutputPin;
	CComPtr<IBaseFilter> m_pTunerDevice;
	CComPtr<IBDA_DeviceControl> m_pDeviceControl;

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

	BOOL m_bEnableSelectStandard;
	BOOL m_bEnableSelectStream;
	BOOL m_bSelectStreamRelative;
	/* Test用コード */
	BOOL m_bNeedCommitChanges;
	/* Test用コード終わり */
	BOOL m_bDisableTSMF;
};
