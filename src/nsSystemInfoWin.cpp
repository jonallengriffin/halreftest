/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Halreftest.
 *
 * The Initial Developer of the Original Code is Mozilla.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonathan Griffin <jgriffin@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsSystemInfoWin.h"
#include <windows.h>
#include <stdlib.h>

static int GetKeyValue(const char* keyLocation, const char* keyName, nsAString* destString, int type)
{
  HKEY key;
  DWORD dwcbData;
  WCHAR wCharValue[1024];
  char mbCharValue[1024];
  TCHAR tCharValue[1024];
  size_t convertedChars = 0;
  DWORD dValue;
  LONG result;
  int retval = NS_OK;
  
  result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyLocation, 0, KEY_QUERY_VALUE, &key);
  if (result != ERROR_SUCCESS) {
    return NS_ERROR_FAILURE;
  }

  switch (type) {
    case REG_BINARY: {
      // This is actually a wide string, that we must convert to a multi-byte string
      dwcbData = sizeof(wCharValue);
      result = RegQueryValueEx(key, keyName, 0, NULL, (LPBYTE)wCharValue, &dwcbData);
      if (result != ERROR_SUCCESS) {
        retval = NS_ERROR_FAILURE;
      }
      convertedChars = 0;
      wcstombs_s(&convertedChars, mbCharValue, sizeof(mbCharValue), wCharValue, wcslen(wCharValue));
      destString->AssignLiteral(mbCharValue);
      break;
    }
    case REG_DWORD: {
      // We only use this for vram size
      dwcbData = sizeof(dValue);
      result = RegQueryValueEx(key, keyName, 0, NULL, (LPBYTE)&dValue, &dwcbData);
      if (result != ERROR_SUCCESS) {
        retval = NS_ERROR_FAILURE;
      }
      dValue = dValue / 1024 / 1024;
      destString->AppendInt(dValue);
      break;
    }
    case REG_MULTI_SZ: {
      // A chain of null-separated strings; we convert the nulls to spaces
      dwcbData = sizeof(tCharValue);
      result = RegQueryValueEx(key, keyName, 0, NULL, (LPBYTE)tCharValue, &dwcbData);
      if (result != ERROR_SUCCESS) {
        retval = NS_ERROR_FAILURE;
      }
      for (DWORD i = 0; i < dwcbData - 1; i++) {
        if (tCharValue[i] == '\0')
          tCharValue[i] = ' ';
      }
      destString->AssignLiteral(tCharValue);
      break;
    }
  }
  RegCloseKey(key);
  
  return retval;
}

// The driver ID is a string like PCI\VEN_15AD&DEV_0405&SUBSYS_040515AD, possibly
// followed by &REV_XXXX.  We uppercase the string, and strip the &REV_ part
// from it, if found.
static void normalizeDriverId(nsCString& driverid) {
  ToUpperCase(driverid);
  PRInt32 rev = driverid.Find(NS_LITERAL_CSTRING("&REV_"));
  if (rev != -1) {
    driverid.Cut(rev, driverid.Length());
  }
}

