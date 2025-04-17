#ifdef _WIN32

#include "OSDisplayInfo.h"

#include <wingdi.h>

#include <algorithm>
#include <codecvt>
#include <locale>
#include <memory>

using namespace std;

namespace ara {
DispRegistryInfo::DispRegistryInfo() : c_Limit_NoneDynamic(c_SPDispRegistryInfo_NDDSz) {
    _Name[0]            = 0;
    _strHWID[0]         = 0;
    _strFriendlyName[0] = 0;
    _strSerial[0]       = 0;
    _strMisc[0]         = 0;
}

void DispRegistryInfo::DefaultEDIDParam() {
    _iEDIDVer = 0;
    _iEDIDRev = 0;
    m_fFilled = FLAG_FILLED_NONE;
}

bool DispRegistryInfo::ParseEDID() {
    DefaultEDIDParam();

    float             r;
    WORD              pxClk;
    BYTE *            pEDID, *pB;
    DispStdTimingInfo tmInfo;
    long              i, l, w, h;
    BYTE              b, iVer, iRev, bDigital, bInterlaced, devWidth, devHeight;
    size_t            qEDID   = _lEDID.size();
    const BYTE        idHdr[] = {0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};

    if (qEDID < 128) return true;
    pEDID = &_lEDID[0];
    if (memcmp(pEDID, idHdr, 8)) return true;

    iVer = _lEDID[18];
    iRev = _lEDID[19];

    _iEDIDVer = iVer;
    _iEDIDRev = iRev;

    if (iVer != 1) return true;

    b         = pEDID[20];
    bDigital  = (b & 0x80) ? TRUE : FALSE;
    devWidth  = pEDID[21];
    devHeight = pEDID[22];

    for (i = 38; (i < 54);) {
        l = (long)pEDID[i++];
        if (l <= 0x1) break;
        b           = pEDID[i++];
        tmInfo.freq = 60 + (long)(b & 0x3f);
        b >>= 6;
        if (b & 0x1)
            r = (b & 0x2) ? 16.0f / 9.0f : 4.0f / 3.0f;
        else
            r = (b & 0x2) ? 5.0f / 4.0f : 16.0f / 10.0f;
        w                  = 248 + (l << 3);
        tmInfo.height      = (long)std::round((float)w / r);
        tmInfo.width       = w;
        tmInfo.ratio       = r;
        tmInfo.bpp         = 32;
        tmInfo.bInterlaced = FALSE;
        tmInfo.tmPxClock   = 0;

        _lStdTimings.push_back(tmInfo);
    }

    for (i = 54; (i < 126); i += 18) {
        pxClk = *((WORD*)(pEDID + i));
        if (pxClk) {
            if (m_fFilled & FLAG_FILLED_EDID_PREFTIMING) continue;

            l = (long)pEDID[i + 4];
            l &= 0xf0;
            w = (long)pEDID[i + 2] + (l << 4);
            l = (long)pEDID[i + 7];
            l &= 0xf0;
            h           = (long)pEDID[i + 5] + (l << 4);
            b           = pEDID[i + 17];
            bInterlaced = (b & 0x80) ? TRUE : FALSE;

            _PrefTiming.width  = w;
            _PrefTiming.height = h;
            _PrefTiming.bpp    = 32;
            GetPreferedRefreshRate(_PrefTiming.freq, pxClk, w, h, bInterlaced);
            _PrefTiming.ratio       = (h > 0) ? (float)w / (float)h : 0.0f;
            _PrefTiming.bInterlaced = bInterlaced;
            _PrefTiming.tmPxClock   = pxClk;

            m_fFilled |= FLAG_FILLED_EDID_PREFTIMING;
            continue;
        }

        b = pEDID[i + 3];

        if (b == 0xff) {
            if (m_fFilled & FLAG_FILLED_EDID_STR_SERIAL) continue;
            if (!strncpy_s(_strSerial, c_DefNameBufferSz, (char*)(pEDID + i + 5), 13)) {
                for (pB = (BYTE*)_strSerial, l = 13; l > 0; l--, pB++) {
                    if (*pB == 0x0a) *pB = 0x0;
                    if (!*pB) break;
                }
                m_fFilled |= FLAG_FILLED_EDID_STR_SERIAL;
            }
        } else if (b == 0xfe) {
            if (m_fFilled & FLAG_FILLED_EDID_STR_MISC) continue;
            if (!strncpy_s(_strMisc, c_DefNameBufferSz, (char*)(pEDID + i + 5), 13)) {
                for (pB = (BYTE*)_strMisc, l = 13; l > 0; l--, pB++) {
                    if (*pB == 0x0a) *pB = 0x0;
                    if (!*pB) break;
                }
                m_fFilled |= FLAG_FILLED_EDID_STR_MISC;
            }
        } else if (b == 0xfd) {
            if (m_fFilled & FLAG_FILLED_EDID_RANGE_LIMIT) continue;
            l                            = i + 5;
            _FreqRangeLimits.freqVertMin = (long)pEDID[l++];
            _FreqRangeLimits.freqVertMax = (long)pEDID[l++];
            _FreqRangeLimits.freqHoriMin = (long)pEDID[l++];
            _FreqRangeLimits.freqHoriMax = (long)pEDID[l++];
            _FreqRangeLimits.pxClock     = 10 * (long)pEDID[l];

            if ((_FreqRangeLimits.freqVertMax >= _FreqRangeLimits.freqVertMin) &&
                (_FreqRangeLimits.freqHoriMax >= _FreqRangeLimits.freqHoriMin) && (_FreqRangeLimits.pxClock > 0))
                m_fFilled |= FLAG_FILLED_EDID_RANGE_LIMIT;
        } else if (b == 0xfc) {
            if (m_fFilled & FLAG_FILLED_EDID_STR_NAME) continue;
            if (!strncpy_s(_strFriendlyName, c_DefNameBufferSz, (char*)(pEDID + i + 5), 13)) {
                for (pB = (BYTE*)_strFriendlyName, l = 13; l > 0; l--, pB++) {
                    if (*pB == 0x0a) *pB = 0x0;
                    if (!*pB) break;
                }
                m_fFilled |= FLAG_FILLED_EDID_STR_NAME;
            }
        } else if (b == 0xfa) {
            pB = pEDID + i + 5;
            for (l = 0; l < 12;) {
                h = (long)pB[l++];
                if (h <= 0x1) break;
                b           = pB[l++];
                tmInfo.freq = 60 + (long)(b & 0x3f);
                b >>= 6;
                if (b & 0x1)
                    r = (b & 0x2) ? 16.0f / 9.0f : 4.0f / 3.0f;
                else
                    r = (b & 0x2) ? 5.0f / 4.0f : 16.0f / 10.0f;
                w                  = 248 + (h << 3);
                tmInfo.height      = (long)std::round((float)w / r);
                tmInfo.width       = w;
                tmInfo.ratio       = r;
                tmInfo.bpp         = 32;
                tmInfo.bInterlaced = FALSE;
                tmInfo.tmPxClock   = 0;

                _lStdTimings.push_back(tmInfo);
            }
        }
    }

    _bDigital  = bDigital;
    _szDeviceX = devWidth;
    _szDeviceY = devHeight;

    m_fFilled |= FLAG_FILLED_EDID_VERSION;

    if (m_fFilled & FLAG_FILLED_EDID_RANGE_LIMIT) {
        if (m_fFilled & FLAG_FILLED_EDID_PREFTIMING) {
            l = _PrefTiming.freq;
            _PrefTiming.freq =
                std::min<long>(std::max<long>(l, _FreqRangeLimits.freqVertMin), _FreqRangeLimits.freqVertMax);
        }
    }

    return true;
}

void DispRegistryInfo::GetPreferredDevMode(DEVMODE& prefMode) {
    if (!IsPreferredTimingInfoFilled()) return;

    ZeroMemory(&prefMode, sizeof(DEVMODE));
    prefMode.dmSize             = sizeof(DEVMODE);
    prefMode.dmFields           = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
    prefMode.dmPelsWidth        = _PrefTiming.width;
    prefMode.dmPelsHeight       = _PrefTiming.height;
    prefMode.dmBitsPerPel       = _PrefTiming.bpp;
    prefMode.dmDisplayFrequency = _PrefTiming.freq;

    return;
}

DispRegistryVolatileInfo* DispRegistryInfo::GetBy_PnPKey(wchar_t* pKeyStr, DWORD qKeyStr) {
    DispRegistryVolatileInfo* pVolI;
    size_t                    q = _lVolatileI.size();

    if (q && pKeyStr && qKeyStr) {
        for (pVolI = &_lVolatileI[0]; q; q--, pVolI++)
            if (!wcsncmp(pVolI->strKey, pKeyStr, qKeyStr)) return pVolI;
    }

    return NULL;
}

void DispRegistryInfo::GetPreferedRefreshRate(long& refreshRate, WORD pxClock, long width, long height,
                                              int bInterlaced) {
    long sz = width * height;

    if (pxClock && (sz > 0)) {
        float f = 10000.0f * (float)pxClock;
        f /= (float)sz;

        if (f >= 60.0f)
            refreshRate = 60;
        else if (f >= 50.0f)
            refreshRate = 50;
        else if (f >= 30.0f)
            refreshRate = 30;
        else if (f >= 25.0f)
            refreshRate = 25;
        else
            refreshRate = 0;

        if (refreshRate > 0) return;
    }

    refreshRate = (bInterlaced) ? 30 : 60;
}

bool DispRegistryInfo::GetCompatibleDisplayMode(DEVMODE& compMode, vector<DEVMODE>& lModes, DEVMODE* pDefDevMode) {
    int             bRegMI;
    long            l, diffV;
    DEVMODE         devM;
    DEVMODE *       pDevM, *pDevM0;
    DWORD           i, q, w, h, qCurr, bpp, iBest;
    vector<DEVMODE> lCurrM;
    const DWORD     compatibleFreq    = 60;
    const long      szCompatibleWidth = 800;
    const int       szDevM            = sizeof(DEVMODE);
    const int       fFull             = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

    ZeroMemory(&devM, szDevM);
    devM.dmSize = szDevM;

    if (pDefDevMode && ((pDefDevMode->dmFields & fFull) == fFull) && (pDefDevMode->dmPelsWidth > 0) &&
        (pDefDevMode->dmPelsHeight > 0) && (pDefDevMode->dmBitsPerPel > 0) && (pDefDevMode->dmDisplayFrequency > 0))
        bRegMI = TRUE;
    else
        bRegMI = FALSE;

    q = (DWORD)lModes.size();
    lCurrM.resize(q);
    if (!q) goto GET_FAIL;

    for (bpp = 32; bpp > 0; bpp -= 8) {
        lCurrM.clear();
        for (pDevM = &lModes[0], i = 0; i < q; i++, pDevM++)
            if ((pDevM->dmBitsPerPel == bpp) && ((pDevM->dmFields & fFull) == fFull)) lCurrM.push_back(*pDevM);
        qCurr = (DWORD)lCurrM.size();
        if (!qCurr) continue;

        for (pDevM = &lCurrM[0], i = 0; i < qCurr; i++, pDevM++)
            if (pDevM->dmPelsWidth >= szCompatibleWidth) break;
        if (i < qCurr) {
            iBest = i;
            diffV = (long)pDevM->dmPelsWidth - szCompatibleWidth;
            if (!diffV) break;
            for (pDevM++, i++; i < qCurr; i++, pDevM++) {
                l = pDevM->dmPelsWidth - szCompatibleWidth;
                if ((l >= 0) && (l < diffV)) {
                    iBest = i;
                    if (l)
                        diffV = l;
                    else
                        break;
                }
            }
            pDevM = &lCurrM[iBest];
            break;
        }

        for (pDevM = &lCurrM[0], i = 0; i < qCurr; i++, pDevM++)
            if (pDevM->dmPelsWidth < szCompatibleWidth) break;
        if (i < qCurr) {
            iBest = i;
            diffV = szCompatibleWidth - (long)pDevM->dmPelsWidth;
            for (pDevM++, i++; i < qCurr; i++, pDevM++) {
                l = szCompatibleWidth - (long)pDevM->dmPelsWidth;
                if ((l > 0) && (l < diffV)) {
                    iBest = i;
                    diffV = l;
                }
            }
            pDevM = &lCurrM[iBest];
            break;
        }
    }

    if (bpp) {
        if (pDevM->dmDisplayFrequency > compatibleFreq) {
            bpp   = 0;
            w     = pDevM->dmPelsWidth;
            h     = pDevM->dmPelsHeight;
            diffV = (long)pDevM->dmDisplayFrequency - compatibleFreq;
            for (pDevM0 = &lCurrM[0], i = 0; i < qCurr; i++, pDevM0++)
                if ((pDevM0->dmPelsWidth == w) && (pDevM0->dmPelsHeight == h) &&
                    (pDevM0->dmDisplayFrequency >= compatibleFreq)) {
                    l = (long)pDevM0->dmDisplayFrequency - compatibleFreq;
                    if (l < diffV) {
                        bpp++;
                        iBest = i;
                        if (l)
                            diffV = l;
                        else
                            break;
                    }
                }
            if (bpp) pDevM = &lCurrM[iBest];
        }

        memcpy(&compMode, pDevM, szDevM);
        if (bRegMI) {
            compMode.dmFields |= DM_POSITION;
            compMode.dmPosition.x = pDefDevMode->dmPosition.x;
            compMode.dmPosition.y = pDefDevMode->dmPosition.y;
        }

        return true;
    }

GET_FAIL:
    if (bRegMI) {
        memcpy(&compMode, pDefDevMode, szDevM);
        compMode.dmFields |= DM_POSITION;

        return true;
    }
    ZeroMemory(&compMode, szDevM);
    compMode.dmSize             = szDevM;
    compMode.dmFields           = fFull;
    compMode.dmPelsWidth        = 800;
    compMode.dmPelsHeight       = 600;
    compMode.dmBitsPerPel       = 32;
    compMode.dmDisplayFrequency = 60;

    return true;
}

bool DispRegistryInfo::GetCompatibleDisplayMode(DEVMODE& compMode, vector<DEVMODE>& lModes, DEVMODE* pDefDevMode,
                                                DWORD dispW, DWORD dispH, DWORD refreshFreq) {
    int             bRegMI;
    DEVMODE         devM;
    DEVMODE*        pDevM;
    long            l, diffV, wl, hl;
    vector<DEVMODE> lCurrM;
    DWORD           i, q, w, h, qCurr, bpp, iBest;
    const DWORD     maxV   = 1000000;
    const int       szDevM = sizeof(DEVMODE);
    const int       fFull  = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

    if (dispW >= 0x40000000) dispW >>= 2;
    if (dispH >= 0x40000000) dispH >>= 2;
    if (refreshFreq >= 0x40000000) refreshFreq >>= 2;

    if (pDefDevMode && ((pDefDevMode->dmFields & fFull) == fFull) && (pDefDevMode->dmPelsWidth > 0) &&
        (pDefDevMode->dmPelsHeight > 0) && (pDefDevMode->dmBitsPerPel > 0) && (pDefDevMode->dmDisplayFrequency > 0))
        bRegMI = TRUE;
    else
        bRegMI = FALSE;

    q = (DWORD)lModes.size();
    lCurrM.resize(q);
    if (!q) goto GET_FAIL;

    ZeroMemory(&devM, szDevM);
    devM.dmSize = szDevM;

    for (pDevM = &lModes[0], i = 0; i < q; i++, pDevM++)
        if (((pDevM->dmFields & fFull) == fFull) && (pDevM->dmPelsWidth > 0) && (pDevM->dmPelsHeight > 0) &&
            (pDevM->dmBitsPerPel > 0) && (pDevM->dmDisplayFrequency > 0))
            break;
    if (i >= q) goto GET_FAIL;

    iBest = i;
    wl    = (long)dispW;
    hl    = (long)dispH;
    w     = pDevM->dmPelsWidth;
    h     = pDevM->dmPelsHeight;
    diffV = abs(wl - (long)w) + abs(hl - (long)h);
    if (diffV > 0) {
        for (pDevM++, i++; i < q; i++, pDevM++)
            if (((pDevM->dmFields & fFull) == fFull) && (pDevM->dmPelsWidth > 0) && (pDevM->dmPelsHeight > 0) &&
                (pDevM->dmBitsPerPel > 0) && (pDevM->dmDisplayFrequency > 0)) {
                l = abs(wl - (long)pDevM->dmPelsWidth) + abs(hl - (long)pDevM->dmPelsHeight);
                if (l < diffV) {
                    iBest = i;
                    if (l > 0)
                        diffV = l;
                    else
                        break;
                }
            }
        pDevM = &lModes[iBest];
        w     = pDevM->dmPelsWidth;
        h     = pDevM->dmPelsHeight;
    }
    wl = (long)refreshFreq;
    for (bpp = 32; bpp > 0; bpp -= 8) {
        lCurrM.clear();
        for (pDevM = &lModes[0], i = 0; i < q; i++, pDevM++)
            if (((pDevM->dmFields & fFull) == fFull) && (pDevM->dmBitsPerPel == bpp) && (pDevM->dmPelsWidth == w) &&
                (pDevM->dmPelsHeight == h))
                lCurrM.push_back(*pDevM);
        qCurr = (DWORD)lCurrM.size();
        if (!qCurr) continue;

        iBest = 0;
        pDevM = &lCurrM[0];
        diffV = abs(wl - (long)pDevM->dmDisplayFrequency);
        if (diffV > 0) {
            for (pDevM++, i = 1; i < qCurr; i++, pDevM++) {
                l = abs(wl - (long)pDevM->dmDisplayFrequency);
                if (l < diffV) {
                    iBest = i;
                    if (l > 0)
                        diffV = l;
                    else
                        break;
                }
            }
            pDevM = &lCurrM[iBest];
        }
        break;
    }

    if (bpp) {
        memcpy(&compMode, pDevM, szDevM);

        if (bRegMI) {
            compMode.dmFields |= DM_POSITION;
            compMode.dmPosition.x = pDefDevMode->dmPosition.x;
            compMode.dmPosition.y = pDefDevMode->dmPosition.y;
        }

        return true;
    }

GET_FAIL:
    if (bRegMI) {
        memcpy(&compMode, pDefDevMode, szDevM);
        compMode.dmFields |= DM_POSITION;

        return true;
    }
    ZeroMemory(&compMode, szDevM);
    compMode.dmSize             = szDevM;
    compMode.dmFields           = fFull;
    compMode.dmPelsWidth        = dispW;
    compMode.dmPelsHeight       = dispH;
    compMode.dmBitsPerPel       = 32;
    compMode.dmDisplayFrequency = (refreshFreq && (refreshFreq != -1)) ? refreshFreq : 60;

    return true;
}

bool DispRegistryInfo::GetCompatibleDisplayMode(DEVMODE& compMode, wchar_t* pAdapterName) {
    DWORD           q;
    DEVMODE         defDevMode;
    vector<DEVMODE> lModes;
    int             bDefI  = FALSE;
    const int       szDevM = sizeof(DEVMODE);

    if (pAdapterName) {
        ZeroMemory(&defDevMode, szDevM);
        defDevMode.dmSize = szDevM;

        for (q = 0; EnumDisplaySettings(pAdapterName, q, &defDevMode); q++) lModes.push_back(defDevMode);

        if (EnumDisplaySettings(pAdapterName, ENUM_REGISTRY_SETTINGS, &defDevMode)) bDefI = TRUE;
    }

    return GetCompatibleDisplayMode(compMode, lModes, (bDefI) ? &defDevMode : NULL);
}

bool DispRegistryInfo::GetCompatibleDisplayMode(DEVMODE& compMode, wchar_t* pAdapterName, DWORD dispW, DWORD dispH,
                                                DWORD refreshFreq) {
    DWORD           q;
    DEVMODE         defDevMode;
    vector<DEVMODE> lModes;
    int             bDefI  = FALSE;
    const int       szDevM = sizeof(DEVMODE);

    if (pAdapterName) {
        ZeroMemory(&defDevMode, szDevM);
        defDevMode.dmSize = szDevM;

        for (q = 0; EnumDisplaySettings(pAdapterName, q, &defDevMode); q++) lModes.push_back(defDevMode);

        if (EnumDisplaySettings(pAdapterName, ENUM_REGISTRY_SETTINGS, &defDevMode)) bDefI = TRUE;
    }

    return GetCompatibleDisplayMode(compMode, lModes, (bDefI) ? &defDevMode : NULL, dispW, dispH, refreshFreq);
}

bool DispRegistryInfo::GetSimilarModes(vector<DEVMODE>& lSimilar, vector<DEVMODE>& lModes, DWORD dispW, DWORD dispH,
                                       DWORD refreshFreq) {
    DEVMODE*  pDevM;
    DWORD     q       = 0;
    DWORD     qModes  = (DWORD)lModes.size();
    const int fFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    lSimilar.clear();

    if (qModes) {
        lSimilar.resize(qModes);

        lSimilar.clear();
        for (pDevM = &lModes[0]; qModes; qModes--, pDevM++)
            if (((pDevM->dmFields & fFields) == fFields) && (pDevM->dmPelsWidth == dispW) &&
                (pDevM->dmPelsHeight == dispH) && (pDevM->dmDisplayFrequency == refreshFreq)) {
                lSimilar.push_back(*pDevM);
                q++;
            }
    }

    return (bool)q;
}

DWORD DispRegistryInfo::GetMaximumBpp(vector<DEVMODE>& lModes, DWORD dispW, DWORD dispH, DWORD refreshFreq,
                                      DWORD bppMax) {
    DWORD           v, i, q;
    DEVMODE*        pDevM;
    vector<DEVMODE> lSimilar;

    if (!bppMax) bppMax = -1;

    if (GetSimilarModes(lSimilar, lModes, dispW, dispH, refreshFreq)) {
        q = (DWORD)lSimilar.size();
        for (pDevM = &lSimilar[0]; q; q--, pDevM++)
            if ((pDevM->dmFields & DM_BITSPERPEL) && (pDevM->dmBitsPerPel <= bppMax)) break;
        if (!q) return 0;
        v = pDevM->dmBitsPerPel;
        if (v == bppMax) return v;
        for (pDevM++, q--; q; q--, pDevM++)
            if (pDevM->dmFields & DM_BITSPERPEL) {
                i = pDevM->dmBitsPerPel;
                if ((i <= bppMax) && (i > v)) {
                    v = i;
                    if (v == bppMax) break;
                }
            }

        return v;
    }

    return 0;
}

OSDisplayInfo::OSDisplayInfo() {
    int               i, j;
    WCHAR*            pStr;
    DEVMODE           devM;
    DISPLAY_DEVICE    dDev;
    DispAdapter       disp;
    DisplayDeviceCont ddc;
    DWORD             szDispDev = sizeof(DISPLAY_DEVICE);

    i = sizeof(DEVMODE);
    ZeroMemory(&devM, i);
    devM.dmSize = i;

    ZeroMemory(&dDev, szDispDev);
    dDev.cb = szDispDev;

    // iterate through all graphic cards (adapters) and query the device
    // connected to them
    for (i = 0; EnumDisplayDevices(NULL, i, &disp._AdapterInfo, 0); i++)  // name, info,
    {
        disp._iAdapter = i;
        pStr           = disp._AdapterInfo.DeviceName;

        disp._lDevModes.clear();
        for (j = 0; EnumDisplaySettings(pStr, j, &devM); j++) disp._lDevModes.push_back(devM);

        disp._lDispDevI.clear();
        for (j = 0; EnumDisplayDevices(pStr, j, &ddc.devCont, 0x0); j++) {
            if (EnumDisplayDevices(pStr, j, &dDev, EDD_GET_DEVICE_INTERFACE_NAME))
                ddc.interfaceName.Set((wchar_t*)dDev.DeviceID);
            disp._lDispDevI.push_back(ddc);
        }

        sprintf(disp._Name, "%ws", pStr);
        m_dispAdapters.push_back(disp);
    }

    // collect monitor info
    // only what the connected displays appear here
    if (EnumDisplayMonitors(NULL, NULL, MonitorEnumProcCb, reinterpret_cast<LPARAM>(&m_displays))) {
        // associate the found monitor with the found adapters
        for (auto& d : m_displays) SetDisplayInfo(m_dispAdapters, d);
    } else {
        cerr << "OSDisplayInfo Error: EnumDisplayMonitors failed" << endl;
        return;
    }

    // query all DISPLAY entries from the registry, from here we get the
    // friendly Display Names
    m_DispRegInfo = InitFromRegistry();

    // check which displays are active
    IdentifyActiveAdapterDisplay(m_dispAdapters);

    // build friendly names
    BuildFriendlyNames(m_dispAdapters, m_DispRegInfo);
}

OSDisplayInfo::~OSDisplayInfo() { m_dispAdapters.clear(); }

/** Add AdapterInfo to m_monitors, received from EnumDisplayMonitors */
bool OSDisplayInfo::SetDisplayInfo(list<DispAdapter>& dispAdapters, DisplayDevice& disp) {
    int i;

    char*    pChar  = disp._Name;
    wchar_t* pWChar = disp.displayInfo.szDevice;
    for (; *pWChar; pChar++, pWChar++) {
        i      = wctob(*pWChar);
        *pChar = (BYTE)i;
    }
    *pChar = 0x0;

    auto dispAdIt        = find_if(dispAdapters.begin(), dispAdapters.end(),
                                   [&disp](const DispAdapter& d) { return !strcmp(d._Name, disp._Name); });
    auto dispIt          = find_if(m_displays.begin(), m_displays.end(),
                                   [&disp](const DisplayDevice& d) { return !strcmp(d._Name, disp._Name); });
    disp.pAdapter        = &(*dispAdIt);
    disp.iDesktopDisplay = (int)(dispIt - m_displays.begin());
    dispAdIt->_pDisplay  = &disp;

    return true;
}

BOOL CALLBACK MonitorEnumProcCb(HMONITOR hMonitor, HDC hDC, LPRECT pRect, LPARAM pData) {
    auto displays = reinterpret_cast<vector<DisplayDevice>*>(pData);
    displays->push_back(DisplayDevice());
    displays->back().hMonitor           = hMonitor;
    displays->back().displayInfo.cbSize = sizeof(MONITORINFOEX);

    GetMonitorInfo(hMonitor, &displays->back().displayInfo);

    if (displays->back().displayInfo.dwFlags != DISPLAY_DEVICE_MIRRORING_DRIVER) {
        EnumDisplaySettings(displays->back().displayInfo.szDevice, ENUM_CURRENT_SETTINGS, &displays->back().mAct);
        return true;
    } else
        return false;
}

list<unique_ptr<DispRegistryInfo>> OSDisplayInfo::InitFromRegistry() {
    bool                     bSet;
    long                     iErr;
    size_t                   sz;
    TCHAR                    bufTmp[2048];
    TCHAR                    bufTmp2[2048];
    TCHAR                    bufTmp3[2048];
    int                      bOk, bActive, iFirstActive;
    DWORD                    i, j, szBuf, tKey, fCaps, qActive;
    DispRegistryVolatileInfo volI;
    TCHAR                    bufKeyName[c_MaxRegistryKeyNameSz];
    TCHAR                    strHWID[c_MaxRegistryValueSzWChar];
    TCHAR                    strDriver[c_MaxRegistryValueSzWChar];
    HKEY                     hAttrKey     = NULL;
    HKEY                     hDispKey     = NULL;
    HKEY                     hBaseKey     = NULL;
    const wchar_t            strBaseKey[] = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY";

    list<unique_ptr<DispRegistryInfo>> dispList;

    iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strBaseKey, 0, KEY_READ, &hBaseKey);
    if (iErr != ERROR_SUCCESS) return std::move(list<unique_ptr<DispRegistryInfo>>());

