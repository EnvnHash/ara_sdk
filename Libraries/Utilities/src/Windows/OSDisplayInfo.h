#ifdef _WIN32

#pragma once

#include <util_common.h>
#include <windows.h>

namespace ara {
static const int   c_DefNameBufferSz         = 72;    ///<   should be greater or equal 64
static const int   c_DefDeviceDescSz         = 260;   ///<   should be greater or equal 128
static const DWORD c_MaxRegistryKeyNameSz    = 256;   ///<   defines the maximum size of the name of a registry key
static const DWORD c_MaxRegistryValueSz      = 1024;  ///<   defines the maximum size of the value of a registry key
static const DWORD c_MaxRegistryValueSzWChar = c_MaxRegistryValueSz >> 1;  ///<   defines the maximum size of the value
                                                                           ///<   of a registry key in wchar characters
static const int c_DefResStrIDSz = 128;                                    ///<   should be greater or equal 128

typedef enum {
    TYP_SP_GPU_VENDOR_UNKNOWN = 0,  ///<   unknown GPU vendor typ
    TYP_SP_GPU_VENDOR_NVIDIA,       ///<   NVIDIA
    TYP_SP_GPU_VENDOR_AMD,          ///<   AMD
    TYP_SP_GPU_VENDOR_INTEL,        ///<   Intel
    TYP_SP_GPU_VENDOR_UNDEF         ///<   undefined
} ESPGPUVendorTyp;

typedef enum {
    TYP_SP_DISPLAY_MONITOR_PANEL = 0,  ///<   panel based display
    TYP_SP_DISPLAY_PROJECTOR,          ///<   projector or display with variable output
                                       ///<   size
    TYP_SP_DISPLAY_ABSTRACT,           ///<   identifies an abstract display
    TYP_SP_DISPLAY_UNDEF
} ESPUMA_DisplayTyp;

typedef enum {
    TYP_SP_DISPLAYTECHNOLOGY_UNKNOWN  = 0x00,
    TYP_SP_DISPLAYTECHNOLOGY_MASKCRT  = 0x01,
    TYP_SP_DISPLAYTECHNOLOGY_GRILLCRT = 0x02,
    TYP_SP_DISPLAYTECHNOLOGY_TFT      = 0x03,
    TYP_SP_DISPLAYTECHNOLOGY_LCOS     = 0x04,
    TYP_SP_DISPLAYTECHNOLOGY_PLASMA   = 0x05,
    TYP_SP_DISPLAYTECHNOLOGY_OLED     = 0x06,
    TYP_SP_DISPLAYTECHNOLOGY_EL       = 0x07,
    TYP_SP_DISPLAYTECHNOLOGY_MEM      = 0x08,
    TYP_SP_DISPLAYTECHNOLOGY_FED      = 0x09,
    TYP_SP_DISPLAYTECHNOLOGY_UNDEF,
} ESPUMA_DisplayTechnology;

/** SSPHWS_DispStdTimingInfo\n
 *  ( Struct SmartProjector HardWare Support Display Standard Timing Information
 * )\n Container describes a standard timing mode for a display.\n
 * @author Johannes Mueller
 * @date Nov. 09
 * @version 1.0
 * @bug No.
 * @todo Nothing. */
typedef struct DispStdTimingInfo {
public:
    long  width;        ///<   size of pixel in x direction
    long  height;       ///<   size of pixel in y direction
    DWORD bpp;          ///<   bit per pixel
    long  freq;         ///<   prefered frequenz for this mode
    float ratio;        ///<   aspect ratio
    int   bInterlaced;  ///<   optional, indicates an interlacing timing mode
    WORD  tmPxClock;    ///<   optional, pixel clock in 10kHz
    DispStdTimingInfo() { ::ZeroMemory(this, sizeof(DispStdTimingInfo)); }
} DispStdTimingInfo;

/** SSPHWS_DispFreqRangeLimitsInfo\n
 *  ( Struct SmartProjector HardWare Support Display Frequenzy Range Limits
 * Information )\n Container describes the frequenzy range limits for a
 * display.\n
 * @author Johannes Mueller
 * @date Nov. 09
 * @version 1.0
 * @bug No.
 * @todo Nothing. */
typedef struct DispFreqRangeLimitsInfo {
public:
    long freqVertMin = 0;  ///<   minimal vertical frequenz (in Hz)
    long freqVertMax = 0;  ///<   maximal vertical frequenz (in Hz)
    long freqHoriMin = 0;  ///<   minimal horicontal frequenz (in kHz)
    long freqHoriMax = 0;  ///<   maximal horicontal frequenz (in kHz)
    long pxClock     = 0;  ///<   pixel clock (in MHz)
    DispFreqRangeLimitsInfo() {}
} DispFreqRangeLimitsInfo;

/** DispRegistryVolatileInfo\n
 *  ( Struct SmartProjector HardWare Support Display Registry Volatile
 * Information )\n Container to collect the volatile informations about a
 * display.\n
 * @author Johannes Mueller
 * @author Sven Hahne
 * @date Jan. 20
 * @version 1.0
 * @bug No.
 * @todo Nothing. */
typedef struct DispRegistryVolatileInfo {
public:
    int     bActive = 0;                     ///<   indicates an as active marked registry information
    wchar_t strDispID[c_DefDeviceDescSz];    ///<   volatile device id string of
                                             ///<   the used display
    wchar_t strKey[c_MaxRegistryKeyNameSz];  ///<   name of the registry key with
                                             ///<   the volatile informations (pnp
                                             ///<   identifier)
    DispRegistryVolatileInfo() {}
} DispRegistryVolatileInfo;

typedef struct DeviceInterfaceName {
public:
    int     bParsed;            ///<   indicates if strInterface was parsed successfully
    wchar_t strInterface[128];  ///<   holds the complete device interface name
    DWORD   qStr;               ///<   size of string inside strInterface
    DWORD   vName[2];           ///<   holds the index and length of the device name in
                                ///<   strInterface
    DWORD vKey[2];              ///<   holds the index and length of the device key in
                                ///<   strInterface
    DWORD
    vGUID[2];  ///<   holds the index and length of the GUID in strInterface

    DeviceInterfaceName() {}

    /** Sets the container to given string
     * @param  pSrc                pointer to source string  */
    bool Set(wchar_t* pSrc) {
        wchar_t *pS, *pE;
        DWORD    q = (pSrc) ? (DWORD)wcslen(pSrc) : 0;

        if (!q) return false;

        if (wcscpy_s(strInterface, c_DefResStrIDSz, pSrc)) return false;

        qStr = q;
        pS   = wcschr(strInterface, L'#');
        if (!pS) return false;
        pE = wcschr(++pS, L'#');
        if (!pE) return false;
        vName[0] = (DWORD)(pS - strInterface);
        vName[1] = (DWORD)(pE - pS);

        pS = ++pE;
        pE = wcschr(pS, L'#');
        if (!pE) return false;
        vKey[0] = (DWORD)(pS - strInterface);
        vKey[1] = (DWORD)(pE - pS);

        pS       = ++pE;
        vGUID[0] = (DWORD)(pS - strInterface);
        vGUID[1] = qStr - vGUID[0];

        bParsed = TRUE;

        return true;
    }

} DeviceInterfaceName;

typedef struct DisplayDeviceCont {
public:
    int                 bActive;        ///<   indicates an active/used display device
    DISPLAY_DEVICE      devCont;        ///<   holds the wrapped DISPLAY_DEVICE container
    DeviceInterfaceName interfaceName;  ///<   used to store the device interface name for
                                        ///<   (GUID_DEVINTERFACE_MONITOR)

    DisplayDeviceCont() {
        ZeroMemory(&devCont, sizeof(DISPLAY_DEVICE));
        devCont.cb = sizeof(DISPLAY_DEVICE);
    }
} DisplayDeviceCont;

class DispAdapter;

typedef struct DisplayDevice {
public:
    HMONITOR hMonitor;                             ///<	handle to the monitor, unique if the hardware
                                                   ///< configuration don't change
    MONITORINFOEX displayInfo;                     ///<	info about the monitor
    DEVMODE       mAct;                            ///<	informations about the actual mode, the monitor is
                                                   ///< running
    SIZE                     szOptimalResolution;  ///<	optimal resolution of that display
    ESPUMA_DisplayTyp        tDisplay;             ///<	display type @see ESPUMA_DisplayTyp
    ESPUMA_DisplayTechnology tTechnology;          ///<	display technology used
    DispAdapter*             pAdapter;             ///<	pointer to the adapter device
                                                   ///< informations for this display
    long iDesktopDisplay;                          ///<	index of the assigned display beyond the desktop
                                                   ///< order

    int bUserDefinedDesc;             ///<	if TRUE, the friendly name was set explicit by
                                      ///< the user
    char strDesc[c_DefNameBufferSz];  ///<	friendly name of the display

    int  qTmp;                      ///<	temporary attribute, !! do not use !!
    char _Name[c_DefNameBufferSz];  ///<	name of the display

    DisplayDevice() { ::ZeroMemory(this, sizeof(DisplayDevice)); }
} DisplayDevice;

//-------------------------------------------------------------------------------------------------------------------------------------------------------

/** graphics card */
class DispAdapter {
public:
    typedef enum {
        TYP_SP_ADAPTER_UNSPECIFIC = 0,  ///<   unspecific adapter
        TYP_SP_ADAPTER_NORMAL,          ///<   normal adapter
        TYP_SP_ADAPTER_SPAN,            ///<   span display adapter
        TYP_SP_ADAPTER_UNDEF            ///<   undefined
    } ESPAdapterTyp;

