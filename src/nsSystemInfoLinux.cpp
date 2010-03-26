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
 *   Vladimir Vukicevic <vladimir@pobox.com> (GLXWrap code)
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

#include "nsSystemInfoLinux.h"
#include "glwrap.h"

extern "C" {
#include <pci/pci.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
}

#include "nsServiceManagerUtils.h"
#include "nsIConsoleService.h"

class GLXWrap : public LibrarySymbolLoader
{
public:
  GLXWrap() { }

  bool Init();

protected:

  //
  // the wrapped functions
  //
public:
  typedef PRFuncPtr (* PFNGLXGETPROCADDRESS) (const GLubyte *procName);
  PFNGLXGETPROCADDRESS fGetProcAddress;
  typedef GLXContext (* PFNGLXCREATECONTEXTPROC) (Display *dpy, XVisualInfo *visinfo, GLXContext share_list, Bool direct);
  PFNGLXCREATECONTEXTPROC fCreateContext;
  typedef void (* PFNGLXDESTROYCONTEXTPROC) (Display *dpy, GLXContext ctx);
  PFNGLXDESTROYCONTEXTPROC fDestroyContext;
  typedef const char* (* PFNGLGETSTRING) (int name);
  PFNGLGETSTRING fGetString;
  typedef Bool (* PFNGLXMAKECURRENT) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
  PFNGLXMAKECURRENT fMakeCurrent;
};

bool
GLXWrap::Init()
{
  if (fCreateContext)
    return true;

  SymLoadStruct symbols[] = {
    { (PRFuncPtr*) &fGetProcAddress, { "glXGetProcAddress", "glXGetProcAddressARB", NULL } },
    { (PRFuncPtr*) &fCreateContext, { "glXCreateContext", NULL } },
    { (PRFuncPtr*) &fDestroyContext, { "glXDestroyContext", NULL } },
    { (PRFuncPtr*) &fGetString, { "glGetString", NULL } },
    { (PRFuncPtr*) &fMakeCurrent, { "glXMakeCurrent", NULL } },
    { NULL, { NULL } }
  };

  return LoadSymbols(&symbols[0]);
}

static GLXWrap gGLXWrap;


NS_IMPL_ISUPPORTS1(nsSystemInfo, nsISystemInfo)

nsSystemInfo::nsSystemInfo()
{
}

nsSystemInfo::~nsSystemInfo()
{
}

void nsSystemInfo::Log(const char* msg) 
{
  nsCOMPtr<nsIConsoleService> console = do_GetService("@mozilla.org/consoleservice;1");
  console->LogStringMessage(NS_ConvertASCIItoUTF16(msg).get());
}