    for (i = 0; TRUE; i++) {
        szBuf = c_MaxRegistryKeyNameSz;
        iErr  = RegEnumKeyEx(hBaseKey, i, bufKeyName, &szBuf, NULL, NULL, NULL, NULL);
        if (iErr == ERROR_NO_MORE_ITEMS) break;
        if (iErr != ERROR_SUCCESS) goto INIT_FAIL;
        if (!wcscmp((wchar_t*)bufKeyName, L"Default_Monitor")) continue;

        wsprintf(bufTmp, L"%s\\%s", strBaseKey, bufKeyName);
        iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, bufTmp, 0, KEY_READ, &hDispKey);
        if (iErr != ERROR_SUCCESS) goto INIT_FAIL;

        unique_ptr<DispRegistryInfo> pDispRegI = make_unique<DispRegistryInfo>();

        szBuf++;
        if (szBuf > c_DefNameBufferSz) {
            szBuf = c_DefNameBufferSz - 1;
            wcstombs_s(&sz, pDispRegI->_Name, c_DefNameBufferSz, (wchar_t*)bufKeyName, szBuf);
            pDispRegI->_Name[szBuf] = 0x0;
        } else
            wcstombs_s(&sz, pDispRegI->_Name, c_DefNameBufferSz, (wchar_t*)bufKeyName, szBuf);

