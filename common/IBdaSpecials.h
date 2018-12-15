//------------------------------------------------------------------------------
// File: IBdaSpecials.h
//   Define interface
//------------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <dshow.h>

// IBdaSpecials interface
////////////////////////////////

class IBdaSpecials
{
public:
	virtual const HRESULT InitializeHook(void) = 0;
	virtual const HRESULT Set22KHz(bool bActive) = 0;
	virtual const HRESULT FinalizeHook(void) = 0;
	// GetSignalState �� LockChannel ���ہX�`���[�i�ŗL�R�[�h���g���ꍇ��
	// �ȉ��̓���g��
	virtual const HRESULT GetSignalState(int *pnStrength, int *pnQuality, int *pnLock) = 0;
	virtual const HRESULT LockChannel(BYTE bySatellite, BOOL bHorizontal,
		unsigned long ulFrequency, BOOL bDvbS2) = 0;
	// LNB
	virtual const HRESULT SetLNBPower(bool bActive) = 0;
	virtual void Release(void) = 0;
};

// Method to create IBdaSpecials interface
////////////////////////////////

#if defined _USRDLL && !defined BONDRIVER_EXPORTS
extern "C" __declspec(dllexport) IBdaSpecials * CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice);
#else
extern "C" __declspec(dllimport) IBdaSpecials * CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice);
#endif
