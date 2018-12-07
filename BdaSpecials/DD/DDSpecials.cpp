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
	if (IniFileAccess.ReadKeyB(L"DD", L"DebugLog", FALSE)) {
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
	CComQIPtr<IBDA_DeviceControl> pDeviceControl(m_pTunerDevice);
	if (!pDeviceControl) {
		OutputDebug(L"Can not get IBDA_DeviceControl.\n");
		return E_NOINTERFACE;
	}
	m_pDeviceControl = pDeviceControl;

	if (m_pControlTunerOutputPin == NULL) {
		// チューナーデバイスの output pinを見つける
		CComPtr<IEnumPins> pPinEnum;
		m_pTunerDevice->EnumPins(&pPinEnum);
		if (pPinEnum) {
			while (!m_pControlTunerOutputPin) {
				CComPtr<IPin> pPin;
				if (SUCCEEDED(pPinEnum->Next(1, &pPin, NULL))) {
					if (!pPin) {
						OutputDebug(L"Can not find tuner output pin.\n");
						break;
					}
					PIN_DIRECTION dir;
					if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PIN_DIRECTION::PINDIR_OUTPUT) {
						// output pin の IKsPropertySet を取得
						CComQIPtr<IKsControl> pControlTunerPin(pPin);
						if (!pControlTunerPin) {
							OutputDebug(L"Tuner output pin does not have IKsPropertySet.\n");
						}
						m_pControlTunerOutputPin = pControlTunerPin;
						break;
					}
				}
			}
		}

		if (!m_pControlTunerOutputPin) {
			OutputDebug(L"Fail to get IKsPropertySet of tuner output pin.\n");
			return E_NOINTERFACE;
		}
		
		if (!m_bEnableSelectStandard) {
			OutputDebug(L"SelectStandard is disabled.\n");
		}
		else {
			// KSPROPERTY_DD_BDA_SELECT_STANDARD がサポートされているか確認
			DWORD TypeSupport = 0;
			ULONG BytesReturned = 0;
			KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_BASICSUPPORT);
			if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &TypeSupport, sizeof(TypeSupport), &BytesReturned))) {
				switch (hr) {
				case E_NOTIMPL:
				case E_PROP_SET_UNSUPPORTED:
					OutputDebug(L"KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR property set is not supported.\n");
					break;

				case E_PROP_ID_UNSUPPORTED:
					OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD property ID is not supported.\n");
					break;

				default:
					OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD:KSPROPERTY_TYPE_BASICSUPPORT: Fail to IKsControl::KsProperty() function.\n");
					break;
				}
				return E_NOINTERFACE;
			}
			OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD: TypeSupport=%ld.\n", TypeSupport);
			if (!(TypeSupport & KSPROPERTY_SUPPORT_SET)) {
				OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STANDARD property ID does not support KSPROPERTY_TYPE_SET.\n");
				return E_NOINTERFACE;
			}
			OutputDebug(L"SelectStandard is enabled.\n");
		}

		if (!m_bEnableSelectStream) {
			OutputDebug(L"SelectStream is disabled.\n");
		}
		else {
			// KSPROPERTY_DD_BDA_SELECT_STREAM がサポートされているか確認
			DWORD TypeSupport = 0;
			ULONG BytesReturned = 0;
			KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStream(KSPROPERTY_DD_BDA_SELECT_STREAM, KSPROPERTY_TYPE_BASICSUPPORT);
			if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &TypeSupport, sizeof(TypeSupport), &BytesReturned))) {
				switch (hr) {
				case E_NOTIMPL:
				case E_PROP_SET_UNSUPPORTED:
					OutputDebug(L"KSPROPERTYSET_DD_BDA_DIGITAL_DEMODULATOR property set is not supported.\n");
					break;

				case E_PROP_ID_UNSUPPORTED:
					OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM property ID is not supported.\n");
					break;

				default:
					OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM:KSPROPERTY_TYPE_BASICSUPPORT: Fail to IKsControl::KsProperty() function.\n");
					break;
				}
				return E_NOINTERFACE;
			}
			OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM: TypeSupport=%ld.\n", TypeSupport);
			if (!(TypeSupport & KSPROPERTY_SUPPORT_SET)) {
				OutputDebug(L"KSPROPERTY_DD_BDA_SELECT_STREAM property ID does not support KSPROPERTY_TYPE_SET.\n");
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

	static const std::map<const std::wstring, const int> mapGetSignalStrengthFunction = {
		{ L"",        -1 },
		{ L"SignalStrength", KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_STRENGTH },
		{ L"SignalQuality",  KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_QUALITY },
		{ L"SNR",            KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_SNR },
		{ L"BER",            KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_BER },
		{ L"BITRATE",        KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_BITRATE },
	};

	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"DD");

	// Select Standard 処理を行う
	m_bEnableSelectStandard = IniFileAccess.ReadKeyB(L"EnableSelectStandard", FALSE);

	// Select Stream 処理を行う
	m_bEnableSelectStream = IniFileAccess.ReadKeyB(L"EnableSelectStream", FALSE);

	// Select Stream 処理でTSIDを相対でセットする
	m_bSelectStreamRelative = IniFileAccess.ReadKeyB(L"SelectStreamRelative", FALSE);

	// Select Stream 処理でTSMFを無効にする
	m_bDisableTSMF = IniFileAccess.ReadKeyB(L"DisableTSMF", FALSE);

	// GetSignalStrength 関数で返す値
	m_nGetSignalStrengthFunction = (KSPROPERTY_DD_BDA_SIGNAL_INFO)IniFileAccess.ReadKeyIValueMap(L"GetSignalStrengthFunction", -1, mapGetSignalStrengthFunction);

	/* Test用コード */
	// IKsControl::KsProperty() の前後に IBDA_DeviceControl::StartChanges() / IBDA_DeviceControl::CommitChanges() が必要
	m_bNeedCommitChanges = IniFileAccess.ReadKeyB(L"NeedCommitChanges", FALSE);
	/* Test用コード終わり */

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
	if (m_nGetSignalStrengthFunction == -1) {
		return E_NOINTERFACE;
	}

	if (!fVal)
		return E_INVALIDARG;

	HRESULT hr;
	SignalInfo signalInfo;
	ULONG BytesReturned = 0;
	KSPROPERTY_DD_BDA_SIGNAL_INFO_S PropSignalInfo(m_nGetSignalStrengthFunction, KSPROPERTY_TYPE_GET);
	if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropSignalInfo, sizeof(PropSignalInfo), &signalInfo, sizeof(signalInfo), &BytesReturned))) {
		OutputDebug(L"SignalInfo: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET function. ret=0x%08lx\n", hr);
		return hr;
	}
	*fVal = 0.0F;
	switch (m_nGetSignalStrengthFunction) {
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_STRENGTH:
		*fVal = (float)signalInfo.Strength / 10.0F;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_QUALITY:
		*fVal = (float)signalInfo.Quality;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_SNR:
		*fVal = (float)signalInfo.SNR / 10.0F;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_BER:
		if (signalInfo.BER.Denominator == 0) {
			*fVal = -1;
		}
		else {
			*fVal = (float)signalInfo.BER.Numerator / (float)signalInfo.BER.Denominator;
		}
		break;
	}

	return S_OK;
}