        bSet         = true;
        qActive      = 0;
        iFirstActive = -1;

        for (j = 0; TRUE; j++) {
            szBuf = c_MaxRegistryKeyNameSz;
            iErr  = RegEnumKeyEx(hDispKey, j, bufKeyName, &szBuf, NULL, NULL, NULL, NULL);
            if (iErr == ERROR_NO_MORE_ITEMS) break;
            if (iErr != ERROR_SUCCESS) goto INIT_FAIL;

            wsprintf(bufTmp2, L"%s\\%s", bufTmp, bufKeyName);
            iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, bufTmp2, 0, KEY_READ, &hAttrKey);
            if (iErr != ERROR_SUCCESS) continue;

            bOk = FALSE;
            do {
                tKey  = 0;
                szBuf = 4;
                iErr  = RegQueryValueEx(hAttrKey, L"Capabilities", NULL, &tKey, (BYTE*)&fCaps, &szBuf);
                if (!((iErr == ERROR_SUCCESS) && (tKey == REG_DWORD) && (szBuf == 4))) fCaps = 0x0;
                tKey  = 0;
                szBuf = c_MaxRegistryValueSz;
                iErr  = RegQueryValueEx(hAttrKey, L"Driver", NULL, &tKey, (BYTE*)strDriver, &szBuf);
                if (!((iErr == ERROR_SUCCESS) && (tKey == REG_SZ))) break;
                strDriver[(szBuf < c_MaxRegistryValueSz) ? szBuf >> 1 : c_MaxRegistryValueSzWChar - 1] = 0x0;
                tKey                                                                                   = 0;
                szBuf = c_MaxRegistryValueSz;
                iErr  = RegQueryValueEx(hAttrKey, L"HardwareID", NULL, &tKey, (BYTE*)strHWID, &szBuf);
                if (!((iErr == ERROR_SUCCESS) && (tKey == REG_MULTI_SZ))) break;
                strHWID[(szBuf < c_MaxRegistryValueSz) ? szBuf >> 1 : c_MaxRegistryValueSzWChar - 1] = 0x0;

                bOk = TRUE;
            } while (0);

