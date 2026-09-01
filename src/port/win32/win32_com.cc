#include "port/win32/win32_com.h"

#ifndef _WIN32

// The published values, so that a sink comparing an incoming iid against
// IID_IUnknown behaves the way it would on Windows.
const IID IID_IUnknown = {0x00000000,
                          0x0000,
                          0x0000,
                          {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const IID IID_IConnectionPoint = {
    0xB196B286,
    0xBAB4,
    0x101A,
    {0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07}};
const IID IID_IConnectionPointContainer = {
    0xB196B284,
    0xBAB4,
    0x101A,
    {0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07}};

IUnknown::~IUnknown() = default;
IConnectionPoint::~IConnectionPoint() = default;
IConnectionPointContainer::~IConnectionPointContainer() = default;

HRESULT CoCreateInstance(REFCLSID /*clsid*/, IUnknown* /*outer*/,
                         DWORD /*context*/, REFIID /*iid*/, void** object) {
  if (object != nullptr) {
    *object = nullptr;
  }
  return REGDB_E_CLASSNOTREG;
}

#endif  // _WIN32