    DISPLAY_DEVICE _AdapterInfo;  ///<   info about the adapter
    int            _iAdapter;     ///<   index of the adapter in system enumeration order

    std::vector<DEVMODE> _lDevModes;            ///<   list with DEVMODE structures, defining the supported
                                                ///<   modes of the adapter
    std::vector<DisplayDeviceCont> _lDispDevI;  ///<   list of wrapped DISPLAY_DEVICES, describing all
                                                ///<   assigned m_monitors of this display adapter

    DWORD _qActiveDispDev = 0;   ///<   used to count the quantum of active
                                 ///<   display devices for this adapter
    DWORD _qDesktopDispDev = 0;  ///<   used to count the quantum of desktop attached display devices
                                 ///<   for this adapter
    ESPAdapterTyp _tAdapter = TYP_SP_ADAPTER_UNSPECIFIC;  ///<   used to specify the display adapter
                                                          ///<   in detail
                                                          ///<  @see ESPAdapterTyp for details
    DisplayDevice* _pDisplay = NULL;                      ///<   pointer to the assigned display (part of the desktop)
    int            _qTmp;                                 ///<   temporarly attribute, !! do not use !!
    char           _Name[c_DefNameBufferSz];              ///<   name of the display adapter
    std::string    friendlyName;                          ///<	D# RegInfo._strFriendlyName (RegInfo._Name)