            iErr     = RegCloseKey(hAttrKey);
            hAttrKey = NULL;

            wsprintf(bufTmp3, L"%s\\%s", bufTmp2, "Control");
            iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, bufTmp3, 0, KEY_READ, &hAttrKey);
            // why is this always false?????
            if (iErr == ERROR_SUCCESS) {
                iErr     = RegCloseKey(hAttrKey);
                hAttrKey = NULL;

                qActive++;
                bActive = TRUE;
                if (bOk && (iFirstActive < 0)) iFirstActive = (int)pDispRegI->_lVolatileI.size();
            } else
                bActive = FALSE;

            if (bOk) {
                volI.bActive = bActive;
                wsprintf(volI.strDispID, L"%s\\%s", strHWID, strDriver);
                wcscpy_s(volI.strKey, c_MaxRegistryKeyNameSz, (wchar_t*)bufKeyName);

                pDispRegI->_lVolatileI.push_back(volI);

                if (bSet) {
                    wcscat_s((wchar_t*)bufTmp2, 2048, L"\\");
                    wcscat_s((wchar_t*)bufTmp2, 2048, L"Device Parameters");
                    iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, bufTmp2, 0, KEY_READ, &hAttrKey);
                    if (iErr == ERROR_SUCCESS) {
                        szBuf = 0;
                        iErr  = RegQueryValueEx(hAttrKey, L"EDID", NULL, NULL, NULL, &szBuf);
                        pDispRegI->_lEDID.resize(szBuf);
                        if ((iErr == ERROR_SUCCESS) && szBuf) {
                            tKey = 0;
                            iErr = RegQueryValueEx(hAttrKey, L"EDID", NULL, &tKey, &pDispRegI->_lEDID[0], &szBuf);
                            if ((iErr == ERROR_SUCCESS) && (tKey == REG_BINARY)) {
                                pDispRegI->_iKeyEnum      = i;
                                pDispRegI->_fCapabilities = fCaps;
                                wcscpy_s((wchar_t*)pDispRegI->_strHWID, c_MaxRegistryValueSzWChar,
                                         (const wchar_t*)strHWID);
                                pDispRegI->m_filled = TRUE;
                                pDispRegI->ParseEDID();
                                bSet = false;
                            } else
                                pDispRegI->_lEDID.clear();
                        }

                        iErr     = RegCloseKey(hAttrKey);
                        hAttrKey = NULL;
                    }
                }
            }
        }

        iErr     = RegCloseKey(hDispKey);
        hDispKey = NULL;

        if (!bSet) {
            pDispRegI->_qActiveVolI      = qActive;
            pDispRegI->_iFirstActiveVolI = iFirstActive;
            dispList.push_back(std::move(pDispRegI));
        }
    }

    iErr     = RegCloseKey(hBaseKey);
    hBaseKey = NULL;

    return std::move(dispList);

