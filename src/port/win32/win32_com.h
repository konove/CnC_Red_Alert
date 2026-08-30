// File: Just enough COM for the Westwood Online client to compile and be
// type-checked off Windows.
//
// The WOL client talks to wolapi.dll through COM: `IChat`, `IDownload` and
// `INetUtil` are in-process COM interfaces declared by a MIDL-generated header,
// and Red Alert implements the matching event sinks by hand. None of that can
// run here -- there is no wolapi.dll, and the servers it dialled have been gone
// for two decades -- but all of it has to compile, or the WOL branches cannot
// be type-checked at all.
//
// So this header declares the vocabulary (HRESULT, GUID, IUnknown, connection
// points) and CreateComInstance() always fails. Every WOL code path already
// handles that failure: it is what the game did when wolapi.dll was missing.
// Failing is therefore both the honest answer and the one the 1998 code is
// prepared for.
//
// Names follow the Win32 SDK rather than the project's Google style, because
// the call sites spell them that way. On Windows the real SDK headers are used.

#ifndef CNC_RED_ALERT_PORT_WIN32_WIN32_COM_H_
#define CNC_RED_ALERT_PORT_WIN32_WIN32_COM_H_

#ifdef _WIN32

#include <objbase.h>
#include <windows.h>

#else

#include <cstdint>

#include "port/win32/win32_types.h"

// MIDL output and the code around it lean on these spellings. `interface` is a
// macro on Windows too (objbase.h defines it as `struct`), which is why the
// generated header can say `interface IChat : public IUnknown`.
#define interface struct
#define STDMETHODCALLTYPE
#define STDMETHOD(method) virtual HRESULT method
#define STDMETHOD_(type, method) virtual type method
#define STDMETHODIMP HRESULT
#define STDMETHODIMP_(type) type
#define EXTERN_C extern "C"
#define DECLSPEC_UUID(x)
#define MIDL_INTERFACE(x) struct
#define CONST_VTBL const
#define BEGIN_INTERFACE
#define END_INTERFACE

// Reserved to the implementation, and spelled this way by every line of MIDL
// output -- renaming them is not on offer. Silenced here rather than left to
// warn from each of the thousands of lines that use them, which also keeps the
// preprocessor quiet enough for clang-tidy-cache to hash the TU (see
// CLAUDE.md).
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#endif
#define __RPC_FAR
#define __RPC_USER
#define __RPC_STUB

using HRESULT = std::int32_t;
using SCODE = std::int32_t;

constexpr bool SUCCEEDED(HRESULT status) { return status >= 0; }
constexpr bool FAILED(HRESULT status) { return status < 0; }

inline constexpr int SEVERITY_SUCCESS = 0;
inline constexpr int SEVERITY_ERROR = 1;
inline constexpr int FACILITY_NULL = 0;
inline constexpr int FACILITY_ITF = 4;
inline constexpr int FACILITY_WIN32 = 7;

// The WOL error tables (wolapi/chatdefs.h and friends) are built entirely out
// of this, so it has to fold at compile time.
constexpr HRESULT MAKE_HRESULT(int severity, int facility, int code) {
  return static_cast<HRESULT>((static_cast<std::uint32_t>(severity) << 31U) |
                              (static_cast<std::uint32_t>(facility) << 16U) |
                              static_cast<std::uint32_t>(code));
}

inline constexpr HRESULT S_OK = 0;
inline constexpr HRESULT S_FALSE = 1;
inline constexpr HRESULT E_FAIL = static_cast<HRESULT>(0x80004005U);
inline constexpr HRESULT E_NOINTERFACE = static_cast<HRESULT>(0x80004002U);
inline constexpr HRESULT E_POINTER = static_cast<HRESULT>(0x80004003U);
inline constexpr HRESULT E_INVALIDARG = static_cast<HRESULT>(0x80070057U);
inline constexpr HRESULT E_OUTOFMEMORY = static_cast<HRESULT>(0x8007000EU);
inline constexpr HRESULT E_NOTIMPL = static_cast<HRESULT>(0x80004001U);
inline constexpr HRESULT REGDB_E_CLASSNOTREG =
    static_cast<HRESULT>(0x80040154U);

// Layout matches the Win32 GUID, so the brace initializers in MIDL's companion
// wolapi_i.c stay correct. Windows spells the first field `unsigned long`,
// which would be 8 bytes here; the fixed-width types keep it at 16.
//
// The guard names are the ones wolapi_i.c actually tests, so defining them
// stops it declaring a second, differently sized IID.
#define __IID_DEFINED__
#define CLSID_DEFINED
#ifdef __clang__
#pragma clang diagnostic pop
#endif
struct GUID {
  std::uint32_t Data1;
  std::uint16_t Data2;
  std::uint16_t Data3;
  std::uint8_t Data4[8];

  friend constexpr bool operator==(const GUID&, const GUID&) = default;
};
static_assert(sizeof(GUID) == 16);

using IID = GUID;
using CLSID = GUID;
using REFIID = const IID&;
using REFCLSID = const CLSID&;

// The base of every COM interface. Unlike the real IUnknown this has a virtual
// destructor: nothing here is ever handed across a binary boundary, and without
// it every derived sink trips -Wnon-virtual-dtor.
interface IUnknown {
  virtual HRESULT QueryInterface(REFIID iid, void** object) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;

  // Defined out of line so each interface has a key function and its vtable is
  // emitted once rather than in every translation unit.
  virtual ~IUnknown();
};

// Event sinks are attached to a COM object through these two. The WOL client
// uses only Advise/Unadvise and FindConnectionPoint.
interface IConnectionPoint : public IUnknown {
  virtual HRESULT Advise(IUnknown * sink, DWORD * cookie) = 0;
  virtual HRESULT Unadvise(DWORD cookie) = 0;
  ~IConnectionPoint() override;
};

interface IConnectionPointContainer : public IUnknown {
  virtual HRESULT FindConnectionPoint(REFIID iid,
                                      IConnectionPoint * *point) = 0;
  ~IConnectionPointContainer() override;
};

extern const IID IID_IUnknown;
extern const IID IID_IConnectionPoint;
extern const IID IID_IConnectionPointContainer;

inline constexpr DWORD CLSCTX_INPROC_SERVER = 0x1;

// Apartment setup is a no-op: nothing is ever created, so there is nothing to
// marshal.
HRESULT CoInitialize(void* reserved);
void CoUninitialize();

// Always fails with REGDB_E_CLASSNOTREG -- "class not registered" -- which is
// exactly what Windows returned when wolapi.dll was not installed, and leaves
// `*object` null so a caller that ignores the result still sees no object.
HRESULT CoCreateInstance(REFCLSID clsid, IUnknown* outer, DWORD context,
                         REFIID iid, void** object);

#endif  // _WIN32

#endif  // CNC_RED_ALERT_PORT_WIN32_WIN32_COM_H_