nsresult nsSystemInfo::Init() 
{
  struct pci_access *pacc;  
  struct pci_dev *p;
  pciaddr_t ram = 0;
  char buf[128];
  int i, found, v, n;
  Display *dpy;
  char *display = NULL;
  Window root;
  XVisualInfo *info, templ;
  XWindowAttributes wts;
  XPixmapFormatValues *pf;
  XSetWindowAttributes attr;
  Window win;

  mWidth = 0;
  mHeight = 0;
  mDepth = 0;

  pacc = pci_alloc();
  pci_init(pacc);
  
  pci_scan_bus(pacc);
  for (p = pacc->devices; p; p=p->next) {
    pci_fill_info(p, 
        PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_BASES | PCI_FILL_SIZES);
    if (p->device_class == PCI_CLASS_DISPLAY_VGA) {
      pci_lookup_name(pacc, buf, sizeof(buf), 
                      PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, 
                      p->vendor_id, p->device_id);
      mDeviceName.AssignLiteral(buf);
      sprintf(buf, "0x%04X", p->vendor_id);
      mVendorID.AssignLiteral(buf);
      sprintf(buf, "0x%04X", p->device_id);
      mDeviceID.AssignLiteral(buf);
      for (i=0; i<6; i++) {
        pciaddr_t len = (p->known_fields & PCI_FILL_SIZES) ? p->size[i] : 0;
        if (len > ram) ram = len;
      }
      vram = ram / 1024 / 1024;
    }
    else {
      Log("Grafx Bot: No PCI VGA device found");
    }
  }

  pci_cleanup(pacc);

  if (NULL != (display = getenv("DISPLAY"))) {
    if (display[0] != ':') {
      display = strchr(display, ':');
      if (NULL == display) {
        Log("Grafx Bot: unable to find display");
        return NS_OK;      
      }
    }
    if (NULL == (dpy = XOpenDisplay(display))) {
      Log("Grafx Bot: unable to find X display");
      return NS_OK;
    }
    root = DefaultRootWindow(dpy);
    XGetWindowAttributes(dpy, root, &wts);
    mWidth = wts.width;
    mHeight = wts.height;

    templ.screen = XDefaultScreen(dpy);
    info = XGetVisualInfo(dpy, VisualScreenMask, &templ, &found);
    v = -1;
    for (i = 0; v == -1 && i < found; i++) {
      if (info[i].depth >= 15)
        v = i;
    }
    for (i = 0; v == -1 && i < found; i++) {
      if (info[i].depth == 8)
        v = i;
    }
    if (-1 == v) {
      Log("Grafx Bot: can't find visual");
      return NS_OK;
    }

    pf = XListPixmapFormats(dpy, &n);
    for (i = 0; i < n; i++) {
      if (pf[i].depth == info[v].depth) {
        mDepth = pf[i].depth;
      }
    }
    
    if (gGLXWrap.OpenLibrary("libGL.so.1") && gGLXWrap.Init()) {
      attr.background_pixel = 0;
      attr.border_pixel = 0;
      attr.colormap  = XCreateColormap(dpy, root, info[v].visual, AllocNone);
      attr.event_mask = StructureNotifyMask | ExposureMask;
      win = XCreateWindow(dpy, root, 0, 0, 100, 100, 0, info[v].depth, 
        InputOutput, info[v].visual,
        CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &attr);
      GLXContext ctx = gGLXWrap.fCreateContext(dpy, info, NULL, true);
      if (ctx) { 
        if (gGLXWrap.fMakeCurrent(dpy, win, ctx)) {
          mDriverVersion.AssignLiteral((char*)gGLXWrap.fGetString(LOCAL_GL_VERSION));
          mDriver.AssignLiteral((char*)gGLXWrap.fGetString(LOCAL_GL_RENDERER));
          mDriver.AppendLiteral(" (");
          mDriver.AppendLiteral((char*)gGLXWrap.fGetString(LOCAL_GL_VENDOR));
          mDriver.AppendLiteral(")");
        } 
        else {
          Log("Grafx Bot: unable to make current");
        }
        gGLXWrap.fDestroyContext(dpy, ctx);
      }
      else {
        Log("Grafx Bot: unable to create context");
      }
      XDestroyWindow(dpy, win);
    }
    else {
      Log("Grafx Bot: can't init libGL.so.1");
    }
  }

  return NS_OK;
}

/* readonly attribute DOMString DisplayAdapter; */
NS_IMETHODIMP nsSystemInfo::GetDisplayAdapter(nsAString & aDisplayAdapter)
{
  aDisplayAdapter.Assign(mDeviceName);
  return NS_OK;
}

/* readonly attribute DOMString DisplayChipset; */
NS_IMETHODIMP nsSystemInfo::GetDisplayChipset(nsAString & aDisplayChipset)
{
  aDisplayChipset.Assign(mDeviceID);
  return NS_OK;
}

/* readonly attribute DOMString DisplayRAM; */
NS_IMETHODIMP nsSystemInfo::GetDisplayRAM(nsAString & aDisplayRAM)
{
  aDisplayRAM.AppendInt(vram);
  return NS_OK;
}

/* readonly attribute DOMString DisplayDriver; */
NS_IMETHODIMP nsSystemInfo::GetDisplayDriver(nsAString & aDisplayDriver)
{
  aDisplayDriver.Assign(mDriver);
  return NS_OK;
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
  return NS_OK;
}

/* readonly attribute DOMString DisplayWidth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayWidth(nsAString & aDisplayWidth)
{
  aDisplayWidth.AppendInt(mWidth);
  return NS_OK;
}

/* readonly attribute DOMString DisplayHeight; */
NS_IMETHODIMP nsSystemInfo::GetDisplayHeight(nsAString & aDisplayHeight)
{
  aDisplayHeight.AppendInt(mHeight);
  return NS_OK;
}

/* readonly attribute DOMString DisplayColorDepth; */
NS_IMETHODIMP nsSystemInfo::GetDisplayColorDepth(nsAString & aDisplayColorDepth)
{
  aDisplayColorDepth.AppendInt(mDepth);
  return NS_OK;
}

/* readonly attribute DOMString DisplayVendorID; */
NS_IMETHODIMP nsSystemInfo::GetDisplayVendorID(nsAString & aDisplayVendorID)
{
  aDisplayVendorID.Assign(mVendorID);
  return NS_OK;
}
