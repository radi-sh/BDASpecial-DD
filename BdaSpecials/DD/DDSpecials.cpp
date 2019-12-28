#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "common.h"

#include "DDSpecials.h"

#include <Windows.h>
#include <string>

#include <dshow.h>
#include "CIniFileAccess.h"
#include "DSFilterEnum.h"

HMODULE CDDSpecials::m_hMySelf = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		// モジュールハンドル保存
		CDDSpecials::m_hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// デバッグログファイルのクローズ
		CloseDebugLog();
		break;
	}
	return TRUE;
}

__declspec(dllexport) IBdaSpecials* CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice)
{
	return new CDDSpecials(pTunerDevice);
}

__declspec(dllexport) HRESULT CheckAndInitTuner(IBaseFilter* /*pTunerDevice*/, const WCHAR* /*szDisplayName*/, const WCHAR* /*szFriendlyName*/, const WCHAR* szIniFilePath)
{
	CIniFileAccess IniFileAccess(szIniFilePath);

	// DebugLogを記録するかどうか
	if (IniFileAccess.ReadKeyB(L"DD", L"DebugLog", FALSE)) {
		// DebugLogのファイル名取得
		SetDebugLog(common::GetModuleName(CDDSpecials::m_hMySelf) + L"log");
	}

	return S_OK;
}

CDDSpecials::CDDSpecials(CComPtr<IBaseFilter> pTunerDevice)
	: m_pTunerDevice(pTunerDevice)
{
	::InitializeCriticalSection(&m_CriticalSection);

	return;
}

CDDSpecials::~CDDSpecials()
{
	::DeleteCriticalSection(&m_CriticalSection);

	return;
}

