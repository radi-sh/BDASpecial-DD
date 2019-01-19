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
		// ���W���[���n���h���ۑ�
		hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// �f�o�b�O���O�t�@�C���̃N���[�Y
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

	// DebugLog���L�^���邩�ǂ���
	if (IniFileAccess.ReadKeyB(L"DD", L"DebugLog", FALSE)) {
		// DebugLog�̃t�@�C�����擾
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

	// IBDA_DeviceControl
	{
		CComQIPtr<IBDA_DeviceControl> pDeviceControl(m_pTunerDevice);
		if (!pDeviceControl) {
			OutputDebug(L"Can not get IBDA_DeviceControl.\n");
			return E_NOINTERFACE;
		}
		m_pIBDA_DeviceControl = pDeviceControl;
	}

	// �`���[�i�[�f�o�C�X�� output pin��������
	{
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
						// output pin �� IKsPropertySet ���擾
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
	}
	if (!m_pControlTunerOutputPin) {
		OutputDebug(L"Fail to get IKsPropertySet of tuner output pin.\n");
		return E_NOINTERFACE;
	}

	// IBDA_LNBInfo / IBDA_DigitalDemodulator / IBDA_FrequencyFilter / IBDA_DiseqCommand / IBDA_SignalStatistics
	{
		CComQIPtr<IBDA_Topology> pIBDA_Topology(m_pTunerDevice);
		if (!pIBDA_Topology) {
			OutputDebug(L"Fail to get IBDA_Topology interface.\n");
			return E_NOINTERFACE;
		}
		OutputDebug(L"Succeeded to get IBDA_Topology interface.\n");

		ULONG NodeTypes;
		ULONG NodeType[32];
		if (FAILED(hr = pIBDA_Topology->GetNodeTypes(&NodeTypes, 32, NodeType))) {
			OutputDebug(L"Fail to get NodeTypes.\n");
			return E_NOINTERFACE;
		}
		OutputDebug(L"Succeeded to get NodeTypes. Num=%ld.\n", NodeTypes);
		for (ULONG i = 0; i < NodeTypes; i++) {
			CComPtr<IUnknown> pControlNode;
			if (SUCCEEDED(hr = pIBDA_Topology->GetControlNode(0UL, 1UL, NodeType[i], &pControlNode))) {
				OutputDebug(L"GetControlNode(0, 1, NodeType[%ld]=%ld).\n", i, NodeType[i]);
				CComQIPtr<IBDA_LNBInfo> pIBDA_LNBInfo(pControlNode);
				CComQIPtr<IBDA_FrequencyFilter> pIBDA_FrequencyFilter(pControlNode);
				CComQIPtr<IBDA_DiseqCommand> pIBDA_DiseqCommand(pControlNode);
				CComQIPtr<IBDA_DigitalDemodulator> pIBDA_DigitalDemodulator(pControlNode);
				CComQIPtr<IBDA_SignalStatistics> pIBDA_SignalStatistics(pControlNode);
				switch(NodeType[i]) {
				case 0:
					if (pIBDA_LNBInfo) {
						m_pIBDA_LNBInfo = pIBDA_LNBInfo;
						OutputDebug(L"  Found IBDA_LNBInfo.\n");
					}
					if (pIBDA_FrequencyFilter) {
						m_pIBDA_FrequencyFilter = pIBDA_FrequencyFilter;
						OutputDebug(L"  Found IBDA_FrequencyFilter.\n");
					}
					if (pIBDA_DiseqCommand) {
						m_pIBDA_DiseqCommand = pIBDA_DiseqCommand;
						OutputDebug(L"  Found IBDA_DiseqCommand.\n");
					}
					if (pIBDA_SignalStatistics) {
						m_pIBDA_SignalStatistics = pIBDA_SignalStatistics;
						OutputDebug(L"  Found IBDA_SignalStatistics.\n");
					}
					break;
				case 1:
					if (pIBDA_DigitalDemodulator) {
						m_pIBDA_DigitalDemodulator = pIBDA_DigitalDemodulator;
						OutputDebug(L"  Found IBDA_DigitalDemodulator.\n");
					}
					break;
				}
				if (m_pIBDA_LNBInfo && m_pIBDA_DigitalDemodulator && m_pIBDA_FrequencyFilter && m_pIBDA_DiseqCommand && m_pIBDA_SignalStatistics) {
					OutputDebug(L"All control nodes was found.\n");
					break;
				}
			}
		}
	}

	// KSPROPERTY_DD_BDA_SELECT_STANDARD ���T�|�[�g����Ă��邩�m�F
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

	// KSPROPERTY_DD_BDA_SELECT_STREAM ���T�|�[�g����Ă��邩�m�F
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

const HRESULT CDDSpecials::LockChannel(const TuningParam *pTuningParam)
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

	ULONG State;
	if (FAILED(hr = m_pIBDA_DeviceControl->GetChangeState(&State))) {
		OutputDebug(L"  Fail to IBDA_DeviceControl::GetChangeState() function. ret=0x%08lx\n", hr);
		return E_FAIL;
	}
	// �y���f�B���O��Ԃ̃g�����U�N�V���������邩�m�F
	if (State == BDA_CHANGES_PENDING) {
		OutputDebug(L"  Some changes are pending. Trying CommitChanges.\n");
		// �g�����U�N�V�����̃R�~�b�g
		if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
			OutputDebug(L"    Fail to CommitChanges. ret=0x%08lx\n", hr);
		}
		else {
			OutputDebug(L"    Succeeded to CommitChanges.\n");
		}
	}

	// �g�����U�N�V�����J�n�ʒm
	if (FAILED(hr = m_pIBDA_DeviceControl->StartChanges())) {
		OutputDebug(L"  Fail to IBDA_DeviceControl::StartChanges() function. ret=0x%08lx\n", hr);
		return E_FAIL;
	}

	// �M���K�i�iISDB-T/ISDB-S���j���Z�b�g
	{
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_SET);
		ULONG val = m_TuningData.GetSignalStandard(pTuningParam->IniSpaceID);
		OutputDebug(L"  Trying to set SelectStandard. val=%ld.\n", val);
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), NULL))) {
			OutputDebug(L"  Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET SelectStandard function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
	}

	// IBDA_DiseqCommand
	if (m_pIBDA_DiseqCommand) {
		// DiseqLNBSource��ݒ�
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
		// LNB ���g����ݒ�
		m_pIBDA_LNBInfo->put_LocalOscilatorFrequencyHighBand((ULONG)pTuningParam->Antenna.HighOscillator);
		m_pIBDA_LNBInfo->put_LocalOscilatorFrequencyLowBand((ULONG)pTuningParam->Antenna.LowOscillator);

		// LNB�X�C�b�`�̎��g����ݒ�
		if (pTuningParam->Antenna.LNBSwitch != -1L) {
			// LNBSwitch���g���̐ݒ肪����Ă���
			m_pIBDA_LNBInfo->put_HighLowSwitchFrequency((ULONG)pTuningParam->Antenna.LNBSwitch);
		}
		else {
			// 10GHz��ݒ肵�Ă�����High���ɁA20GHz��ݒ肵�Ă�����Low���ɐؑւ��͂�
			m_pIBDA_LNBInfo->put_HighLowSwitchFrequency((pTuningParam->Antenna.Tone != 0L) ? 10000000UL : 20000000UL);
		}
	}

	// IBDA_DigitalDemodulator
	if (m_pIBDA_DigitalDemodulator) {
		// �ʑ��ϒ��X�y�N�g�����]�̎��
		eSpectralInversion = BDA_SPECTRAL_INVERSION_AUTOMATIC;
		m_pIBDA_DigitalDemodulator->put_SpectralInversion(&eSpectralInversion);

		// �����O���������̃^�C�v��ݒ�
		eInnerFECMethod = pTuningParam->Modulation.InnerFEC;
		m_pIBDA_DigitalDemodulator->put_InnerFECMethod(&eInnerFECMethod);

		// ���� FEC ���[�g��ݒ�
		eInnerFECRate = pTuningParam->Modulation.InnerFECRate;
		m_pIBDA_DigitalDemodulator->put_InnerFECRate(&eInnerFECRate);

		// �ϒ��^�C�v��ݒ�
		eModulationType = pTuningParam->Modulation.Modulation;
		m_pIBDA_DigitalDemodulator->put_ModulationType(&eModulationType);

		// �O���O���������̃^�C�v��ݒ�
		eOuterFECMethod = pTuningParam->Modulation.OuterFEC;
		m_pIBDA_DigitalDemodulator->put_OuterFECMethod(&eOuterFECMethod);

		// �O�� FEC ���[�g��ݒ�
		eOuterFECRate = pTuningParam->Modulation.OuterFECRate;
		m_pIBDA_DigitalDemodulator->put_OuterFECRate(&eOuterFECRate);

		// �V���{�� ���[�g��ݒ�
		SymbolRate = (ULONG)pTuningParam->Modulation.SymbolRate;
		m_pIBDA_DigitalDemodulator->put_SymbolRate(&SymbolRate);
	}

	// IBDA_FrequencyFilter
	if (m_pIBDA_FrequencyFilter) {
		// ���g���̒P��(Hz)��ݒ�
		m_pIBDA_FrequencyFilter->put_FrequencyMultiplier(1000UL);

		// �M���̕Δg��ݒ�
		m_pIBDA_FrequencyFilter->put_Polarity(m_bLNBPowerOff ? (Polarisation)0L : pTuningParam->Polarisation);

		// ���g���̑ш敝 (MHz)��ݒ�
		m_pIBDA_FrequencyFilter->put_Bandwidth((ULONG)pTuningParam->Modulation.BandWidth);

		// RF �M���̎��g����ݒ�
		m_pIBDA_FrequencyFilter->put_Frequency((ULONG)pTuningParam->Frequency);
	}

	// TSID�����Z�b�g
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
				return E_FAIL;
			}
		}
	}

	// �g�����U�N�V�����̃R�~�b�g
	if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
		OutputDebug(L"  Fail to IBDA_DeviceControl::CommitChanges() function. ret=0x%08lx\n", hr);
		// ���s������S�Ă̕ύX��������
		hr = m_pIBDA_DeviceControl->StartChanges();
		hr = m_pIBDA_DeviceControl->CommitChanges();
		return E_FAIL;
	}
	OutputDebug(L"  Succeeded to IBDA_DeviceControl::CommitChanges() function.\n");

	BOOLEAN locked = 0;
	// ���ۂ̃`���[�j���O�͂����ōs����
	hr = m_pIBDA_SignalStatistics->get_SignalLocked(&locked);
	OutputDebug(L"  SignalLocked=%d.\n", locked);

	OutputDebug(L"LockChannel: Complete.\n");

	return locked ? S_OK : E_FAIL;
}

