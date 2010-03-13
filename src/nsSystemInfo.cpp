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

#include "nsSystemInfo.h"
#include "nsStringAPI.h"

// XXX:  Linux port still NYI

NS_IMPL_ISUPPORTS1(nsSystemInfo, nsISystemInfo)

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

nsresult nsSystemInfo::Init() 
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayAdapter; */
NS_IMETHODIMP nsSystemInfo::GetDisplayAdapter(nsAString & aDisplayAdapter)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayChipset; */
NS_IMETHODIMP nsSystemInfo::GetDisplayChipset(nsAString & aDisplayChipset)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayRAM; */
NS_IMETHODIMP nsSystemInfo::GetDisplayRAM(nsAString & aDisplayRAM)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriver; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriver(nsAString & aDisplayDriver)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriverVersion; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriverVersion(nsAString & aDisplayDriverVersion)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriverDate; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriverDate(nsAString & aDisplayDriverDate)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayWidth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayWidth(nsAString & aDisplayWidth)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayHeight; */
NS_IMETHODIMP nsSystemInfo::GetDisplayHeight(nsAString & aDisplayHeight)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayColorDepth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayColorDepth(nsAString & aDisplayColorDepth)
{
  return NS_OK;
}

/* readonly attribute DOMString DisplayVendorID; */
NS_IMETHODIMP nsSystemInfo::GetDisplayVendorID(nsAString & aDisplayVendorID)
{
  return NS_OK;
}