    DispAdapter() {
        ZeroMemory(&_AdapterInfo, sizeof(DISPLAY_DEVICE));
        _AdapterInfo.cb = sizeof(DISPLAY_DEVICE);
    }

    /** Returns the pointer to the first active display device information
     * @return pointer             if found
     * @return NULL                not found */
    DisplayDeviceCont* GetFirstActiveDispDevI() {
        auto pDDC = find_if(_lDispDevI.begin(), _lDispDevI.end(), [](const DisplayDeviceCont& s) { return s.bActive; });
        return pDDC != _lDispDevI.end() ? &(*pDDC) : nullptr;
    }

    /** Searchs in the given source list a display adapter (graphics card) that
     * has\n a display device container with specified display id string
     * @param  iDDC                if found, retrieves the index of the matching
     * display device container, c_NotAssignedIndex otherwise
     * @param  pDispID             pointer to the display id of interest
     * @param  pSrcL               pointer to the list to search within
     * @param  bActiveOnly         if TRUE, only active display device container
     * are interest
     * @return pointer             if found
     * @return NULL                otherwise */
    DispAdapter* GetBy_DisplayID(int& iDDC, wchar_t* pDispID, std::vector<DispAdapter>* pSrcL, int bActiveOnly = TRUE);

    /** Searchs in the given source list a display adapter that has\n
     *  a display device container with specified interface name.\n
     * @param  iDDC                if found, retrieves the index of the matching
     * display device container, c_NotAssignedIndex otherwise
     * @param  pName               pointer to the interface name of interest
     * @param  pSrcL               pointer to the list to search within
     * @param  bActiveOnly         if TRUE, only active display device container
     * are interest
     * @return pointer             if found
     * @return NULL                otherwise */
    DispAdapter* GetBy_InterfaceName(int& iDDC, wchar_t* pName, std::vector<DispAdapter>* pSrcL,
                                     int bActiveOnly = TRUE);