const HRESULT CDDSpecials::PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest)
{
	if (!m_pControlTunerOutputPin || !m_bEnableSelectStandard) {
		return S_OK;
	}
	HRESULT hr;
	ULONG val = 0;
	ULONG BytesReturned = 0;
	KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_GET);
	if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), &BytesReturned))) {
		OutputDebug(L"SelectStandard: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET function. ret=0x%08lx\n", hr);
		return E_FAIL;
	}
	OutputDebug(L"SelectStandard: Succeeded to IKsControl::KsProperty() KSPROPERTY_TYPE_GET function. bytes=%ld, val=%ld\n", BytesReturned, val);
	val = m_TuningData.GetSignalStandard(pTuningParm->IniSpaceID);
	/* Test用コード */
	if (m_bNeedCommitChanges) {
		if (FAILED(hr = m_pDeviceControl->StartChanges())) {
			OutputDebug(L"SelectStandard: Fail to IBDA_DeviceControl::StartChanges() function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
	}
	/* Test用コード終わり */
	PropStandard.SetFlags(KSPROPERTY_TYPE_SET);
	OutputDebug(L"SelectStandard: trying to set. val=%ld.\n", val);
	if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), NULL))) {
		OutputDebug(L"SelectStandard: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET function. ret=0x%08lx\n", hr);
		return E_FAIL;
	}
	/* Test用コード */
	if (m_bNeedCommitChanges) {
		if (FAILED(hr = m_pDeviceControl->CommitChanges())) {
			OutputDebug(L"SelectStandard: Fail to IBDA_DeviceControl::CommitChanges() function. ret=0x%08lx\n", hr);
			// 全ての変更を取り消す
			hr = m_pDeviceControl->StartChanges();
			hr = m_pDeviceControl->CommitChanges();
			return E_FAIL;
		}
	}
	/* Test用コード終わり */
	return S_OK;
}

