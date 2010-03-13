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

#include "nsSystemInfoMac.h"
#include "nsStringAPI.h"

#import <IOKit/Graphics/IOFrameBufferShared.h>
#import <Carbon/Carbon.h>

typedef const void *CGSConnectionID;
extern CGSConnectionID _CGSDefaultConnection();

NS_IMPL_ISUPPORTS1(nsSystemInfo, nsISystemInfo)

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

nsresult nsSystemInfo::Init() 
{
  this->displayID = CGMainDisplayID();
  return NS_OK;
}

/* readonly attribute DOMString DisplayAdapter; */
NS_IMETHODIMP nsSystemInfo::GetDisplayAdapter(nsAString & aDisplayAdapter)
{
  io_registry_entry_t dspPort;
  CFDataRef model;

  // Get the I/O Kit service port for the display
  dspPort = CGDisplayIOServicePort(this->displayID);

  model = (CFDataRef)IORegistryEntrySearchCFProperty(dspPort,kIOServicePlane,CFSTR("model"),
          kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
  NSString* modelOut  = [[NSString alloc] initWithBytes:CFDataGetBytePtr(model) 
                                          length:CFDataGetLength(model) 
                                          encoding:NSUTF8StringEncoding];

  aDisplayAdapter.AssignLiteral([modelOut UTF8String]);
  return NS_OK;
}

/* readonly attribute DOMString DisplayChipset; */
NS_IMETHODIMP nsSystemInfo::GetDisplayChipset(nsAString & aDisplayChipset)
{
  io_registry_entry_t dspPort;
  CFDataRef deviceID;

  // Get the I/O Kit service port for the display
  dspPort = CGDisplayIOServicePort(this->displayID);

  deviceID = (CFDataRef)IORegistryEntrySearchCFProperty(dspPort,kIOServicePlane,CFSTR("device-id"),
             kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
  NSString* deviceIDOut = [NSString stringWithFormat:@"0x%04X",*((UInt32*)CFDataGetBytePtr(deviceID))];

  aDisplayChipset.AssignLiteral([deviceIDOut UTF8String]);
  return NS_OK;
}

/* readonly attribute DOMString DisplayVendorID; */
NS_IMETHODIMP nsSystemInfo::GetDisplayVendorID(nsAString & aDisplayVendorID)
{
  io_registry_entry_t dspPort;
  CFDataRef vendorID;

  // Get the I/O Kit service port for the display
  dspPort = CGDisplayIOServicePort(this->displayID);

  vendorID = (CFDataRef)IORegistryEntrySearchCFProperty(dspPort,kIOServicePlane,CFSTR("vendor-id"),
             kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
  NSString* vendorIDOut = [NSString stringWithFormat:@"0x%04X",*((UInt32*)CFDataGetBytePtr(vendorID))];

  aDisplayVendorID.AssignLiteral([vendorIDOut UTF8String]);
  return NS_OK;
}

/* readonly attribute DOMString DisplayRAM; */
NS_IMETHODIMP nsSystemInfo::GetDisplayRAM(nsAString & aDisplayRAM)
{
  io_registry_entry_t dspPort;
  CFTypeRef typeCode = 0;
  long vramSize = 0;

  // Get the I/O Kit service port for the display
  dspPort = CGDisplayIOServicePort(this->displayID);

  // Ask IOKit for the property for the display VRAM size
  typeCode = IORegistryEntrySearchCFProperty(dspPort,kIOServicePlane,CFSTR(kIOFBMemorySizeKey),
             kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);

  // Ensure we have valid data from IOKit
  if(typeCode && CFGetTypeID(typeCode) == CFNumberGetTypeID())
  {
    // If so, convert the CFNumber into a plain unsigned long
    CFNumberGetValue((CFNumberRef)typeCode, kCFNumberSInt32Type, &vramSize);
    if(typeCode)
      CFRelease(typeCode);
  }

  NSString* vramOut = [NSString stringWithFormat:@"%d",(vramSize / (1024 * 1024))];

  aDisplayRAM.AssignLiteral([vramOut UTF8String]);
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriver; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriver(nsAString & aDisplayDriver)
{
  // N/A on OSX
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriverVersion; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriverVersion(nsAString & aDisplayDriverVersion)
{
  // N/A on OSX
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriverDate; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriverDate(nsAString & aDisplayDriverDate)
{
  // N/A on OSX
  return NS_OK;
}

/* readonly attribute DOMString DisplayWidth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayWidth(nsAString & aDisplayWidth)
{
  aDisplayWidth.AppendInt(CGDisplayPixelsWide(this->displayID));
  return NS_OK;
}

/* readonly attribute DOMString DisplayHeight; */
NS_IMETHODIMP nsSystemInfo::GetDisplayHeight(nsAString & aDisplayHeight)
{
  aDisplayHeight.AppendInt(CGDisplayPixelsHigh(this->displayID));
  return NS_OK;
}

/* readonly attribute DOMString DisplayColorDepth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayColorDepth(nsAString & aDisplayColorDepth)
{
  aDisplayColorDepth.AppendInt(CGDisplayBitsPerPixel(this->displayID));
  return NS_OK;
}
