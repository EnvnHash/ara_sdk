#pragma once

#ifdef _WIN32
#if !defined(_TOOLS_REGISTRY_HPP_)
#define _TOOLS_REGISTRY_HPP_

#include <windows.h>

namespace ara {
/** @brief RegLoad Loads a string (REG_SZ)from the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @param [OUT] szString The string to store the string value
 * @param [IN] nMaxString The capacity of the string in characters. If szString
 * is NULL this value is ignored.
 * @return nonzero on success, 0 otherwise
 * @remark Double check for terminating '\0' as it is not guaranteed a string
 * has stored it!*/
BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, LPWSTR szString, const size_t nMaxString);
template <size_t s>
BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, wchar_t (&szString)[s]) {
    return RegLoadW(root, szPath, szKey, szString, s);
}
BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, LPSTR szString, const size_t nMaxString);
template <size_t s>
BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, char (&szString)[s]) {
    return RegLoadA(root, szPath, szKey, szString, s);
}

/** @brief RegLoad Loads a DWORD (REG_DWORD) from the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @param [OUT] dw The reference to store the value
 * @return nonzero on success, 0 otherwise*/
BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD& dw);
BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD& dw);

/** @brief RegLoad Loads a BINARY (REG_BINARY) from the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @param [IN] n The size of the blob
 * @param [IN] p Pointer to data
 * @return nonzero on success, 0 otherwise*/
BOOL RegLoadW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD const& n, void* p);
BOOL RegLoadA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD const& n, void* p);

#ifdef UNICODE
#define RegLoad RegLoadW
#else
#define RegLoad RegLoadA
#endif  // !UNICODE

/** @brief RegLoad Saves a string (REG_SZ) to the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @param [IN] szString The string to store
 * @return nonzero on success, 0 otherwise*/
BOOL RegSaveW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, LPCWSTR szString);
BOOL RegSaveA(HKEY root, LPCSTR szPath, LPCSTR szKey, LPCSTR szString);

/** @brief RegLoad Saves a DWORD (REG_DWORD) to the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @param [IN] dw The value
 * @return nonzero on success, 0 otherwise*/
BOOL RegSaveW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD const& dw);
BOOL RegSaveA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD const& dw);

/** @brief RegLoad Saves a BINARY (REG_BINARY) to the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @param [IN] n The size of the blob
 * @param [IN] p Pointer to data
 * @return nonzero on success, 0 otherwise*/
BOOL RegSaveW(HKEY root, LPCWSTR szPath, LPCWSTR szKey, DWORD const& n, void const* p);
BOOL RegSaveA(HKEY root, LPCSTR szPath, LPCSTR szKey, DWORD const& n, void const* p);

#ifdef UNICODE
#define RegSave RegSaveW
#else
#define RegSave RegSaveA
#endif  // !UNICODE

/** @brief RegDelete Deletes a key from the registry
 * @param [IN] root SDK HKEY definition like HKEY_LOCAL_MACHINE
 * @param [IN] szPath The path to the key
 * @param [IN] szKey The name of the key
 * @return nonzero on success, 0 otherwise*/
BOOL RegDeleteW(HKEY root, LPCWSTR szPath, LPCWSTR szKey);
BOOL RegDeleteA(HKEY root, LPCSTR szPath, LPCSTR szKey);

#ifdef UNICODE
#define RegDelete RegDeleteW
#else
#define RegDelete RegDeleteA
#endif  // !UNICODE

}  // namespace ara

#endif  //! defined( _TOOLS_REGISTRY_HPP_ )
#endif