INIT_FAIL:
    if (hAttrKey) RegCloseKey(hAttrKey);
    if (hDispKey) RegCloseKey(hDispKey);
    RegCloseKey(hBaseKey);

    // Destroy();
    return std::move(list<unique_ptr<DispRegistryInfo>>());
}

void OSDisplayInfo::IdentifyActiveAdapterDisplay(list<DispAdapter>& dispAr) {
    DWORD                       qDskMon = 0;
    wchar_t*                    pStrW   = (wchar_t*)L"";
    long                        i       = 0;
    DispRegistryInfo*           pDRI;
    vector<DispAdapter*>        lSrcPtr;
    list<ExternApiDisplayIdent> lExternDispIdent;

    // GetExternApiDisplayIdentList(lExternDispIdent, NULL, _StdTimeOut);
    // SetInitialDisplayNames(lExternDispIdent, NULL, _StdTimeOut);

    for (auto it = dispAr.begin(); it != dispAr.end(); ++it) {
        // _lDispDevI.size() == 0 means there is no device connected
        for (auto& pDDC : it->_lDispDevI) {
            pDDC.bActive = false;
            if (pDDC.devCont.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) qDskMon++;
        }

        it->_qActiveDispDev  = 0;
        it->_qDesktopDispDev = qDskMon;
        it->_tAdapter        = DispAdapter::TYP_SP_ADAPTER_UNSPECIFIC;
        if (!(it->_AdapterInfo.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) lSrcPtr.push_back(&(*it));
    }

    for (auto& pA : lSrcPtr) {
        if (!pA->_pDisplay) continue;

        qDskMon = pA->_qDesktopDispDev;
        if (qDskMon == 1) {
            for (auto& pDDC : pA->_lDispDevI)
                if (pDDC.devCont.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                    pDDC.bActive = true;
                    pA->_qActiveDispDev++;
                    pA->_tAdapter = DispAdapter::TYP_SP_ADAPTER_NORMAL;
                    break;
                }
        } else if (qDskMon == 0) {
            for (auto& pA1 : lSrcPtr) {
                if (!pA1 || pA1->_pDisplay || (pA1->_qDesktopDispDev != 1) || wcscmp(pA1->_AdapterInfo.DeviceID, pStrW))
                    continue;

                auto pDDC = find_if(pA1->_lDispDevI.begin(), pA1->_lDispDevI.end(), [](const DisplayDeviceCont& s) {
                    return s.devCont.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
                });

                auto pDDC1 = find_if(pA->_lDispDevI.begin(), pA->_lDispDevI.end(), [pDDC](const DisplayDeviceCont& s) {
                    return !wcscmp(s.devCont.DeviceID, pDDC->devCont.DeviceID);
                });

                if (pA->_lDispDevI.size() > 0)
                    pDDC1->devCont.StateFlags |= DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
                else {
                    pA->_lDispDevI.push_back((*pDDC));
                    pDDC1 = pDDC;
                }

                pDDC1->bActive = true;
                pA->_qActiveDispDev++;
                pA->_qDesktopDispDev++;
                pA->_tAdapter = DispAdapter::TYP_SP_ADAPTER_SPAN;

                pDDC->devCont.StateFlags &= ~DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

                // set this entry to null lSrcPtr vector
                auto lSrcPtrIt =
                    find_if(lSrcPtr.begin(), lSrcPtr.end(), [pA1](const DispAdapter* s) { return s == pA1; });
                if (lSrcPtrIt != lSrcPtr.end()) *lSrcPtrIt = nullptr;
                break;
            }
        } else {
            vector<DisplayDeviceCont*> lDDCPtr;

            auto pDDCIt = find_if(pA->_lDispDevI.begin(), pA->_lDispDevI.end(), [](const DisplayDeviceCont& s) {
                return s.devCont.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
            });

            if (pDDCIt != pA->_lDispDevI.end()) lDDCPtr.push_back(&(*pDDCIt));

            vector<DispAdapter*> lTmpSrcPtr;

            /*
            if (IsATIMultiView(lTmpSrcPtr, lDDCPtr, lSrcPtr, i))
            {
                for (m = 0, j = (long)lTmpSrcPtr.size() - 1; j >= 0; j--, m++)
            // !! revert assign order !!
                {
                    pA = lTmpSrcPtr[j];
                    auto pDDC = find_if(pA->_lDispDevI.begin(),
            pA->_lDispDevI.end(), [lDDCPtr, m](const DisplayDeviceCont& s) {
            return !wcscmp(s.devCont.DeviceID, lDDCPtr[m]->devCont.DeviceID);
            });

                    if (pDDC != pA->_lDispDevI.end())
                    {
                        pDDC->bActive = TRUE;
                        pA->_qActiveDispDev++;
                        pA->_tAdapter = DispAdapter::TYP_SP_ADAPTER_NORMAL;

                        // set this entry to null lSrcPtr vector
                        auto lSrcPtrIt = find_if(lSrcPtr.begin(), lSrcPtr.end(),
            [pA](const DispAdapter* s) { return s == pA; }); if (lSrcPtrIt !=
            lSrcPtr.end()) *lSrcPtrIt = nullptr;
                    }
                }
            }
            else
            {
            */
            lDDCPtr[0]->bActive = TRUE;
            pA->_qActiveDispDev++;
            //}
        }

        i++;
    }

    if (lExternDispIdent.size()) {
        /*
        for (pA = pSrcL->GetFirst(); pA; pA = pA->_pNext)
            if (pA->_pDisplay)
            {
                if (ExternApiDisplayIdent::IsDisplayIdentInList(pEDI,
        pA->_pDisplay->_Name, lExternDispIdent) && pEDI->strDisplayName[0])
                {
                    pA->_pDisplay->bUserDefinedDesc = TRUE;
                    strcpy_s(pA->_pDisplay->strDesc, c_DefNameBufferSz,
        pEDI->strDisplayName);
                }
                else
                    pA->_pDisplay->bUserDefinedDesc = FALSE;
            }
            */
    }

    for (auto& pA : lSrcPtr)
        if (pA && pA->_pDisplay) {
            auto pDDC = pA->GetFirstActiveDispDevI();
            if (!pDDC) continue;

            pDRI = FindDisplayRegInfo(pDDC->devCont.DeviceID);

            if (!pDRI) continue;

            pA->_pDisplay->tDisplay =
                (pDRI->MaybeAProjector()) ? TYP_SP_DISPLAY_PROJECTOR : TYP_SP_DISPLAY_MONITOR_PANEL;

            if (pDRI->IsPreferredTimingInfoFilled()) {
                pA->_pDisplay->szOptimalResolution.cx = pDRI->_PrefTiming.width;
                pA->_pDisplay->szOptimalResolution.cy = pDRI->_PrefTiming.height;
            }

            if (!pA->_pDisplay->bUserDefinedDesc && pDRI->IsDisplayNameStrFilled())
                strcpy_s(pA->_pDisplay->strDesc, c_DefNameBufferSz, pDRI->_strFriendlyName);
            // else bleibt nun leer...
        }
}

/** also build a list with active DisplayAdaptes only */
void OSDisplayInfo::BuildFriendlyNames(list<DispAdapter>& dispAdapters, list<unique_ptr<DispRegistryInfo>>& regInfo) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    m_activeAdapters.clear();
    for (auto& disp : dispAdapters) {
        // compare DeviceID of DispAdapter with HWID of regInfo
        if (disp._lDispDevI.size() > 0) {
            // this is ok as the system string contains only ASCII-translatable
            // chars, so it will not grow in length when converting double check
            // anyway, if there is a "\" after
            string devIdStr = converter.to_bytes(wstring(disp._lDispDevI[0].devCont.DeviceID).substr(0, 15));
            bool   found    = false;
            for (auto& reg : regInfo) {
                if (reg->m_filled) {
                    string hwidStr = converter.to_bytes(wstring(reg->_strHWID));
                    if (!strcmp(hwidStr.c_str(), devIdStr.c_str())) {
                        // build friendlyname
                        string adName     = string(disp._Name);
                        disp.friendlyName = "D" + string(adName.begin() + 11, adName.end()) + " " +
                                            string(reg->_strFriendlyName) + " (" + string(reg->_Name) + ")";

                        // save to m_activeAdapters
                        m_activeAdapters.push_back(&disp);
                        found = true;
                    }
                }
            }

            if (!found) {
                string adName = string(disp._Name);
                disp.friendlyName =
                    "D" + string(adName.begin() + 11, adName.end()) + "  (Default_Monitor)";  // two space needed before
                                                                                              // (Default_Monitor)

                // save to m_activeAdapters
                m_activeAdapters.push_back(&disp);
            }
        }
    }
}

