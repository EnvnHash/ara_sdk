
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <util_common.h>

#ifdef _WIN32

namespace ara {
typedef HRESULT(WINAPI* LPFUNC_GETDPIFORMONITOR)(HMONITOR, int, UINT*, UINT*);
static HMODULE                 _hShcoreDLL = nullptr;
static LPFUNC_GETDPIFORMONITOR _pFuncGetDpiForMonitor;

static int loadShCore() {
    if (_hShcoreDLL) {
        return FALSE;
    }

    _hShcoreDLL = ::LoadLibraryA("shcore.dll");
    if (!_hShcoreDLL) {
        return FALSE;
    }

    _pFuncGetDpiForMonitor = reinterpret_cast<LPFUNC_GETDPIFORMONITOR>(::GetProcAddress(_hShcoreDLL, "GetDpiForMonitor"));

    return TRUE;
}

static int unloadShCore() {
    int bRes = TRUE;

    if (_hShcoreDLL) {
        bRes = ::FreeLibrary(_hShcoreDLL);
    }

    return bRes;
}

static HRESULT GetDpiForMonitor(HMONITOR hMon, int dpiType, UINT* dpiX, UINT* dpiY) {
    if (!_pFuncGetDpiForMonitor) {
        loadShCore();
    }

    if (_pFuncGetDpiForMonitor) {
        return (_pFuncGetDpiForMonitor)(hMon, dpiType, dpiX, dpiY);
    }

    if (dpiX) {
        *dpiX = 96;
    }

    if (dpiY) {
        *dpiY = 96;
    }
    return S_OK;
}

static int getDisplayScale(HWND hwnd, float* xscale, float* yscale) {
#ifdef _WIN32

    auto handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    UINT xdpi, ydpi;

    GetDpiForMonitor(handle, 0, &xdpi, &ydpi);

    if (xscale) {
        *xscale = static_cast<float>(xdpi) / 96.f;
    }
    if (yscale) {
        *yscale = static_cast<float>(ydpi) / 96.f;
    }
#endif
    return 1;
}

static int getDisplayScale(HMONITOR mon, float* xscale, float* yscale) {
#ifdef _WIN32

    UINT xdpi, ydpi;
    GetDpiForMonitor(mon, 0, &xdpi, &ydpi);

    if (xscale) *xscale = static_cast<float>(xdpi) / 96.f;
    if (yscale) *yscale = static_cast<float>(ydpi) / 96.f;
#endif
    return 1;
}
}  // namespace ara
#endif