const HRESULT CDDSpecials::InitializeHook(void)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	HRESULT hr;

	// Tuner の IKsControl
	{
		CComQIPtr<IKsControl> pControlTunerFilter(m_pTunerDevice);
		if (!pControlTunerFilter) {
			OutputDebug(L"Fail to get IKsControl of tuner filter.\n");
			return E_NOINTERFACE;
		}
		m_pControlTunerFilter = pControlTunerFilter;
	}

	// チューナーデバイス input pin の IKsControl / output pin の IKsControl
	{
		CDSEnumPins DSEnumPins(m_pTunerDevice);
		CComPtr<IPin> pPin;
		PIN_DIRECTION dir;
		do {
			if (FAILED(hr = DSEnumPins.getNextPin(&pPin, &dir))) {
				break;
			}
			CComQIPtr<IKsControl> pControlTunerPin(pPin);
			switch (dir) {
			case PIN_DIRECTION::PINDIR_INPUT:
				if (!m_pControlTunerInputPin) {
					m_pControlTunerInputPin = pControlTunerPin;
				}
				break;
			case PIN_DIRECTION::PINDIR_OUTPUT:
				if (!m_pControlTunerOutputPin) {
					m_pControlTunerOutputPin = pControlTunerPin;
				}
				break;
			}
		} while (!m_pControlTunerInputPin || !m_pControlTunerOutputPin);
	}
	if (!m_pControlTunerInputPin) {
		OutputDebug(L"Fail to get IKsControl of tuner input pin.\n");
		return E_NOINTERFACE;
	}
	if (!m_pControlTunerOutputPin) {
		OutputDebug(L"Fail to get IKsControl of tuner output pin.\n");
		return E_NOINTERFACE;
	}

	// IBDA_DeviceControl
	{
		CComQIPtr<IBDA_DeviceControl> pDeviceControl(m_pTunerDevice);
		if (!pDeviceControl) {
			OutputDebug(L"Can not get IBDA_DeviceControl.\n");
			return E_NOINTERFACE;
		}
		m_pIBDA_DeviceControl = pDeviceControl;
	}

	// Control Node 取得
	{
		CDSEnumNodes DSEnumNodes(m_pTunerDevice);

		// IBDA_FrequencyFilter / IBDA_SignalStatistics / IBDA_LNBInfo / IBDA_DiseqCommand
		{
			ULONG NodeTypeTuner = DSEnumNodes.findControlNode(__uuidof(IBDA_FrequencyFilter));
			if (NodeTypeTuner != -1) {
				OutputDebug(L"Found RF Tuner Node. NodeType=%ld.\n", NodeTypeTuner);
				CComPtr<IUnknown> pControlNodeTuner;
				if (FAILED(hr = DSEnumNodes.getControlNode(NodeTypeTuner, &pControlNodeTuner))) {
					OutputDebug(L"Fail to get control node.\n");
				}
				else {
					CComQIPtr<IBDA_FrequencyFilter> pIBDA_FrequencyFilter(pControlNodeTuner);
					if (pIBDA_FrequencyFilter) {
						m_pIBDA_FrequencyFilter = pIBDA_FrequencyFilter;
						OutputDebug(L"  Found IBDA_FrequencyFilter.\n");
					}
					CComQIPtr<IBDA_SignalStatistics> pIBDA_SignalStatistics(pControlNodeTuner);
					if (pIBDA_SignalStatistics) {
						m_pIBDA_SignalStatistics = pIBDA_SignalStatistics;
						OutputDebug(L"  Found IBDA_SignalStatistics.\n");
					}
					CComQIPtr<IBDA_LNBInfo> pIBDA_LNBInfo(pControlNodeTuner);
					if (pIBDA_LNBInfo) {
						m_pIBDA_LNBInfo = pIBDA_LNBInfo;
						OutputDebug(L"  Found IBDA_LNBInfo.\n");
					}
					CComQIPtr<IBDA_DiseqCommand> pIBDA_DiseqCommand(pControlNodeTuner);
					if (pIBDA_DiseqCommand) {
						m_pIBDA_DiseqCommand = pIBDA_DiseqCommand;
						OutputDebug(L"  Found IBDA_DiseqCommand.\n");
					}
				}
			}
		}

		// IBDA_DigitalDemodulator (DD の driver は Demodulator Node には IBDA_SignalStatistics interface を持たない)
		{
			ULONG NodeTypeDemod = DSEnumNodes.findControlNode(__uuidof(IBDA_DigitalDemodulator));
			if (NodeTypeDemod != -1) {
				OutputDebug(L"Found Demodulator Node. NodeType=%ld.\n", NodeTypeDemod);
				CComPtr<IUnknown> pControlNodeDemod;
				if (FAILED(hr = DSEnumNodes.getControlNode(NodeTypeDemod, &pControlNodeDemod))) {
					OutputDebug(L"Fail to get control node.\n");
				}
				else {
					CComQIPtr<IBDA_DigitalDemodulator> pIBDA_DigitalDemodulator(pControlNodeDemod);
					if (pIBDA_DigitalDemodulator) {
						m_pIBDA_DigitalDemodulator = pIBDA_DigitalDemodulator;
						OutputDebug(L"  Found IBDA_DigitalDemodulator.\n");
					}
				}
			}
		}
	}

	// KSPROPERTY_DD_BDA_SELECT_STANDARD がサポートされているか確認
	{
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

	// KSPROPERTY_DD_BDA_SELECT_STREAM がサポートされているか確認
	{
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

	/*
	// KSMETHODSETID_BdaTSSelector がサポートされているか
	{
		KSM_NODE Method = {};
		DWORD Type = 0;
		ULONG BytesReturned = 0;
		Method.Method = { KSMETHODSETID_BdaTSSelector, KSMETHOD_BDA_TS_SELECTOR_SETTSID, KSMETHOD_TYPE_BASICSUPPORT };
		Method.NodeId = 0;
		if (FAILED(hr = m_pControlTunerOutputPin->KsMethod((PKSMETHOD)&Method, sizeof(Method), &Type, sizeof(Type), &BytesReturned))) {
			switch (hr) {
			case E_NOTIMPL:
			case E_PROP_SET_UNSUPPORTED:
				OutputDebug(L"KSMETHODSETID_BdaTSSelector method is not supported.\n");
				break;

			case E_PROP_ID_UNSUPPORTED:
				OutputDebug(L"KSMETHOD_BDA_TS_SELECTOR_SETTSID method ID is not supported.\n");
				break;

			default:
				OutputDebug(L"KSMETHOD_BDA_TS_SELECTOR_SETTSID:KSPROPERTY_TYPE_BASICSUPPORT: Fail to IKsControl::KsProperty() function.\n");
				break;
			}
		}
		else {
			OutputDebug(L"KSMETHODSETID_BdaTSSelector::KSMETHOD_BDA_TS_SELECTOR_SETTSID: Type=%ld.\n", Type);
		}
	}

	// KSPROPERTYSET_DD_BDA_UNKNOWN_A606 がサポートされているか
	{
		KSP_NODE Prop = {};
		DWORD TypeSupport = 0;
		ULONG BytesReturned = 0;
		Prop.Property = { KSPROPERTYSET_DD_BDA_UNKNOWN_A606, 0, KSPROPERTY_TYPE_BASICSUPPORT };
		Prop.NodeId = 0;
		if (FAILED(hr = m_pControlTunerFilter->KsProperty((PKSPROPERTY)&Prop, sizeof(Prop), &TypeSupport, sizeof(TypeSupport), &BytesReturned))) {
			switch (hr) {
			case E_NOTIMPL:
			case E_PROP_SET_UNSUPPORTED:
				OutputDebug(L"KSPROPERTYSET_DD_BDA_UNKNOWN_A606 property set is not supported.\n");
				break;

			case E_PROP_ID_UNSUPPORTED:
				OutputDebug(L"0 property ID is not supported.\n");
				break;

			default:
				OutputDebug(L"0:KSPROPERTY_TYPE_BASICSUPPORT: Fail to IKsControl::KsProperty() function.\n");
				break;
			}
		}
		else {
			OutputDebug(L"KSPROPERTYSET_DD_BDA_UNKNOWN_A606::0: TypeSupport=%ld.\n", TypeSupport);
		}
	}

	// KSPROPERTYSET_DD_BDA_UNKNOWN_A502 がサポートされているか
	{
		KSP_NODE Prop = {};
		DWORD TypeSupport = 0;
		ULONG BytesReturned = 0;
		Prop.Property = { KSPROPERTYSET_DD_BDA_UNKNOWN_A502, 0, KSPROPERTY_TYPE_BASICSUPPORT };
		Prop.NodeId = 0;
		if (FAILED(hr = m_pControlTunerFilter->KsProperty((PKSPROPERTY)&Prop, sizeof(Prop), &TypeSupport, sizeof(TypeSupport), &BytesReturned))) {
			switch (hr) {
			case E_NOTIMPL:
			case E_PROP_SET_UNSUPPORTED:
				OutputDebug(L"KSPROPERTYSET_DD_BDA_UNKNOWN_A502 property set is not supported.\n");
				break;

			case E_PROP_ID_UNSUPPORTED:
				OutputDebug(L"0 property ID is not supported.\n");
				break;

			default:
				OutputDebug(L"0:KSPROPERTY_TYPE_BASICSUPPORT: Fail to IKsControl::KsProperty() function.\n");
				break;
			}
		}
		else {
			OutputDebug(L"KSPROPERTYSET_DD_BDA_UNKNOWN_A502::0: TypeSupport=%ld.\n", TypeSupport);
		}
	}
	*/

	return S_OK;
}

const HRESULT CDDSpecials::LockChannel(const TuningParam* pTuningParam)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	if (!m_pIBDA_SignalStatistics) {
		return E_FAIL;
	}

	HRESULT hr;
	SpectralInversion eSpectralInversion = BDA_SPECTRAL_INVERSION_NOT_SET;
	FECMethod eInnerFECMethod = BDA_FEC_METHOD_NOT_SET;
	BinaryConvolutionCodeRate eInnerFECRate = BDA_BCC_RATE_NOT_SET;
	ModulationType eModulationType = BDA_MOD_NOT_SET;
	FECMethod eOuterFECMethod = BDA_FEC_METHOD_NOT_SET;
	BinaryConvolutionCodeRate eOuterFECRate = BDA_BCC_RATE_NOT_SET;
	ULONG SymbolRate = (ULONG)-1L;

	OutputDebug(L"LockChannel: Start.\n");

	BOOL success = FALSE;
	::EnterCriticalSection(&m_CriticalSection);
	do {
		ULONG State;
		if (FAILED(hr = m_pIBDA_DeviceControl->GetChangeState(&State))) {
			OutputDebug(L"  Fail to IBDA_DeviceControl::GetChangeState() function. ret=0x%08lx\n", hr);
			break;
		}
		// ペンディング状態のトランザクションがあるか確認
		if (State == BDA_CHANGES_PENDING) {
			OutputDebug(L"  Some changes are pending. Trying CommitChanges.\n");
			// トランザクションのコミット
			if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
				OutputDebug(L"    Fail to CommitChanges. ret=0x%08lx\n", hr);
			}
			else {
				OutputDebug(L"    Succeeded to CommitChanges.\n");
			}
		}

		// トランザクション開始通知
		if (FAILED(hr = m_pIBDA_DeviceControl->StartChanges())) {
			OutputDebug(L"  Fail to IBDA_DeviceControl::StartChanges() function. ret=0x%08lx\n", hr);
			break;
		}

		// 信号規格（ISDB-T/ISDB-S等）をセット
		{
			KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_SET);
			ULONG val = m_TuningData.GetSignalStandard(pTuningParam->IniSpaceID);
			OutputDebug(L"  Trying to set SelectStandard. val=%ld.\n", val);
			if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), NULL))) {
				OutputDebug(L"  Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET SelectStandard function. ret=0x%08lx\n", hr);
				break;
			}
		}

		// IBDA_DiseqCommand
		if (m_pIBDA_DiseqCommand) {
			// DiseqLNBSourceを設定
			if (pTuningParam->Antenna.DiSEqC == -1L || pTuningParam->Antenna.DiSEqC == 0L) {
				m_pIBDA_DiseqCommand->put_EnableDiseqCommands(FALSE);
				m_pIBDA_DiseqCommand->put_DiseqLNBSource((ULONG)BDA_LNB_SOURCE_NOT_SET);
			}
			else {
				m_pIBDA_DiseqCommand->put_EnableDiseqCommands(TRUE);
				m_pIBDA_DiseqCommand->put_DiseqLNBSource((ULONG)pTuningParam->Antenna.DiSEqC);
			}
			m_pIBDA_DiseqCommand->put_DiseqUseToneBurst(FALSE);
			m_pIBDA_DiseqCommand->put_DiseqRepeats(0UL);
		}

		// IBDA_LNBInfo
		if (m_pIBDA_LNBInfo) {
			// LNB 周波数を設定
			m_pIBDA_LNBInfo->put_LocalOscilatorFrequencyHighBand((ULONG)pTuningParam->Antenna.HighOscillator);
			m_pIBDA_LNBInfo->put_LocalOscilatorFrequencyLowBand((ULONG)pTuningParam->Antenna.LowOscillator);

			// LNBスイッチの周波数を設定
			if (pTuningParam->Antenna.LNBSwitch != -1L) {
				// LNBSwitch周波数の設定がされている
				m_pIBDA_LNBInfo->put_HighLowSwitchFrequency((ULONG)pTuningParam->Antenna.LNBSwitch);
			}
			else {
				// 10GHzを設定しておけばHigh側に、20GHzを設定しておけばLow側に切替わるはず
				m_pIBDA_LNBInfo->put_HighLowSwitchFrequency((pTuningParam->Antenna.Tone != 0L) ? 10000000UL : 20000000UL);
			}
		}

		// IBDA_DigitalDemodulator
		if (m_pIBDA_DigitalDemodulator) {
			// 位相変調スペクトル反転の種類
			eSpectralInversion = BDA_SPECTRAL_INVERSION_AUTOMATIC;
			m_pIBDA_DigitalDemodulator->put_SpectralInversion(&eSpectralInversion);

			// 内部前方誤り訂正のタイプを設定
			eInnerFECMethod = pTuningParam->Modulation.InnerFEC;
			m_pIBDA_DigitalDemodulator->put_InnerFECMethod(&eInnerFECMethod);

			// 内部 FEC レートを設定
			eInnerFECRate = pTuningParam->Modulation.InnerFECRate;
			m_pIBDA_DigitalDemodulator->put_InnerFECRate(&eInnerFECRate);

			// 変調タイプを設定
			eModulationType = pTuningParam->Modulation.Modulation;
			m_pIBDA_DigitalDemodulator->put_ModulationType(&eModulationType);

			// 外部前方誤り訂正のタイプを設定
			eOuterFECMethod = pTuningParam->Modulation.OuterFEC;
			m_pIBDA_DigitalDemodulator->put_OuterFECMethod(&eOuterFECMethod);

			// 外部 FEC レートを設定
			eOuterFECRate = pTuningParam->Modulation.OuterFECRate;
			m_pIBDA_DigitalDemodulator->put_OuterFECRate(&eOuterFECRate);

			// シンボル レートを設定
			SymbolRate = (ULONG)pTuningParam->Modulation.SymbolRate;
			m_pIBDA_DigitalDemodulator->put_SymbolRate(&SymbolRate);
		}

		// IBDA_FrequencyFilter
		if (m_pIBDA_FrequencyFilter) {
			// 周波数の単位(Hz)を設定
			m_pIBDA_FrequencyFilter->put_FrequencyMultiplier(1000UL);

			// 信号の偏波を設定
			Polarisation pol = pTuningParam->Polarisation;
			enumLNBPowerMode lnbMode = m_TuningData.GetLNBPowerMode(pTuningParam->IniSpaceID);
			switch (lnbMode) {
			case enumLNBPowerMode::eLNBPowerModeForceOff:
				pol = Polarisation::BDA_POLARISATION_NOT_DEFINED;
				break;
			case enumLNBPowerMode::eLNBPowerModeForce13V:
				if (pol >= Polarisation::BDA_POLARISATION_CIRCULAR_L) {
					pol = Polarisation::BDA_POLARISATION_CIRCULAR_R;
				}
				else {
					pol = Polarisation::BDA_POLARISATION_LINEAR_V;
				}
				break;
			case enumLNBPowerMode::eLNBPowerModeForce18V:
				if (pol >= Polarisation::BDA_POLARISATION_CIRCULAR_L) {
					pol = Polarisation::BDA_POLARISATION_CIRCULAR_L;
				}
				else {
					pol = Polarisation::BDA_POLARISATION_LINEAR_H;
				}
				break;
			}
			m_pIBDA_FrequencyFilter->put_Polarity(pol);

			// 周波数の帯域幅 (MHz)を設定
			m_pIBDA_FrequencyFilter->put_Bandwidth((ULONG)pTuningParam->Modulation.BandWidth);

			// RF 信号の周波数を設定
			m_pIBDA_FrequencyFilter->put_Frequency((ULONG)pTuningParam->Frequency);
		}

		// TSID等をセット
		{
			SelectStream writeStream;
			ULONG ss = m_TuningData.GetSignalStandard(pTuningParam->IniSpaceID);
			BOOL needWrite = FALSE;
			switch (ss) {
			case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
				writeStream.ISDBC.TSID = m_bDisableTSMF ? 0xFFFFU : (USHORT)pTuningParam->TSID;
				writeStream.ISDBC.ONID = m_bSelectStreamRelative ? 0U : (USHORT)pTuningParam->ONID;
				needWrite = TRUE;
				break;
			case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
				if (pTuningParam->TSID == -1L) {
					writeStream.ISDBS.TSID = 0U;
					writeStream.ISDBS.Flag = 0U;
				}
				else {
					writeStream.ISDBS.TSID = (USHORT)pTuningParam->TSID;
					writeStream.ISDBS.Flag = m_bSelectStreamRelative ? 0U : 1U;
				}
				needWrite = TRUE;
				break;
			case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
				if (pTuningParam->SID != -1L) {
					writeStream.DVBS2.StreamID = (USHORT)pTuningParam->SID;
					needWrite = TRUE;
				}
				break;
			}
			if (needWrite) {
				KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStream(KSPROPERTY_DD_BDA_SELECT_STREAM, KSPROPERTY_TYPE_SET);
				OutputDebug(L"  Trying to set SelectStream. val=0x%08lx.\n", writeStream.alignment);
				if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &writeStream, sizeof(writeStream), NULL))) {
					OutputDebug(L"  Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET SelectStream function. ret=0x%08lx\n", hr);
					break;
				}
			}
		}

		// トランザクションのコミット
		if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
			OutputDebug(L"  Fail to IBDA_DeviceControl::CommitChanges() function. ret=0x%08lx\n", hr);
			// 失敗したら全ての変更を取り消す
			hr = m_pIBDA_DeviceControl->StartChanges();
			hr = m_pIBDA_DeviceControl->CommitChanges();
			break;
		}
		OutputDebug(L"  Succeeded to IBDA_DeviceControl::CommitChanges() function.\n");

		BOOLEAN locked = 0;
		// 実際のチューニングはここで行われる
		hr = m_pIBDA_SignalStatistics->get_SignalLocked(&locked);
		OutputDebug(L"  SignalLocked=%d.\n", locked);

		OutputDebug(L"LockChannel: Complete.\n");
		if (locked)
			success = TRUE;

	} while (0);
	::LeaveCriticalSection(&m_CriticalSection);

	return success ? S_OK : E_FAIL;
}

