#include "common.h"

#include "DDSpecials.h"

#include <Windows.h>
#include <string>

#include <dshow.h>
#include "CIniFileAccess.h"
#include "WaitWithMsg.h"

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

	// IBDA_LNBInfo / IBDA_DigitalDemodulator / IBDA_FrequencyFilter / IBDA_SignalStatistics
	{
		CComQIPtr<IBDA_Topology> pIBDA_Topology(m_pTunerDevice);
		if (!pIBDA_Topology) {
			OutputDebug(L"LockChannel: Fail to get IBDA_Topology interface.\n");
			return E_NOINTERFACE;
		}
		OutputDebug(L"LockChannel: Succeeded to get IBDA_Topology interface.\n");

		ULONG NodeTypes;
		ULONG NodeType[32];
		if (FAILED(hr = pIBDA_Topology->GetNodeTypes(&NodeTypes, 32, NodeType))) {
			OutputDebug(L"LockChannel: Fail to get NodeTypes.\n");
			return E_NOINTERFACE;
		}
		OutputDebug(L"LockChannel: Succeeded to get NodeTypes. Num=%ld.\n", NodeTypes);
		for (ULONG i = 0; i < NodeTypes; i++) {
			CComPtr<IUnknown> pControlNode;
			if (SUCCEEDED(hr = pIBDA_Topology->GetControlNode(0UL, 1UL, NodeType[i], &pControlNode))) {
				OutputDebug(L"LockChannel: GetControlNode(0, 1, NodeType[%ld]).\n", i);
				CComQIPtr<IBDA_LNBInfo> pIBDA_LNBInfo(pControlNode);
				if (!m_pIBDA_LNBInfo) {
					m_pIBDA_LNBInfo = pIBDA_LNBInfo;
					OutputDebug(L"  Found IBDA_LNBInfo.\n");
				}
				CComQIPtr<IBDA_DigitalDemodulator> pIBDA_DigitalDemodulator(pControlNode);
				if (!m_pIBDA_DigitalDemodulator) {
					m_pIBDA_DigitalDemodulator = pIBDA_DigitalDemodulator;
					OutputDebug(L"  Found IBDA_DigitalDemodulator.\n");
				}
				CComQIPtr<IBDA_FrequencyFilter> pIBDA_FrequencyFilter(pControlNode);
				if (!m_pIBDA_FrequencyFilter) {
					m_pIBDA_FrequencyFilter = pIBDA_FrequencyFilter;
					OutputDebug(L"  Found IBDA_FrequencyFilter.\n");
				}
				CComQIPtr<IBDA_SignalStatistics> pIBDA_SignalStatistics(pControlNode);
				if (!m_pIBDA_SignalStatistics) {
					m_pIBDA_SignalStatistics = pIBDA_SignalStatistics;
					OutputDebug(L"  Found IBDA_SignalStatistics.\n");
				}
				if (m_pIBDA_LNBInfo && m_pIBDA_DigitalDemodulator && m_pIBDA_FrequencyFilter && m_pIBDA_SignalStatistics) {
					OutputDebug(L"LockChannel: All control nodes was found.\n");
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

const HRESULT CDDSpecials::LockChannel(const TuningParam *pTuningParm)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	HRESULT hr;

	// �g�����U�N�V�����J�n�ʒm
	if (FAILED(hr = m_pIBDA_DeviceControl->StartChanges())) {
		OutputDebug(L"LockChannel: Fail to IBDA_DeviceControl::StartChanges() function. ret=0x%08lx\n", hr);
		return E_FAIL;
	}

	if (m_pIBDA_LNBInfo) {
		OutputDebug(L"LockChannel: IBDA_LNBInfo\n");
		// LNB ���g����ݒ�
		if (pTuningParm->Antenna->HighOscillator != -1) {
			m_pIBDA_LNBInfo->put_LocalOscilatorFrequencyHighBand(pTuningParm->Antenna->HighOscillator);
		}
		if (pTuningParm->Antenna->LowOscillator != -1) {
			m_pIBDA_LNBInfo->put_LocalOscilatorFrequencyLowBand(pTuningParm->Antenna->LowOscillator);
		}

		// LNB�X�C�b�`�̎��g����ݒ�
		if (pTuningParm->Antenna->LNBSwitch != -1) {
			// LNBSwitch���g���̐ݒ肪����Ă���
			m_pIBDA_LNBInfo->put_HighLowSwitchFrequency(pTuningParm->Antenna->LNBSwitch);
		}
		else {
			// 10GHz��ݒ肵�Ă�����High���ɁA20GHz��ݒ肵�Ă�����Low���ɐؑւ��͂�
			m_pIBDA_LNBInfo->put_HighLowSwitchFrequency((pTuningParm->Antenna->Tone != 0) ? 10000000 : 20000000);
		}
	}

	if (m_pIBDA_DigitalDemodulator) {
		OutputDebug(L"LockChannel: IBDA_DigitalDemodulator\n");
		// �ʑ��ϒ��X�y�N�g�����]�̎��
		SpectralInversion eSpectralInversion = BDA_SPECTRAL_INVERSION_AUTOMATIC;
		m_pIBDA_DigitalDemodulator->put_SpectralInversion(&eSpectralInversion);

		// �����O���������̃^�C�v��ݒ�
		FECMethod eInnerFECMethod = pTuningParm->Modulation->InnerFEC;
		m_pIBDA_DigitalDemodulator->put_InnerFECMethod(&eInnerFECMethod);

		// ���� FEC ���[�g��ݒ�
		BinaryConvolutionCodeRate eInnerFECRate = pTuningParm->Modulation->InnerFECRate;
		m_pIBDA_DigitalDemodulator->put_InnerFECRate(&eInnerFECRate);

		// �ϒ��^�C�v��ݒ�
		ModulationType eModulationType = pTuningParm->Modulation->Modulation;
		m_pIBDA_DigitalDemodulator->put_ModulationType(&eModulationType);

		// �O���O���������̃^�C�v��ݒ�
		FECMethod eOuterFECMethod = pTuningParm->Modulation->OuterFEC;
		m_pIBDA_DigitalDemodulator->put_OuterFECMethod(&eOuterFECMethod);

		// �O�� FEC ���[�g��ݒ�
		BinaryConvolutionCodeRate eOuterFECRate = pTuningParm->Modulation->OuterFECRate;
		m_pIBDA_DigitalDemodulator->put_OuterFECRate(&eOuterFECRate);

		// QPSK �V���{�� ���[�g��ݒ�
		ULONG SymbolRate = (ULONG)pTuningParm->Modulation->SymbolRate;
		m_pIBDA_DigitalDemodulator->put_SymbolRate(&SymbolRate);
	}

	if (m_pIBDA_FrequencyFilter) {
		OutputDebug(L"LockChannel: IBDA_FrequencyFilter\n");
		// RF �M���̎��g����ݒ�
		m_pIBDA_FrequencyFilter->put_Frequency((ULONG)pTuningParm->Frequency);

		// �M���̕Δg��ݒ�
		m_pIBDA_FrequencyFilter->put_Polarity(pTuningParm->Polarisation);

		// ���g���̑ш敝 (MHz)��ݒ�
		if (pTuningParm->Modulation->BandWidth != -1) {
			m_pIBDA_FrequencyFilter->put_Bandwidth((ULONG)pTuningParm->Modulation->BandWidth);
		}
	}

	// �M���K�i�iISDB-T/ISDB-S���j���Z�b�g
	{
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_SET);
		ULONG val = m_TuningData.GetSignalStandard(pTuningParm->IniSpaceID);
		OutputDebug(L"LockChannel: trying to set SelectStandard. val=%ld.\n", val);
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), NULL))) {
			OutputDebug(L"LockChannel: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET SelectStandard function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
	}

	// TSID�����Z�b�g
	{
		SelectStream writeStream;
		ULONG ss = m_TuningData.GetSignalStandard(pTuningParm->IniSpaceID);
		switch (ss) {
		case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
			writeStream.ISDBC.TSID = m_bDisableTSMF ? 0xFFFFU : (USHORT)pTuningParm->TSID;
			writeStream.ISDBC.ONID = m_bSelectStreamRelative ? 0U : (USHORT)pTuningParm->ONID;
			break;
		case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
			if (pTuningParm->TSID == -1) {
				writeStream.ISDBS.TSID = 0U;
				writeStream.ISDBS.Flag = 0U;
			}
			else {
				writeStream.ISDBS.TSID = (USHORT)pTuningParm->TSID;
				writeStream.ISDBS.Flag = m_bSelectStreamRelative ? 0U : 1U;
			}
			break;
		case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
			writeStream.DVBS2.StreamID = (USHORT)pTuningParm->SID;
			break;
		}
		switch (ss) {
		case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBC:
		case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_ISDBS:
		case DD_SIGNAL_STANDARD::DD_SIGNAL_STANDARD_DVBS2:
			KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStream(KSPROPERTY_DD_BDA_SELECT_STREAM, KSPROPERTY_TYPE_SET);
			OutputDebug(L"LockChannel: trying to set SelectStream. val=0x%08lx.\n", writeStream.alignment);
			if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &writeStream, sizeof(writeStream), NULL))) {
				OutputDebug(L"LockChannel: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_SET SelectStream function. ret=0x%08lx\n", hr);
				return E_FAIL;
			}
			break;
		}
	}

	// �g�����U�N�V�����̃R�~�b�g
	if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
		OutputDebug(L"LockChannel: Fail to IBDA_DeviceControl::CommitChanges() function. ret=0x%08lx\n", hr);
		// �S�Ă̕ύX��������
		hr = m_pIBDA_DeviceControl->StartChanges();
		hr = m_pIBDA_DeviceControl->CommitChanges();
		return E_FAIL;
	}
	OutputDebug(L"LockChannel: Succeeded to IBDA_DeviceControl::CommitChanges() function.\n");

	// �M���K�i�iISDB-T/ISDB-S���j�̊m�F�iLog�̂݁j
	{
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStandard(KSPROPERTY_DD_BDA_SELECT_STANDARD, KSPROPERTY_TYPE_GET);
		ULONG val = 0;
		ULONG BytesReturned = 0;
		PropStandard.SetFlags(KSPROPERTY_TYPE_GET);
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStandard, sizeof(PropStandard), &val, sizeof(val), &BytesReturned))) {
			OutputDebug(L"LockChannel: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStandard function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		OutputDebug(L"LockChannel: Succeeded to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStandard function. bytes=%ld, val=%ld\n", BytesReturned, val);
	}

	// TSID���̊m�F�iLog�̂݁j
	{
		KSPROPERTY_DD_BDA_DIGITAL_DEMODULATOR_S PropStream(KSPROPERTY_DD_BDA_SELECT_STREAM, KSPROPERTY_TYPE_GET);
		ULONG val = 0;
		ULONG BytesReturned = 0;
		if (FAILED(hr = m_pControlTunerOutputPin->KsProperty((PKSPROPERTY)&PropStream, sizeof(PropStream), &val, sizeof(val), &BytesReturned))) {
			OutputDebug(L"LockChannel: Fail to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStream function. ret=0x%08lx\n", hr);
			return E_FAIL;
		}
		OutputDebug(L"LockChannel: Succeeded to IKsControl::KsProperty() KSPROPERTY_TYPE_GET SelectStream function. bytes=%ld, val=0x%08lx\n", BytesReturned, val);
	}

	if (!m_pIBDA_SignalStatistics) {
		// �m�F�ł��Ȃ��̂ł��̂܂�S_OK��Ԃ�
		return S_OK;
	}

	BOOLEAN locked = 0;
	hr = m_pIBDA_SignalStatistics->get_SignalLocked(&locked);

	return locked ? S_OK : E_FAIL;
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

const HRESULT CDDSpecials::PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest)
{
	return E_NOINTERFACE;
}

const HRESULT CDDSpecials::PostLockChannel(const TuningParam *pTuningParm)
{
	return E_NOINTERFACE;
}

void CDDSpecials::Release(void)
{
	delete this;
}