    /** Searchs in the given source list a display adapter matches\n
     *  the specified desktop rectangle.\n
     * @param  iDDC                if found, retrieves the index of the matching
     * display device container, c_NotAssignedIndex otherwise
     * @param  pDispDeskRect       pointer to display deskop rectangle of
     * interest
     * @param  pSrcL               pointer to the list to search within
     * @return pointer             if found
     * @return NULL                otherwise */
    DispAdapter* GetBy_DesktopRect(int& iDDC, RECT* pDispDeskRect, std::vector<DispAdapter>* pSrcL);
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------

/** DispRegistryInfo\n
 *  Container to collect informations a display, based on informations from the
 * window registry.
 * @author Johannes Mueller
 * @author Sven Hahne
 * @date Jan. 20
 * @version 1.0
 * @bug No.
 * @todo Nothing. */
class DispRegistryInfo {
public:
    typedef enum {
        FLAG_FILLED_NONE             = 0x0,
        FLAG_FILLED_EDID_VERSION     = 0x1,
        FLAG_FILLED_EDID_PREFTIMING  = 0x2,
        FLAG_FILLED_EDID_STR_SERIAL  = 0x4,
        FLAG_FILLED_EDID_STR_NAME    = 0x8,
        FLAG_FILLED_EDID_STR_MISC    = 0x10,
        FLAG_FILLED_EDID_RANGE_LIMIT = 0x20
    } EFilledFlag;

    bool m_filled  = false;                       ///<   indicates valid data
    int  m_fFilled = 0;                           ///<   indicates the valid fill of the available informations
                                                  ///<  @see SSPHWS_DispRegistryInfo::EFilledFlag
    int _iFirstActiveVolI = -1;                   ///<   contains the index of the first active marked volatile
                                                  ///<   information, or c_NotAssignedIndex
    DWORD _qActiveVolI = 0;                       ///<   holds the quantum of active marked volatile
                                                  ///<   display id informations  (from registry)
    DWORD   _iKeyEnum      = 0;                   ///<   holds the original registry key enumeration index
    DWORD   _fCapabilities = 0;                   ///<   flag, describing the capabilities of this display
    wchar_t _strHWID[c_MaxRegistryValueSzWChar];  ///<   string identifing the
                                                  ///<   display

    BYTE _iEDIDVer  = 0;  ///<   EDID version number, or 0
    BYTE _iEDIDRev  = 0;  ///<   EDID revision number, or 0
    long _szDeviceX = 0;  ///<   holds the physical display size in x direction
    long _szDeviceY = 0;  ///<   holds the physical display size in y direction
    int  _bDigital  = 0;  ///<   indicates a digital device

    char _strFriendlyName[c_DefNameBufferSz];  ///<   contains a friendly name
                                               ///<   of the display, read from
                                               ///<   EDID, if set
    char _strSerial[c_DefNameBufferSz];        ///<   contains a string with a serial
                                               ///<   number of the display, read from
                                               ///<   EDID, if set
    char _strMisc[c_DefNameBufferSz];          ///<   contains additional informations to
                                               ///<   the display, read from EDID, if set

    DispStdTimingInfo       _PrefTiming;       ///<   defines the prefered modus of this display
    DispFreqRangeLimitsInfo _FreqRangeLimits;  ///<   defines the frequenzy range limits of the
                                               ///<   display

    const size_t c_Limit_NoneDynamic;  ///<   !! has to be the last none dynamic
                                       ///<   variable of this structure !!

    std::vector<uint8_t>           _lEDID;              ///<   list of bytes containing the EDID buffer data
    std::vector<DispStdTimingInfo> _lStdTimings;        ///<   list with recognized standard timing informations
                                                        ///<   (from EDID)
    std::vector<DispRegistryVolatileInfo> _lVolatileI;  ///<   list with the volatile informations of the display
                                                        ///<   registry reprasentation

    char _Name[c_DefNameBufferSz];  ///<   short name of the display

protected:
    /** Set the list interface attributes to default values. */
    void DefaultListInterface();

    /** Set the list interface attributes to default values. */
    void DefaultEDIDParam();

public:
    DispRegistryInfo();
    ~DispRegistryInfo() = default;

    /** Tries to identify and parse the EDID buffer.  */
    bool ParseEDID();