const HRESULT CDDSpecials::ReadIniFile(const WCHAR* szIniFilePath)
{
	const CIniFileAccess::Map mapSignalStandard = {
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

	const CIniFileAccess::Map mapGetSignalStrengthFunction = {
		{ L"",        -1 },
		{ L"SIGNALSTRENGTH", KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_STRENGTH },
		{ L"SIGNALQUALITY",  KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_QUALITY },
		{ L"SNR",            KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_SNR },
		{ L"BER",            KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_BER },
		{ L"BITRATE",        KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_BITRATE },
		{ L"OFFSET",         KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_OFFSET },
		{ L"LOSTCOUNT",      KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_LOSTCOUNT },
		{ L"STANDARD",       KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_STANDARD },
		{ L"SYMBOLRATE",     KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_SYMBOLRATE },
	};

	const CIniFileAccess::Map mapLNBPowerMode = {
		{ L"OFF",  enumLNBPowerMode::eLNBPowerModeForceOff },
		{ L"13V",  enumLNBPowerMode::eLNBPowerModeForce13V },
		{ L"18V",  enumLNBPowerMode::eLNBPowerModeForce18V },
		{ L"AUTO", enumLNBPowerMode::eLNBPowerModeAuto },
	};

	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"DD");

	// Select Stream 処理でTSIDを相対でセットする
	m_bSelectStreamRelative = IniFileAccess.ReadKeyB(L"SelectStreamRelative", FALSE);

	// Select Stream 処理でTSMFを無効にする
	m_bDisableTSMF = IniFileAccess.ReadKeyB(L"DisableTSMF", FALSE);

	// LNB Powerを強制的にOFFにする
	BOOL bLNBPowerOff = IniFileAccess.ReadKeyB(L"LNBPowerOff", FALSE);

	// GetSignalStrength 関数で返す値
	m_nGetSignalStrengthFunction = (KSPROPERTY_DD_BDA_SIGNAL_INFO)IniFileAccess.ReadKeyIValueMap(L"GetSignalStrengthFunction", -1, &mapGetSignalStrengthFunction);

	// チューニング空間00～99の設定を読込
	for (DWORD space = 0; space < 100; space++) {
		std::wstring section = common::WStringPrintf(L"TUNINGSPACE%02d", space);
		if (IniFileAccess.ReadSection(section.c_str()) <= 0) {
			continue;
		}
		IniFileAccess.CreateSectionData();
		// 信号規格
		DD_SIGNAL_STANDARD nSelectStandard = (DD_SIGNAL_STANDARD)IniFileAccess.ReadKeyIValueMapSectionData(L"DD_SelectStandard", DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED, &mapSignalStandard);
		// LNB Power Mode
		enumLNBPowerMode LNBPowerMode = (enumLNBPowerMode)IniFileAccess.ReadKeyIValueMapSectionData(L"DD_LNBPowerMode", bLNBPowerOff ? enumLNBPowerMode::eLNBPowerModeForceOff : enumLNBPowerMode::eLNBPowerModeAuto, &mapLNBPowerMode);
		m_TuningData.Regist(space, nSelectStandard, LNBPowerMode);
	}

	return S_OK;
}

