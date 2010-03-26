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

 /* This is a JavaScript module (JSM) to be imported via Components.utils.import() and acts as a singleton.
   Only the following listed symbols will exposed on import, and only when and where imported. */
var EXPORTED_SYMBOLS = ["HalReftestHelper", "HalReftestLogger"];

var prefService = null;
var loaded = false;
const HALREFTEST_EXTENSION_ID = "halreftest@mozilla.org";
const CC = Components.classes;
const CI = Components.interfaces;
const postURL = "http://brasstacks.mozilla.com/halreftest/data/post/";
const reftestImage = "REFTEST   IMAGE: ";
const reftestImage1 = "REFTEST   IMAGE 1 (TEST): "
const reftestImage2 = "REFTEST   IMAGE 2 (REFERENCE): "
const reftestPixels = "REFTEST number of differing pixels: "
const compareURL = "chrome://halreftest/content/compare.html";
const startURL = "chrome://halreftest/content/start-testing.html";
      
var HalReftestLogger =
{
  _converter : null,
  _foStream: null,
  _file: null,
  _liStream: null,
  _file: null,
  _tests: null,
  _testToVerify: -1,
  _prevTestToVerify: -1,
  _timer: null,
  initFile: function() {
    var installRdfFile = CC["@mozilla.org/extensions/manager;1"]
                           .getService(CI.nsIExtensionManager)
                           .getInstallLocation(HALREFTEST_EXTENSION_ID)
                           .getItemFile(HALREFTEST_EXTENSION_ID, "install.rdf");
    var dir = installRdfFile.parent.path;
    // Get an nsIFile in the extension's directory for logging.
    HalReftestLogger._file = CC["@mozilla.org/file/local;1"]
                                .createInstance(CI.nsILocalFile);
    HalReftestLogger._file.initWithPath(dir);
    HalReftestLogger._file.append("results.log");  
  },
  init: function () {
    if (HalReftestLogger._file == null) {
      HalReftestLogger.initFile();
    }
    var exists = HalReftestLogger._file.exists();

    // Make a file output stream and converter to handle it
    HalReftestLogger._foStream = CC["@mozilla.org/network/file-output-stream;1"]
                                    .createInstance(CI.nsIFileOutputStream);
    var fileflags = 0x02 | 0x08 | 0x20;
    HalReftestLogger._foStream.init(HalReftestLogger._file, fileflags, 0666, 0);
    HalReftestLogger._converter = CC["@mozilla.org/intl/converter-output-stream;1"]
                                     .createInstance(CI.nsIConverterOutputStream);
    HalReftestLogger._converter.init(HalReftestLogger._foStream, "UTF-8", 0, 0);
  },
  write: function (data) {
    HalReftestLogger._converter.writeString(data);
  },
  close: function () {
    if (HalReftestLogger._converter != null) {
      HalReftestLogger._converter.close();
      HalReftestLogger._converter = null;
      HalReftestLogger._foStream = null;
    }
    if (HalReftestLogger._liStream != null) {
      HalReftestLogger._liStream.close();
    }
  },
  openForRead: function () {
    if (!HalReftestLogger._file) {
      HalReftestLogger.initFile();
    }
    if (!HalReftestLogger._file.exists()) return false;
    var fiStream = CC["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(CI.nsIFileInputStream);
    fiStream.init(HalReftestLogger._file, -1, -1, false);
    HalReftestLogger._liStream = fiStream.QueryInterface(CI.nsILineInputStream);
    return true;
  },
  readLine: function () {
    var line = { value: null };
    if (!HalReftestLogger._liStream) return false;
    var bytesRead = HalReftestLogger._liStream.readLine(line);
    if (line.value == null) return false;
    if (bytesRead == 0) {
      HalReftestLogger._liStream.close();
      HalReftestLogger._liStream = null;
      return false;
    }
    return line.value;
  },
  WindowObserver: {
    observe: function halreftest_compare_windowObserver(aSubject, aTopic, aData) {
      if (aSubject.location == compareURL && aTopic == "domwindowclosed")
      {
        var wwatch = CC["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(CI.nsIWindowWatcher);
        wwatch.unregisterNotification(HalReftestLogger.WindowObserver);

        var event = { notify: function(timer) { HalReftestLogger.lookForFailures(); } }
        HalReftestLogger._timer = CC["@mozilla.org/timer;1"].createInstance(CI.nsITimer);
        HalReftestLogger._timer.initWithCallback(event, 100, CI.nsITimer.TYPE_ONE_SHOT);
      }
    }
  },
  lookForFailures: function() {
    HalReftestLogger._timer = null;
    HalReftestLogger._prevTestToVerify = HalReftestLogger._testToVerify;
    for (var i = 0; i < HalReftestLogger._tests.length; i++) {
      if (typeof(HalReftestLogger._tests[i].userverified) != "undefined" && 
          HalReftestLogger._tests[i].userverified == -1) {
        HalReftestLogger._testToVerify = i;
        if (HalReftestLogger._prevTestToVerify == HalReftestLogger._testToVerify) {
          // the user cancelled the test or closed the window, so we'll just stop
          return;
        }
        // There's a failure that needs user verification, so open the comparison screen.
        var wwatch = CC["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(CI.nsIWindowWatcher);
        wwatch.registerNotification(HalReftestLogger.WindowObserver);
        wwatch.openWindow(null, compareURL, "_blank", "chrome,dialog=no,all", null);
        return;
      }
    }
  
    // We're all done with user verifications, so post test data to the db server.
    HalReftestLogger.doHttpPost();
  },
  doHttpPost: function() {
    var testdata = {"tests": HalReftestLogger._tests};
    testdata["videocard"] = {};
    var sysinfo = CC["@mozilla.org/extensions/halreftest/service;1"]
                     .getService(CI.nsISystemInfo);
    var da = sysinfo.DisplayAdapter;
    if (da.indexOf(" ") == -1) {
      testdata["videocard"]["brand"] = da;
      testdata["videocard"]["description"] = da;
    }
    else {
      testdata["videocard"]["brand"] = da.substring(0, da.indexOf(" "));
      testdata["videocard"]["description"] = da.substring(da.indexOf(" ") + 1);
    }
    testdata["videocard"]["chipset"] = sysinfo.DisplayChipset;
    testdata["videocard"]["vendorid"] = sysinfo.DisplayVendorID;
    testdata["videocard"]["ram"] = sysinfo.DisplayRAM;
    testdata["videocard"]["drivers"] = sysinfo.DisplayDriver;
    testdata["videocard"]["driverversion"] = sysinfo.DisplayDriverVersion;
    testdata["videocard"]["driverdate"] = sysinfo.DisplayDriverDate;
    testdata["settings"] = {};
    testdata["settings"]["height"] = sysinfo.DisplayHeight;
    testdata["settings"]["width"] = sysinfo.DisplayWidth;
    testdata["settings"]["colordepth"] = sysinfo.DisplayColorDepth;

    var hh = CC["@mozilla.org/network/protocol;1?name=http"]
             .getService(CI.nsIHttpProtocolHandler);
    testdata["os"] = hh["oscpu"];
    var macpos = testdata["os"].indexOf("Mac OS");
    if (macpos > 0) {
      testdata["os"] = testdata["os"].substring(macpos);
    }
    var xulAppInfo = CC["@mozilla.org/xre/app-info;1"]
                     .getService(CI.nsIXULAppInfo);
    testdata["builddate"] = xulAppInfo.appBuildID.substr(0,8);
    testdata["buildver"] = xulAppInfo.version;
    
    var request = CC["@mozilla.org/xmlextras/xmlhttprequest;1"].
                  createInstance(CI.nsIXMLHttpRequest);
    request.open("POST", postURL, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Cache-Control", "no-cache");
    request.QueryInterface(CI.nsIJSXMLHttpRequest);

    var self = HalReftestLogger;
    request.onerror = function(event) { self.onXMLUpdate(event); };
    request.onload  = function(event) { self.onXMLUpdate(event);  };
    request.send(JSON.stringify(testdata));
  },
  postToServer: function(crashed, crashID) {
    var line;
    HalReftestLogger._tests = new Array();
    HalReftestLogger._testToVerify = -1;
    if (!HalReftestLogger.openForRead()) return false;
    while ((line = HalReftestLogger.readLine()) != false) {
      if (line.indexOf("REFTEST START") != -1) {
        var test = {};
        test.name = line.substring(line.indexOf("|") + 2, line.lastIndexOf("|") - 1);
        HalReftestLogger._tests.push(test);
      }
      if ((line.indexOf("REFTEST TEST-PASS") != -1) || 
          (line.indexOf("REFTEST TEST-KNOWN-FAIL") != -1) ||
          (line.indexOf("REFTEST TEST-UNEXPECTED-FAIL") != -1) ||
          (line.indexOf("REFTEST TEST-UNEXPECTED-PASS") != -1)) {
        HalReftestLogger._tests[HalReftestLogger._tests.length - 1].status = 
            line.substring(line.indexOf("REFTEST ") + 8, line.indexOf("|") - 1);
      }
      else if (line.indexOf(reftestImage) != -1) {
        HalReftestLogger._tests[HalReftestLogger._tests.length - 1].testimage = 
            line.substring(line.indexOf(reftestImage) + reftestImage.length);
      }
      else if (line.indexOf(reftestImage1) != -1) {
        HalReftestLogger._tests[HalReftestLogger._tests.length - 1].testimage = 
            line.substring(line.indexOf(reftestImage1) + reftestImage1.length);
      }
      else if (line.indexOf(reftestImage2) != -1) {
        HalReftestLogger._tests[HalReftestLogger._tests.length - 1].refimage = 
            line.substring(line.indexOf(reftestImage2) + reftestImage2.length);
      }
      else if (line.indexOf(reftestPixels) != -1) {
        HalReftestLogger._tests[HalReftestLogger._tests.length - 1].differingpixels = 
            line.substring(line.indexOf(reftestPixels) + reftestPixels.length);
      }
    }
    
    if (typeof(HalReftestLogger._tests[HalReftestLogger._tests.length - 1].status) == "undefined" && crashed) {
      HalReftestLogger._tests[HalReftestLogger._tests.length - 1].status = "TEST-CRASH";
      HalReftestLogger._tests[HalReftestLogger._tests.length - 1].crashid = crashID;
    }
    
    for each (test in HalReftestLogger._tests) {
      // For failed tests that have both a test and reference image, mark the test as
      // needing user verification.
      if (test.status == "TEST-UNEXPECTED-FAIL" && typeof(test.testimage) != "undefined"
          && typeof(test.refimage) != "undefined") {
        test.userverified = -1;
      }
    }
    
    HalReftestLogger.lookForFailures();
  },
  onXMLUpdate: function(evt) {
    var request = evt.target;
    dump("-----------------HTTP POST to db server status " + request.status + "\n");
    if (request.status == 200) {
      dump("-----------------HTTP Response Body:\n" + request.responseText + "\n");
    }
  },
};

var HalReftestHelper =
{
  init : function()
  {
    if (loaded) return;
    loaded = true;
    
    prefService = CC["@mozilla.org/preferences-service;1"]
                     .getService(CI.nsIPrefBranch)
                     .QueryInterface(CI.nsIPrefService);
    var observerService = CC["@mozilla.org/observer-service;1"]
                             .getService(CI.nsIObserverService);
    var quitEventObserver =
    {
        observe : function(subject, topic, data)
        {
            prefService.setBoolPref("haltest.testInProgress", false);  
            prefService.savePrefFile(null);
            observerService.removeObserver(quitEventObserver, "quit-application-granted");
        }
    }
    observerService.addObserver(quitEventObserver, "quit-application-granted", false);
    
    var testInProgress = false;
    try {
      testInProgress = prefService.getBoolPref("haltest.testInProgress");
    }
    catch (e) {}
    if (!testInProgress) return;
    
    try {
      var lastCrashPref = prefService.getIntPref("haltest.lastCrash");
    }
    catch (e) {
      var lastCrashPref = 1;
    }
    var lastCrash = HalReftestHelper.getLastCrash();
    if (lastCrashPref != lastCrash.lastCrashTime) {
      dump("--------------------new crash since test was run, id=" + lastCrash.lastCrashId + "\n");
      HalReftestLogger.postToServer(true, lastCrash.lastCrashId);
    }
    else {
      HalReftestLogger.postToServer(true, 0);
      dump("--------------------looks like we crashed, but no new crash found\n");
    }
    
    prefService.setBoolPref("haltest.testInProgress", false);
    
  },
  getLastCrash : function()
  {
    var lastCrashTime = 1;
    var crashId = 0;
    var lastCrashFile = null;

    var directoryService = CC["@mozilla.org/file/directory_service;1"]
                              .getService(CI.nsIProperties);
    var dir = directoryService.get("UAppData", CI.nsIFile);
    dir.append("Crash Reports");
    dir.append("submitted");
    if (dir.exists() && dir.isDirectory())
    {
      var entries = dir.directoryEntries;
      while (entries.hasMoreElements())
      {
        var file = entries.getNext().QueryInterface(CI.nsIFile);
        if (!lastCrashFile || lastCrashFile.lastModifiedTime < file.lastModifiedTime)
          if (file.leafName.substr(0,3) == "bp-" && file.leafName.substr(-4) == ".txt")
            lastCrashFile = file;
      }
      if (lastCrashFile) {
        lastCrashTime = Math.round(lastCrashFile.lastModifiedTime / 1000);
        crashId = lastCrashFile.leafName.substr(3, lastCrashFile.leafName.length - 7);
      }
    }
    return {"lastCrashTime":lastCrashTime, "lastCrashId":crashId};
  },
  saveLastCrash : function()
  {
    var lastCrash = HalReftestHelper.getLastCrash();
    prefService.setIntPref("haltest.lastCrash", lastCrash.lastCrashTime);
  },
};