    /** Tries to fill the give device mode container if\n
     *  the informations from the preferred timing informations. */
    void GetPreferredDevMode(DEVMODE& prefMode);

    /** Searchs for the specified key string and returns a pointer to the\n
     *  volatile information container if found.
     * @param  pKeyStr             pointer to the key string to search for
     * @param  qKeyStr             size of pKeyStr
     * @return pointer             if found
     * @return NULL                if not found */
    DispRegistryVolatileInfo* GetBy_PnPKey(wchar_t* pKeyStr, DWORD qKeyStr);

    /** Test if the container is filled regular */
    bool IsFilled() { return m_filled; }

    /** Test if the container is filled regular and if the the EDID was parsed
     * successfull.*/
    bool IsEDIDParsed() { return (m_filled && (m_fFilled & FLAG_FILLED_EDID_VERSION)); }

    /** Test if the container is filled regular and if the this display was
     * marked as currently used. */
    bool IsDisplayActive() { return (m_filled && _qActiveVolI); }

    /** Test if the container is filled with regular prefered timing values. */
    bool IsPreferredTimingInfoFilled() { return (IsEDIDParsed() && (m_fFilled & FLAG_FILLED_EDID_PREFTIMING)); }

    /** Test if the container is filled with regular prefered timing values. */
    bool MaybeAProjector() { return (IsEDIDParsed() && ((_szDeviceX <= 0) || (_szDeviceY <= 0))); }

    /** Test if the container is filled with regular serial string, read from
     * EDID.*/
    bool IsDisplaySerialStrFilled() { return (IsEDIDParsed() && (m_fFilled & FLAG_FILLED_EDID_STR_SERIAL)); }

    /** Test if the container is filled with regular common information string,
     * read from EDID.*/
    bool IsDisplayMiscStrFilled() { return (IsEDIDParsed() && (m_fFilled & FLAG_FILLED_EDID_STR_MISC)); }

    /** Test if the container is filled with regular name string, read from
     * EDID.*/
    bool IsDisplayNameStrFilled() { return (IsEDIDParsed() && (m_fFilled & FLAG_FILLED_EDID_STR_NAME)); }

    /** Test if the container is filled with regular display range limits, read
     * from EDID. */
    bool IsDisplayFreqRangeLimitsFilled() { return (IsEDIDParsed() && (m_fFilled & FLAG_FILLED_EDID_RANGE_LIMIT)); }

    /** Calculates the preferred refresh rate for the given values.
     * @param  refreshRate         [out] returns the calculated value
     * @param  pxClock             [in]  pixel clock frequency in 10kHz
     * @param  width               [in]  size of resolution in x direction
     * @param  height              [in]  size of resolution in y direction
     * @param  bInterlaced         [in]  indicates an interlaced mode */
    void GetPreferedRefreshRate(long& refreshRate, WORD pxClock, long width, long height, int bInterlaced = FALSE);

    /** Parse the given device mode list to find a compatible device mode.
     * @param  compMode            [out] returns the device mode informations
     * @param  lModes              [in]  list with device mode informations to
     * parse
     * @param  pDefDevMode         [in]  optional pointer t */
    bool GetCompatibleDisplayMode(DEVMODE& compMode, std::vector<DEVMODE>& lModes, DEVMODE* pDefDevMode);

    /** Parse the given device mode list to find a compatible device mode.
     * @param  compMode            [out] returns the device mode informations
     * @param  lModes              [in]  list with device mode informations to
     * parse
     * @param  pDefDevMode         [in]  optional pointer t
     * @param  dispW               [in]  defines the desired display size for x
     * direction
     * @param  dispH               [in]  defines the desired display size for y
     * direction
     * @param  refreshFreq         [in]  defines the desired vertical display
     * refresh rate */
    bool GetCompatibleDisplayMode(DEVMODE& compMode, std::vector<DEVMODE>& lModes, DEVMODE* pDefDevMode, DWORD dispW,
                                  DWORD dispH, DWORD refreshFreq = -1);

    /** Returns a compatible mode for the specified display adapter.
     * @param  compMode            [out] returns the device mode informations
     * @param  pAdapterName        [in]  name of the adapter, a compatible mode
     * is to determine*/
    bool GetCompatibleDisplayMode(DEVMODE& compMode, wchar_t* pAdapterName);