static void getDriverDetails(nsCString& driverId, nsString& aDriverVersion, nsString& aDriverDate)
{
  HKEY key, subkey;
  LONG result, enumresult;
  DWORD index = 0;
  char subkeyname[64];
  TCHAR value[128];
  DWORD dwcbData = sizeof(subkeyname);
  
  // "{4D36E968-E325-11CE-BFC1-08002BE10318}" is the display class
  result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        "System\\CurrentControlSet\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}", 
                        0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &key);
  if (result != ERROR_SUCCESS) {
    return;
  }
  
  nsCString wantedDriverId(driverId);
  normalizeDriverId(wantedDriverId);
  
  while((enumresult = RegEnumKeyEx(key, index, subkeyname, &dwcbData, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS) {
    result = RegOpenKeyEx(key, subkeyname, 0, KEY_QUERY_VALUE, &subkey);
    if (result == ERROR_SUCCESS) {
      dwcbData = sizeof(value);
      result = RegQueryValueEx(subkey, "MatchingDeviceId", 0, NULL, (LPBYTE)value, &dwcbData);
      if (result == ERROR_SUCCESS) {
        nsCString matchingDeviceId(value);
        normalizeDriverId(matchingDeviceId);
        if (wantedDriverId.Equals(matchingDeviceId)) {
          result = RegQueryValueEx(subkey, "DriverVersion", 0, NULL, (LPBYTE)value, &dwcbData);
          if (result == ERROR_SUCCESS) 
            aDriverVersion.AssignLiteral(value);
          result = RegQueryValueEx(subkey, "DriverDate", 0, NULL, (LPBYTE)value, &dwcbData);
          if (result == ERROR_SUCCESS) 
            aDriverDate.AssignLiteral(value);
          break;
        }
      }
      RegCloseKey(subkey);
    }
    index++;
    dwcbData = sizeof(subkeyname);
  }
  
  RegCloseKey(key);
  return;
}

NS_IMPL_ISUPPORTS1(nsSystemInfo, nsISystemInfo)

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

nsresult nsSystemInfo::Init() 
{
  DISPLAY_DEVICE lpDisplayDevice;
  lpDisplayDevice.cb = sizeof(lpDisplayDevice);
  int deviceIndex = 0;
  
  while (EnumDisplayDevices(NULL, deviceIndex, &lpDisplayDevice, 0)) {
    if (lpDisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
      break;
    deviceIndex++;
  }
  mDeviceKey = lpDisplayDevice.DeviceKey;
  mDeviceID = lpDisplayDevice.DeviceID;
  mDeviceString.AssignLiteral(lpDisplayDevice.DeviceString);

  DEVMODE dm;
  dm.dmSize = sizeof(dm);
  EnumDisplaySettings(lpDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &dm);
  mBitsPerPixel = dm.dmBitsPerPel;
  mPixelWidth = dm.dmPelsWidth;
  mPixelHeight = dm.dmPelsHeight;
  
  getDriverDetails(mDeviceID, mDriverVersion, mDriverDate);
  return NS_OK;
}

/* readonly attribute DOMString DisplayAdapter; */
NS_IMETHODIMP nsSystemInfo::GetDisplayAdapter(nsAString & aDisplayAdapter)
{
  aDisplayAdapter.Assign(mDeviceString);
  return NS_OK;
}

/* readonly attribute DOMString DisplayChipset; */
NS_IMETHODIMP nsSystemInfo::GetDisplayChipset(nsAString & aDisplayChipset)
{
  return GetKeyValue((char*)mDeviceKey.BeginReading() + 18, "HardwareInformation.ChipType", &aDisplayChipset, REG_BINARY);
}

/* readonly attribute DOMString DisplayRAM; */
NS_IMETHODIMP nsSystemInfo::GetDisplayRAM(nsAString & aDisplayRAM)
{
  return GetKeyValue((char*)mDeviceKey.BeginReading() + 18, "HardwareInformation.MemorySize", &aDisplayRAM, REG_DWORD);
}

/* readonly attribute DOMString DisplayDriver; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriver(nsAString & aDisplayDriver)
{
  return GetKeyValue((char*)mDeviceKey.BeginReading() + 18, "InstalledDisplayDrivers", &aDisplayDriver, REG_MULTI_SZ);
}

/* readonly attribute DOMString DisplayDriverVersion; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriverVersion(nsAString & aDisplayDriverVersion)
{
  aDisplayDriverVersion.Assign(mDriverVersion);
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriverDate; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriverDate(nsAString & aDisplayDriverDate)
{
  aDisplayDriverDate.Assign(mDriverDate);
  return NS_OK;
}

/* readonly attribute DOMString DisplayWidth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayWidth(nsAString & aDisplayWidth)
{
  aDisplayWidth.AppendInt(mPixelWidth);
  return NS_OK;
}

/* readonly attribute DOMString DisplayHeight; */
NS_IMETHODIMP nsSystemInfo::GetDisplayHeight(nsAString & aDisplayHeight)
{
  aDisplayHeight.AppendInt(mPixelHeight);
  return NS_OK;
}

/* readonly attribute DOMString DisplayColorDepth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayColorDepth(nsAString & aDisplayColorDepth)
{
  aDisplayColorDepth.AppendInt(mBitsPerPixel);
  return NS_OK;
}

/* readonly attribute DOMString DisplayVendorID; */
NS_IMETHODIMP nsSystemInfo::GetDisplayVendorID(nsAString & aDisplayVendorID)
{
  nsCString DeviceId(mDeviceID);
  ToUpperCase(DeviceId);
  PRInt32 start = DeviceId.Find(NS_LITERAL_CSTRING("VEN_"));
  PRInt32 end = DeviceId.Find(NS_LITERAL_CSTRING("&"), start);
  DeviceId.Cut(end, DeviceId.Length());
  DeviceId.Cut(0, start + 4);
  DeviceId.Insert("0x", 0, 2);
  aDisplayVendorID.AssignLiteral(DeviceId.BeginReading());
  return NS_OK;
}
