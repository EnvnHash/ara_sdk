#ifdef _WIN32

#include "Registry.h"

#include <tchar.h>
#include <windows.h>

namespace ara {
BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, LPWSTR szString, const size_t nMaxString) {
    BOOL   res = FALSE;
    HKEY   skey;
    size_t nMax = nMaxString;

    if (ERROR_SUCCESS == ::RegOpenKeyExW(root, szPath, 0, KEY_READ, &skey)) {
        DWORD s = 0;
        if (ERROR_SUCCESS == ::RegQueryValueExW(skey, szKey, 0, 0, 0, &s)) {
            if (s / sizeof(wchar_t) <= nMax) {
                if (ERROR_SUCCESS == ::RegQueryValueExW(skey, szKey, 0, 0, reinterpret_cast<LPBYTE>(szString), &s)) {
                    // szString[s] = L'\0';
                    res = TRUE;
                }
            }
        }
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, LPSTR szString, const size_t nMaxString) {
    BOOL   res = FALSE;
    HKEY   skey;
    size_t nMax = nMaxString;
    if (ERROR_SUCCESS == ::RegOpenKeyExA(root, szPath, 0, KEY_READ, &skey)) {
        DWORD s = 0;
        if (ERROR_SUCCESS == ::RegQueryValueExA(skey, szKey, 0, 0, 0, &s)) {
            if (s <= nMax) {
                if (ERROR_SUCCESS == ::RegQueryValueExA(skey, szKey, 0, 0, reinterpret_cast<LPBYTE>(szString), &s)) {
                    // szString[s] = '\0';
                    res = TRUE;
                }
            }
        }
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD &dw) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegOpenKeyExW(root, szPath, 0, KEY_READ, &skey)) {
        DWORD s = sizeof(DWORD);
        if (ERROR_SUCCESS == ::RegQueryValueExW(skey, szKey, 0, 0, reinterpret_cast<LPBYTE>(&dw), &s)) res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD &dw) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegOpenKeyExA(root, szPath, 0, KEY_READ, &skey)) {
        DWORD s = sizeof(DWORD);
        if (ERROR_SUCCESS == ::RegQueryValueExA(skey, szKey, 0, 0, reinterpret_cast<LPBYTE>(&dw), &s)) res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD const &n, void *p) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegOpenKeyExW(root, szPath, 0, KEY_READ, &skey)) {
        DWORD s = n;
        if (ERROR_SUCCESS == ::RegQueryValueExW(skey, szKey, 0, 0, reinterpret_cast<LPBYTE>(p), &s)) res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD const &n, void *p) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegOpenKeyExA(root, szPath, 0, KEY_READ, &skey)) {
        DWORD s = n;
        if (ERROR_SUCCESS == ::RegQueryValueExA(skey, szKey, 0, 0, reinterpret_cast<LPBYTE>(p), &s)) res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegSaveW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, LPCWSTR szString) {
    HKEY skey;
    BOOL res = FALSE;
    long l   = ::RegCreateKeyExW(root, szPath, 0, 0, 0, KEY_ALL_ACCESS, 0, &skey, 0);

    if (l == ERROR_SUCCESS) {
        if (ERROR_SUCCESS == ::RegSetValueExW(skey, szKey, 0, REG_SZ,
                                              reinterpret_cast<LPBYTE>(const_cast<LPWSTR>(szString)),
                                              static_cast<DWORD>((wcslen(szString) + 1) * sizeof(TCHAR))))
            res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegSaveA(HKEY root, LPCSTR szPath, LPCSTR szKey, LPCSTR szString) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegCreateKeyExA(root, szPath, 0, 0, 0, KEY_ALL_ACCESS, 0, &skey, 0)) {
        if (ERROR_SUCCESS == ::RegSetValueExA(skey, szKey, 0, REG_SZ,
                                              reinterpret_cast<LPBYTE>(const_cast<LPSTR>(szString)),
                                              static_cast<DWORD>((strlen(szString) + 1) * sizeof(TCHAR))))
            res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegSaveW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD const &dw) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegCreateKeyExW(root, szPath, 0, 0, 0, KEY_ALL_ACCESS, 0, &skey, 0)) {
        if (ERROR_SUCCESS == ::RegSetValueExW(skey, szKey, 0, REG_DWORD,
                                              reinterpret_cast<LPBYTE>(const_cast<DWORD *>(&dw)), sizeof(DWORD)))
            res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegSaveA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD const &dw) {
    HKEY skey;
    BOOL res = FALSE;

    if (ERROR_SUCCESS == ::RegCreateKeyExA(root, szPath, 0, 0, 0, KEY_ALL_ACCESS, 0, &skey, 0)) {
        if (ERROR_SUCCESS == ::RegSetValueExA(skey, szKey, 0, REG_DWORD,
                                              reinterpret_cast<LPBYTE>(const_cast<DWORD *>(&dw)), sizeof(DWORD)))
            res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegSaveW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD const &n, void const *p) {
    BOOL res = FALSE;
    HKEY skey;
    if (ERROR_SUCCESS == ::RegCreateKeyExW(root, szPath, 0, 0, 0, KEY_ALL_ACCESS, 0, &skey, 0)) {
        if (ERROR_SUCCESS == ::RegSetValueExW(skey, szKey, 0, REG_BINARY, reinterpret_cast<CONST BYTE *>(p), n))
            res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegSaveA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD const &n, void const *p) {
    HKEY skey;
    BOOL res = FALSE;

    if (ERROR_SUCCESS == ::RegCreateKeyExA(root, szPath, 0, 0, 0, KEY_ALL_ACCESS, 0, &skey, 0)) {
        if (ERROR_SUCCESS == ::RegSetValueExA(skey, szKey, 0, REG_BINARY, reinterpret_cast<CONST BYTE *>(p), n))
            res = TRUE;
        ::RegCloseKey(skey);
    }
    return res;
}

BOOL RegDeleteW(HKEY root, LPCWSTR szPath, LPCWSTR szKey) {
    HKEY skey;
    BOOL res = FALSE;

    if (ERROR_SUCCESS == ::RegOpenKeyExW(root, szPath, 0, KEY_ALL_ACCESS, &skey)) {
        if (ERROR_SUCCESS == ::RegDeleteValueW(skey, szKey)) res = TRUE;
        ::RegCloseKey(skey);
    }

    return res;
}

BOOL RegDeleteA(HKEY root, LPCSTR szPath, LPCSTR szKey) {
    HKEY skey;
    BOOL res = FALSE;

    if (ERROR_SUCCESS == ::RegOpenKeyExA(root, szPath, 0, KEY_ALL_ACCESS, &skey)) {
        if (ERROR_SUCCESS == ::RegDeleteValueA(skey, szKey)) res = TRUE;
        ::RegCloseKey(skey);
    }

    return res;
}

}  // namespace ara

#endif