/*
bool OSDisplayInfo::SetInitialDisplayNames(list<ExternApiDisplayIdent>& lDst,
int* pErr, DWORD timeOut)
{
    size_t i;
    char* pStr;
    GPUSocketLayoutI* pSLI;
    GPUProductLayoutAssignI* pLAI;
    ExternApiDisplayIdent* pIdent;
    GPUOutDeviceEnumSocketAssignI* pDESA;
    size_t qIdent = lDst.GetCount();

    if (!qIdent)
        return false;

    for (pIdent = lDst.GetListPtr(), i = 0; i < qIdent; i++, pIdent++)
        if (_GPUSocketLayoutDB.IsProductLayoutAssignInfoInList(pLAI,
pIdent->strAPIGPU, pIdent->tGPUVendor) &&
            _GPUSocketLayoutDB.IsLayoutInfoInList(pSLI, pLAI->strLayout) &&
            pSLI->IsDeviceEnumSocketAssignInfoInList(pDESA,
pIdent->iAPIDisplayEnum) &&
            SPHWS_InitialDisplayNameDef::IsDisplayNameInList(pStr,
pIdent->iApiGPU, pDESA->strSocketMarker, _lInitialDisplayNameDef)
            )
        {
            strcpy_s(pIdent->strDisplayName, c_DefNameBufferSz, pStr);
        }
        else
            pIdent->strDisplayName[0] = 0x0;
    return true;
}
*/

