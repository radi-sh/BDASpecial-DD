#include "common.h"

#include "DDSpecials.h"

#include <Windows.h>
#include <string>

#include <dshow.h>
#include "CIniFileAccess.h"

FILE *g_fpLog = NULL;

HMODULE hMySelf;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		// モジュールハンドル保存
		hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// デバッグログファイルのクローズ
		CloseDebugLog();
		break;
	}
	return TRUE;
}

__declspec(dllexport) IBdaSpecials * CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice)
{
	return new CDDSpecials(hMySelf, pTunerDevice);
}

__declspec(dllexport) HRESULT CheckAndInitTuner(IBaseFilter *pTunerDevice, const WCHAR *szDisplayName, const WCHAR *szFriendlyName, const WCHAR *szIniFilePath)
{
	CIniFileAccess IniFileAccess(szIniFilePath);

	// DebugLogを記録するかどうか
	if (IniFileAccess.ReadKeyB(L"DD", L"DebugLog", 0)) {
		// DebugLogのファイル名取得
		SetDebugLog(common::GetModuleName(hMySelf) + L"log");
	}

	return S_OK;
}

CDDSpecials::CDDSpecials(HMODULE hMySelf, CComPtr<IBaseFilter> pTunerDevice)
	: m_hMySelf(hMySelf),
	  m_pTunerDevice(pTunerDevice)
{
	return;
}

CDDSpecials::~CDDSpecials()
{
	m_hMySelf = NULL;

	return;
}

const HRESULT CDDSpecials::InitializeHook(void)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	HRESULT hr;
	if (m_pPropsetTunerOutputPin == NULL) {
		// チューナーデバイスの output pinを見つける
		CComPtr<IEnumPins> pPinEnum;
		m_pTunerDevice->EnumPins(&pPinEnum);
		if (pPinEnum) {
			CComPtr<IPin> pPin;
			while (!(m_pPropsetTunerOutputPin) && SUCCEEDED(pPinEnum->Next(1, &pPin, NULL))) {
				PIN_DIRECTION dir;
				if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PIN_DIRECTION::PINDIR_OUTPUT) {
					// output pin の IKsPropertySet を取得
					CComQIPtr<IKsPropertySet> pPropsetTunerPin(pPin);
					if (!pPropsetTunerPin) {
						m_pPropsetTunerOutputPin = pPropsetTunerPin;
						break;
					}
				}
			}
		}

		if (!m_pPropsetTunerOutputPin) {
			OutputDebug(L"Fail to get IKsPropertySet of tuner output pin.\n");
			return E_NOINTERFACE;
		}
		
		DWORD TypeSupport = 0;
		
		if (!m_bEnableSelectStandard) {
			OutputDebug(L"SelectStandard is disabled.\n");
		}
		else {
			// KSPROPERTY_DD_BDA_SELECT_STANDARD がサポートされているか確認
			if (FAILED(hr = m_pPropsetTunerOutputPin->QuerySupported(KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR, KSPROPERTY_DD_BDA_SELECT_STANDARD, &TypeSupport))) {
				switch (hr) {
				case E_NOTIMPL:
				case E_PROP_SET_UNSUPPORTED:
					OutputDebug(L"KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR property set is not supported.\n");
					break;

				case E_PROP_ID_UNSUPPORTED:
					OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD property ID is not supported.\n");
					break;

				default:
					OutputDebug(L"Fail to IKsPropertySet::QuerySupported() function.\n");
					break;
				}
				return E_NOINTERFACE;
			}
			OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD QuerySupported=%ld.\n", TypeSupport);
			if (!(TypeSupport & KSPROPERTY_SUPPORT_SET)) {
				OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD property ID does not support IKsPropertySet::Set() function.\n");
				return E_NOINTERFACE;
			}
			OutputDebug(L"SelectStandard is enabled.\n");
		}

		if (!m_bEnableSelectStream) {
			OutputDebug(L"SelectStream is disabled.\n");
		}
		else {
			// KSPROPERTY_DD_BDA_SELECT_STREAM がサポートされているか確認
			if (FAILED(hr = m_pPropsetTunerOutputPin->QuerySupported(KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR, KSPROPERTY_DD_BDA_SELECT_STREAM, &TypeSupport))) {
				switch (hr) {
				case E_NOTIMPL:
				case E_PROP_SET_UNSUPPORTED:
					OutputDebug(L"KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR property set is not supported.\n");
					break;

				case E_PROP_ID_UNSUPPORTED:
					OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM property ID is not supported.\n");
					break;

				default:
					OutputDebug(L"Fail to IKsPropertySet::QuerySupported() function.\n");
					break;
				}
				return E_NOINTERFACE;
			}
			OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM QuerySupported=%ld.\n", TypeSupport);
			if (!(TypeSupport & KSPROPERTY_SUPPORT_SET)) {
				OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM property ID does not support IKsPropertySet::Set() function.\n");
				return E_NOINTERFACE;
			}
			OutputDebug(L"SelectStream is enabled.\n");
		}
	}

	return S_OK;
}