const HRESULT CDDSpecials::SetLNBPower(bool bActive)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::ReadIniFile(const WCHAR *szIniFilePath)
{
	static const std::map<const std::wstring, const int, std::less<>> mapSignalStandard = {
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

	static const std::map<const std::wstring, const int, std::less<>> mapGetSignalStrengthFunction = {
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

	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"DD");

	// Select Stream ������TSID�𑊑΂ŃZ�b�g����
	m_bSelectStreamRelative = IniFileAccess.ReadKeyB(L"SelectStreamRelative", FALSE);

	// Select Stream ������TSMF�𖳌��ɂ���
	m_bDisableTSMF = IniFileAccess.ReadKeyB(L"DisableTSMF", FALSE);

	// LNB Power�������I��OFF�ɂ���
	m_bLNBPowerOff = IniFileAccess.ReadKeyB(L"LNBPowerOff", FALSE);

	// GetSignalStrength �֐��ŕԂ��l
	m_nGetSignalStrengthFunction = (KSPROPERTY_DD_BDA_SIGNAL_INFO)IniFileAccess.ReadKeyIValueMap(L"GetSignalStrengthFunction", -1, mapGetSignalStrengthFunction);

	// �`���[�j���O���00�`99�̐ݒ��Ǎ�
	for (DWORD space = 0; space < 100; space++) {
		std::wstring section = common::WStringPrintf(L"TUNINGSPACE%02d", space);
		if (IniFileAccess.ReadSection(section) <= 0) {
			continue;
		}
		IniFileAccess.CreateSectionData();
		DD_SIGNAL_STANDARD ss = (DD_SIGNAL_STANDARD)IniFileAccess.ReadKeyIValueMapSectionData(L"DD_SelectStandard", DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_UNDEFINED, mapSignalStandard);
		m_TuningData.Regist(space, ss);
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

const HRESULT CDDSpecials::PreLockChannel(TuningParam *pTuningParam)
{
	return S_OK;
}

const HRESULT CDDSpecials::PreTuneRequest(const TuningParam *pTuningParam, ITuneRequest *pITuneRequest)
{
	return S_OK;
}

const HRESULT CDDSpecials::PostTuneRequest(const TuningParam * pTuningParam)
{
	return S_OK;
}

const HRESULT CDDSpecials::PostLockChannel(const TuningParam *pTuningParam)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	HRESULT hr;

	OutputDebug(L"PostLockChannel: Start.\n");

	// �M���K�i�iISDB-T/ISDB-S���j�̊m�F�iLog�̂݁j
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

	// TSID���̊m�F�iLog�̂݁j
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