const HRESULT CDDSpecials::PostLockChannel(const TuningParam *pTuningParm)
{
	if (!m_pControlTunerOutputPin || !m_bEnableSelectStream) {
		return S_OK;
	}
	HRESULT hr;
	SelectStream writeStream;
	ULONG value = 0;
	ULONG ss = m_TuningData.GetSignalStandard(pTuningParm->IniSpaceID);
	switch (ss) {
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
		writeStream.ISDBC.TSID = m_bDisableTSMF ? 0xFFFFU : (USHORT)pTuningParm->TSID;
		writeStream.ISDBC.ONID = m_bSelectStreamRelative ? 0U : (USHORT)pTuningParm->ONID;
		break;
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
		writeStream.ISDBS.TSID = (USHORT)pTuningParm->TSID;
		writeStream.ISDBS.Flag = m_bSelectStreamRelative ? 0U : 1U;
		break;
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
		writeStream.DVBS2.StreamID = (USHORT)pTuningParm->SID;
		break;
	}
	switch (ss) {
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
	case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
		ULONG val = 0;
		ULONG BytesReturned = 0;
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStream(KSPROPERTY_DD_BDA_SELECT_STREAM, KSPROPERTY_TYPE_GET);
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &val, sizeof(val), &BytesReturned))) {
			OutputDebug(L"SelectStream: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		OutputDebug(L"SelectStream: Succeeded to IKsControl::KsProperty() KSPROPERTY_TYPE_GET function. bytes=%ld, val=0x%08lx\n", BytesReturned, val);
		val = value;
		/* Test用コード */
		if (m_bNeedCommitChanges) {
			if (FAILED(hr = m_pDeviceControl->StartChanges())) {
				OutputDebug(L"SelectStream: Fail to IBDA_DeviceControl::StartChanges() function. ret=0x%08lx\n", hr);
				return E_FAIL;
			}
		}
		/* Test用コード終わり */
		PropStream.SetFlags(KSPROPERTY_TYPE_SET);
		OutputDebug(L"SelectStream: trying to set. val=0x%08lx.\n", writeStream.alignment);
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &writeStream, sizeof(writeStream), NULL))) {
			OutputDebug(L"SelectStream: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		/* Test用コード */
		if (m_bNeedCommitChanges) {
			if (FAILED(hr = m_pDeviceControl->CommitChanges())) {
				OutputDebug(L"SelectStream: Fail to IBDA_DeviceControl::CommitChanges() function. ret=0x%08lx\n", hr);
				// 全ての変更を取り消す
				hr = m_pDeviceControl->StartChanges();
				hr = m_pDeviceControl->CommitChanges();
				return E_FAIL;
			}
		}
		/* Test用コード終わり */
		break;
	}
	return S_OK;
}

void CDDSpecials::Release(void)
{
	delete this;
}