const HRESULT CDDSpecials::Set22KHz(bool bActive)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::Set22KHz(long nTone)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::FinalizeHook(void)
{
	return S_OK;
}

const HRESULT CDDSpecials::GetSignalState(int *pnStrength, int *pnQuality, int *pnLock)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::LockChannel(BYTE bySatellite, BOOL bHorizontal, unsigned long ulFrequency, BOOL bDvbS2)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::LockChannel(const TuningParam *pTuningParm)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::SetLNBPower(bool bActive)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::ReadIniFile(const WCHAR *szIniFilePath)
{
	static const std::map<const std::wstring, const int> mapSignalStandard = {
		{ L"",        DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED },
		{ L"DVB-T",   DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBT },
		{ L"DVB-T2",  DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBT2 },
		{ L"DVB-C",   DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBC },
		{ L"J.83A",   DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBC },
		{ L"DVB-C2",  DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBC2 },
		{ L"DVB-S",   DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS },
		{ L"DVB-S2",  DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2 },
		{ L"DVB-S2X", DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2X },
		{ L"ISDB-T",  DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBT },
		{ L"ISDB-C",  DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC },
		{ L"J.83C",   DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC },
		{ L"ISDB-S",  DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS },
		{ L"J.83B",   DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_J83B },
	};

	std::map<unsigned int, TuningSpaceData*>::iterator itSpace;

	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"DD");

	// Select Standard 処理を行う
	m_bEnableSelectStandard = IniFileAccess.ReadKeyB(L"EnableSelectStandard", 0);

	// Select Stream 処理を行う
	m_bEnableSelectStream = IniFileAccess.ReadKeyB(L"EnableSelectStream", 0);

	// Select Stream 処理でTSIDを相対でセットする
	m_bSelectStreamRelative = IniFileAccess.ReadKeyB(L"SelectStreamRelative", 0);

	// Select Stream 処理でTSMFを無効にする
	m_bDisableTSMF = IniFileAccess.ReadKeyB(L"DisableTSMF", 0);

	if (m_bEnableSelectStandard) {
		// チューニング空間00～99の設定を読込
		for (DWORD space = 0; space < 100; space++) {
			std::wstring section = common::WStringPrintf(L"TUNINGSPACE%02d", space);
			if (IniFileAccess.ReadSection(section) <= 0) {
				continue;
			}
			IniFileAccess.CreateSectionData();
			DD_SIGNAL_STANDARD ss = (DD_SIGNAL_STANDARD)IniFileAccess.ReadKeyIValueMapSectionData(L"DD_SelectStandard", DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED, mapSignalStandard);
			m_TuningData.Regist(space, ss);
		}
	}

	return S_OK;
}

const HRESULT CDDSpecials::IsDecodingNeeded(BOOL *pbAns)
{
	if (pbAns)
		*pbAns = FALSE;

	return S_OK;
}

const HRESULT CDDSpecials::Decode(BYTE *pBuf, DWORD dwSize)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::GetSignalStrength(float *fVal)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest)
{
	if (!m_pPropsetTunerOutputPin || !m_bEnableSelectStandard) {
		return S_OK;
	}
	HRESULT hr;
	ULONG val = m_TuningData.GetSignalStandard(pTuningParm->IniSpaceID);
	OutputDebug(L"SelectStandard: trying to set. val=%ld.\n", val);
	if (FAILED(hr = m_pPropsetTunerOutputPin->Set(KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR, KSPROPERTY_DD_BDA_SELECT_STANDARD, &val, sizeof(val), &val, sizeof(val)))) {
		OutputDebug(L"SelectStandard: Fail to IKsPropertySet::Set() function. ret=0x%08lx\n", hr);
		return E_FAIL;
	}
	return S_OK;
}

const HRESULT CDDSpecials::PostLockChannel(const TuningParam *pTuningParm)
{
	if (!m_pPropsetTunerOutputPin || !m_bEnableSelectStream) {
		return S_OK;
	}
	HRESULT hr;
	ULONG val = 0;
	ULONG ss = m_TuningData.GetSignalStandard(pTuningParm->IniSpaceID);
	switch (ss) {
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
		val = (m_bSelectStreamRelative ? 0L : (pTuningParm->ONID & 0xFFFFL) << 16) + (m_bDisableTSMF ? 0xFFFFL : (pTuningParm->TSID & 0xFFFFL));
		break;
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
		val = (m_bSelectStreamRelative ? 0L : 0x10000L) + (pTuningParm->TSID & 0xFFFFL);
		break;
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
		val = (pTuningParm->SID & 0xFFFFL);
		break;
	}
	switch (ss) {
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
		OutputDebug(L"SelectStream: trying to set. val=%ld.\n", val);
		if (FAILED(hr = m_pPropsetTunerOutputPin->Set(KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR, KSPROPERTY_DD_BDA_SELECT_STREAM, &val, sizeof(val), &val, sizeof(val)))) {
			OutputDebug(L"SelectStream: Fail to IKsPropertySet::Set() function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		break;
	}
	return S_OK;
}

void CDDSpecials::Release(void)
{
	delete this;
}