    /** Returns a compatible mode for the specified display adapter.
     * @param  compMode            [out] returns the device mode informations
     * @param  pAdapterName        [in]  name of the adapter, a compatible mode
     * is to determine
     * @param  dispW               [in]  defines the desired display size for x
     * direction
     * @param  dispH               [in]  defines the desired display size for y
     * direction
     * @param  refreshFreq         [in]  defines the desired vertical display
     * refresh rate */
    bool GetCompatibleDisplayMode(DEVMODE& compMode, wchar_t* pAdapterName, DWORD dispW, DWORD dispH,
                                  DWORD refreshFreq = -1);

    /** Parse the given device mode list to find similar device mode
     * informations.
     * @param  lSimilar            [out] list with similar device mode
     * informations
     * @param  lModes              [in]  list with device mode informations to
     * parse
     * @param  dispW               [in]  defines the display size for x
     * direction to search for
     * @param  dispH               [in]  defines the display size for y
     * direction to search for
     * @param  refreshFreq         [in]  defines the vertical display refresh
     * rate to search for */
    bool GetSimilarModes(std::vector<DEVMODE>& lSimilar, std::vector<DEVMODE>& lModes, DWORD dispW, DWORD dispH,
                         DWORD refreshFreq);

    /** Parse the given device mode list to find the mode with the highest bit
     * per pixel count.
     * @param  lModes              [in]  list with device mode informations to
     * parse
     * @param  dispW               [in]  defines the display size for x
     * direction to search for
     * @param  dispH               [in]  defines the display size for y
     * direction to search for
     * @param  refreshFreq         [in]  defines the vertical display refresh
     * rate to search for
     * @param  bppMax              [in]  optional, can be used to define a
     * maximum bpp, 0 for unused
     * @return bpp                 highest bit per pixel count found
     * @return 0                   could not determine  */
    DWORD GetMaximumBpp(std::vector<DEVMODE>& lModes, DWORD dispW, DWORD dispH, DWORD refreshFreq, DWORD bppMax = 0);
};

static const size_t c_SPDispRegistryInfo_NDDSz = offsetof(DispRegistryInfo, c_Limit_NoneDynamic);

/** ExternApiDisplayIdent\n
 *  ( Extern API Display Identification )\n
 *  Container used to specify a display, found with an extern GPU API.\n
 * !! This container being used as coherent memory block. !!\n
 * !! Never declare dynamic attributes in this container. !!\n
 * @author Johannes Mueller
 * @author Sven Hahne
 * @date Jan. 20
 * @version 1.0
 * @bug No.
 * @todo Nothing. */
typedef struct ExternApiDisplayIdent {
public:
    char strAPIGPU[c_DefNameBufferSz];           ///<   stores the name of the graphic
                                                 ///<   card
    ESPGPUVendorTyp tGPUVendor;                  ///<   specifies GPU vendor
                                                 ///<  @see ESPGPUVendorTyp for details
    int iApiGPU;                                 ///<   stores the index of the graphic card in extern API
                                                 ///<   order
    int iAPIDisplayEnum;                         ///<   stores the enumeration index of the display in
                                                 ///<   extern API order
    char strApiDisplayIdent[c_DefNameBufferSz];  ///<   stores the identification
                                                 ///<   string, the extern API
                                                 ///<   assigned to the display
    char strDisplayName[c_DefNameBufferSz];      ///<   stores the assigned display
                                                 ///<   name

public:
    ExternApiDisplayIdent();
    ~ExternApiDisplayIdent() = default;

    /** Test if the extern API depended informations are valid filled.
     * @return TRUE                valid filled
     * @return FALSE               not valid filled */
    int ExternAPIInfoValid() {
        return (strAPIGPU[0] && (iApiGPU >= 0) && (iAPIDisplayEnum >= 0) && strApiDisplayIdent[0]) ? TRUE : FALSE;
    }

    /** Test if entry if specified identification string is in source list.\n
     * @param  pDst                reference to receive the pointer to ident
     * container if found, NULL otherwise
     * @param  pStrIdent           pointer to identification string of interest
     * @param  lSrc                reference to source list
     * @return TRUE                found
     * @return FALSE               not found */
    static int IsDisplayIdentInList(ExternApiDisplayIdent*& pDst, char* pStrIdent,
                                    std::list<ExternApiDisplayIdent> lSrc) {
        if (lSrc.size() && pStrIdent && pStrIdent[0])
            for (auto& it : lSrc)
                if (!strcmp(it.strApiDisplayIdent, pStrIdent)) return true;
        return false;
    }

} ExternApiDisplayIdent;

/** NVApiGPUInfo\n
 *  ( Struct SmartProjector HardWare Support NVIDIA API GPU Information )\n
 *  Container to collect informations about a physical Nvidia GPU.\n
 * @author Johannes Mueller
 * @date Nov. 17
 * @version 1.0
 * @bug No.
 * @todo Nothing. */
/*
typedef struct SPHWS_NVApiGPUInfo
{

public:
    NvPhysicalGpuHandle                 _hGPU;
///<   stores the physical GPU handle NvU32 _BusID;
///<   stores the bus id, the GPU is connected NvU32 _BusSlotID;
///<   stores the slot id NvAPI_ShortString                   _strDesc;
///<   stores the description string of the GPU const size_t
c_Limit_NoneDynamic;					///<   !! has to be the
last none dynamic variable of this structure !! std::list<NVApiDisplayInfo>
_lDisplayI;							///<   stores
informations about connected displays to the treated GPU
    std::list<NVApiSocketAssignInfo>     _lConnector;
///<   stores the informations that are assigned to the connector of the GPU

public:
    NVApiGPUInfo();
    NVApiGPUInfo(NvPhysicalGpuHandle hGPU);
    ~NVApiGPUInfo();
*/
/** Tries to determine the signal quantums\n
 *  for specified monitor connector typ.\n
 * @param  qDigital            reference to receive the quantum of digital
 * signal typs
 * @param  qAnalog             reference to receive the quantum of analog signal
 * typs
 * @param  tMonConn            monitor connector typ of interest
 * @see    NV_MONITOR_CONN_TYPE for details
 * @return none */
// void GetSignalQByConnectorTyp(size_t& qDigital, size_t& qAnalog,
// NV_MONITOR_CONN_TYPE tMonConn);

/** Tries to fill the connector assign list.\n */
// int FillConnectorAssignList();

/** Tries to fill the connector assign list.\n
 * @param  pPSLDB              pointer to prodct socket layout database to use
 * @return TRUE                okay
 * @return FALSE               could not fill or an error occurs */
// int FillConnectorAssignListFromDB(GPUProductSocketLayoutDB* pPSLDB);

//} NVApiGPUInfo;

//-------------------------------------------------------------------------------------------------------------------------------------------------------

class OSDisplayInfo {
public:
    OSDisplayInfo();
    virtual ~OSDisplayInfo();