const HRESULT CDDSpecials::GetSignalStrength(float* fVal)
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
	::EnterCriticalSection(&m_CriticalSection);
	hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropSignalInfo, sizeof(PropSignalInfo), &signalInfo, sizeof(signalInfo), &BytesReturned);
	::LeaveCriticalSection(&m_CriticalSection);
	if (FAILED(hr)) {
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
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_BITRATE:
		*fVal = (float)signalInfo.Bitrate;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_OFFSET:
		*fVal = (float)signalInfo.Offset;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_LOSTCOUNT:
		*fVal = (float)signalInfo.LostCount;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_STANDARD:
		*fVal = (float)signalInfo.Standard;
		break;
	case KSPROPERTY_DD_BDA_SIGNAL_INFO::KSPROPERTY_DD_BDA_SIGNAL_SYMBOLRATE:
		*fVal = (float)signalInfo.SymbolRate;
		break;
	}

	return S_OK;
}

const HRESULT CDDSpecials::PostLockChannel(const TuningParam* /*pTuningParam*/)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	HRESULT hr;

	OutputDebug(L"PostLockChannel: Start.\n");

	// 信号規格（ISDB-T/ISDB-S等）の確認（Logのみ）
	{
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_GET);
		ULONG val = 0;
		ULONG BytesReturned = 0;
		PropStandard.SetFlags(KSPROPERTY_TYPE_GET);
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), &BytesReturned))) {
			OutputDebug(L"  Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStandard function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		OutputDebug(L"  Succeeded to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStandard function. bytes=%ld, val=%ld\n", BytesReturned, val);
	}

	// TSID等の確認（Logのみ）
	{
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStream(KSPROPERTY_DD_BDA_SELECT_STREAM, KSPROPERTY_TYPE_GET);
		ULONG val = 0;
		ULONG BytesReturned = 0;
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &val, sizeof(val), &BytesReturned))) {
			OutputDebug(L"  Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStream function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		OutputDebug(L"  Succeeded to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStream function. bytes=%ld, val=0x%08lx\n", BytesReturned, val);
	}

	OutputDebug(L"PostLockChannel: Complete.\n");

	return S_OK;
}

void CDDSpecials::Release(void)
{
	delete this;
}