DispRegistryInfo* OSDisplayInfo::FindDisplayRegInfo(wchar_t* strDispID) {
    if (strDispID)
        for (auto& pDRI : m_DispRegInfo) {
            auto it =
                find_if(pDRI->_lVolatileI.begin(), pDRI->_lVolatileI.end(),
                        [strDispID](const DispRegistryVolatileInfo& s) { return !wcscmp(strDispID, s.strDispID); });
            if (it != pDRI->_lVolatileI.end()) return pDRI.get();
        }

    return nullptr;
}

bool OSDisplayInfo::IsATIMultiView(vector<DispAdapter*>& lAdapter, vector<DisplayDeviceCont*>& lDDCPtr,
                                   vector<DispAdapter*>& lSrc, long iStart) {
    size_t             i, j, k, q1;
    wchar_t *          pAId, *pDId;
    DispAdapter*       pR;
    DisplayDeviceCont* pDDC;
    size_t             q    = lDDCPtr.size();
    size_t             qSrc = lSrc.size();

    if (!(lDDCPtr.size() && lSrc.size() && ((size_t)iStart < qSrc))) return false;

    pR = lSrc[iStart];
    if (!pR) return false;

    lAdapter.push_back(pR);

    pAId = pR->_AdapterInfo.DeviceID;
    for (i = (size_t)iStart + 1; i < qSrc; i++) {
        pR = lSrc[i];
        if (!(pR && pR->_pDisplay && (pR->_qDesktopDispDev == (DWORD)q) && !wcscmp(pR->_AdapterInfo.DeviceID, pAId)))
            continue;

        q1 = pR->_lDispDevI.size();
        for (j = 0; j < q; j++) {
            pDId = lDDCPtr[j]->devCont.DeviceID;
            for (pDDC = &pR->_lDispDevI[0], k = q1; k; k--, pDDC++)
                if ((pDDC->devCont.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) &&
                    !wcscmp(pDDC->devCont.DeviceID, pDId))
                    break;

            if (!k) break;
        }
        if (j == q) {
            lAdapter.push_back(pR);
        }
    }

    return (q == lAdapter.size());
}

bool OSDisplayInfo::GetExternApiDisplayIdentList(list<ExternApiDisplayIdent>& lDst, int* pErr, DWORD tmOut) {
#ifdef SPSUPPORT_ENABLE_NVAPI
    /*if (_fInitialized & FLAG_SP_EXTERN_API_NV)
    {
        list<NVApiGPUInfo> lGPUI;
        NVApiDisplayIdentParser parser;

        return (parser.FillGPUInfoList(lGPUI) &&
    parser.FillExternApiDisplayIdentList(lDst, lGPUI));
    }*/
#endif  // SPSUPPORT_ENABLE_NVAPI
    return true;
}

std::list<std::unique_ptr<DispRegistryInfo>>& OSDisplayInfo::registryInfo() { return m_DispRegInfo; }

std::vector<DisplayDevice>& OSDisplayInfo::displayInfo() { return m_displays; }

std::list<DispAdapter*>& OSDisplayInfo::displayAdapters() { return m_activeAdapters; }

string OSDisplayInfo::getFriendlyNameForDevName(string& devName) {
    for (auto& disp : m_dispAdapters)
        if (disp._pDisplay && !strcmp(devName.c_str(), disp._Name)) return disp.friendlyName;
    return "";
}

}  // namespace ara
#endif