    bool SetDisplayInfo(std::list<DispAdapter>& dispAdapters, DisplayDevice& disp);
    std::list<std::unique_ptr<DispRegistryInfo>> InitFromRegistry();
    void                                         IdentifyActiveAdapterDisplay(std::list<DispAdapter>& dispAr);
    void                                         BuildFriendlyNames(std::list<DispAdapter>&                       dispAdapters,
                                                                    std::list<std::unique_ptr<DispRegistryInfo>>& regInfo);

    // bool SetInitialDisplayNames(list<ExternApiDisplayIdent>& lDst, int* pErr,
    // DWORD timeOut);
    DispRegistryInfo* FindDisplayRegInfo(wchar_t* strDispID);
    bool              IsATIMultiView(std::vector<DispAdapter*>& lAdapter, std::vector<DisplayDeviceCont*>& lDDCPtr,
                                     std::vector<DispAdapter*>& lSrc, long iStart);
    bool              GetExternApiDisplayIdentList(std::list<ExternApiDisplayIdent>& lDst, int* pErr, DWORD tmOut);

    std::list<std::unique_ptr<DispRegistryInfo>>& registryInfo();
    std::vector<DisplayDevice>&                   displayInfo();
    std::list<DispAdapter*>&                      displayAdapters();
    std::string                                   getFriendlyNameForDevName(std::string& devName);

private:
    std::list<std::unique_ptr<DispRegistryInfo>> m_DispRegInfo;
    std::vector<DisplayDevice>                   m_displays;
    std::list<DispAdapter>                       m_dispAdapters;
    std::list<DispAdapter*>                      m_activeAdapters;
};

static BOOL CALLBACK MonitorEnumProcCb(HMONITOR hMonitor, HDC hDC, LPRECT pRect, LPARAM pData);

}  // namespace ara

#endif