const CC = Components.classes;
const CI = Components.interfaces;

const testxul = "chrome://halreftest/content/haltest.xul";
const id = "halreftest@mozilla.org";

// Default values for gTestResults
var gDefaults = {
  // Successful...
  Pass: 0,
  LoadOnly: 0,
  // Unexpected...
  Exception: 0,
  FailedLoad: 0,
  UnexpectedFail: 0,
  UnexpectedPass: 0,
  AssertionUnexpected: 0,
  AssertionUnexpectedFixed: 0,
  // Known problems...
  KnownFail : 0,
  AssertionKnown: 0,
  Random : 0,
  Skip: 0,
  // Test control...
  ThisChunkStartTest: 0,
  TestsComplete: false,
  TestsContinue: false,
  ManifestURL: "",
  ChunkSize: 9999,
};
// Current test results
var gTestResults = {};
// Tracks whether a test is currently being run
var gIsTesting;

function copyObject(source, dest) {
  for (var prop in source) {
    dest[prop] = source[prop];
  }
}

/**
 * This funciton handles the "dataevent" notifications from extension
 * chrome code.  When this message is received, parse the "data" attribute
 * of the target element to update the local copy of gTestResults.  If
 * the tests are marked as complete, reset the gTestResults object to its
 * initial state, so that it's ready for another reftest to be executed.
 */
function dataEventListener(event) {
  var elm = event.target;
  var data = elm.getAttribute("data");
  gTestResults = JSON.parse(data);
  if (gTestResults.TestsComplete) {
    for (var prop in gTestResults) {
      if (prop != "TestsComplete" && prop != "TestsContinue")
        gTestResults[prop] = 0;
    }
  }
}
window.addEventListener("dataevent",dataEventListener,false);

var remoteReftestTestDriver = {
  NextTestChunk: function gtd_nextTestChunk() {
    if (gTestResults.TestsComplete || !gTestResults.TestsContinue) {
      gIsTesting = false;
      var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch)
                            .QueryInterface(Components.interfaces.nsIPrefService);
      prefService.setBoolPref("haltest.testInProgress", false);
      var wwatch = CC["@mozilla.org/embedcomp/window-watcher;1"]
                   .getService(Components.interfaces.nsIWindowWatcher);
      wwatch.unregisterNotification(remoteReftestTestDriver.WindowObserver); 
      if (!gTestResults.TestsComplete) {
        var ldata = document.createElement("p");
        ldata.innerHTML = "REFTEST INFO | User aborted tests";
        document.getElementById("results").appendChild(ldata);
      }     
      
      // reset the force_srgb pref to its original value
      var prefs = CC["@mozilla.org/preferences-service;1"]
                  .getService(CI.nsIPrefBranch);
      try {
        prefs.clearUserPref("gfx.color_management.force_srgb");
      }
      catch (e) {}
      document.getElementById("startbutton").removeAttribute("disabled");
    }
    else {
      gTestResults.TestsContinue = false;
      var prefs = CC["@mozilla.org/preferences-service;1"]
                  .getService(Components.interfaces.nsIPrefBranch);
      prefs.setBoolPref("gfx.color_management.force_srgb", true);
      var wwatch = CC["@mozilla.org/embedcomp/window-watcher;1"]
                   .getService(Components.interfaces.nsIWindowWatcher);
      var args = { };
      args.wrappedJSObject = args;
      wwatch.openWindow(null,
         testxul, "_blank", "chrome,dialog=no,all", null);      
    }
  },
  WindowObserver: {
    observe: function gtd_windowObserver(aSubject, aTopic, aData) {
      if (aSubject.location == testxul && aTopic == "domwindowclosed") {
        setTimeout(remoteReftestTestDriver.NextTestChunk, 0);
      }
    }
  },
  runReftest: function gtd_reftst(startTest, chunkSize) {
    // Most of this code is copied from the command line handler code.
    try {
      // Create a URI from the manifest file
      var cmdline = CC["@mozilla.org/toolkit/command-line;1"]
                    .createInstance(Components.interfaces.nsICommandLine);
      var args = { };
      args.wrappedJSObject = args;

      var reftest_manifest = CC["@mozilla.org/extensions/manager;1"]
                             .getService(CI.nsIExtensionManager)
                             .getInstallLocation(id)
                             .getItemFile(id, "tests/layout/reftests/svg/filters/myreftest.list");

      // Add the url to the gTestResults object
      copyObject(gDefaults, gTestResults);
      gTestResults.ManifestURL = cmdline.resolveURI(reftest_manifest.path).spec;
      gTestResults.TestsComplete = false;
      gTestResults.ThisChunkStartTest = startTest;
      gTestResults.ChunkSize = chunkSize;
      var elm = document.getElementById("listen");
      elm.setAttribute("data", JSON.stringify(gTestResults));

      /* Ignore the platform's online/offline status while running 
         reftests. */
      var ios2 = CC["@mozilla.org/network/io-service;1"]
                .getService(CI.nsIIOService2);
      ios2.manageOfflineStatus = false;
      ios2.offline = false;

      /* Force sRGB as an output profile for color management before we load a
         window. */
      var prefs = CC["@mozilla.org/preferences-service;1"]
                  .getService(CI.nsIPrefBranch);
      prefs.setBoolPref("gfx.color_management.force_srgb", true);

      var wwatch = CC["@mozilla.org/embedcomp/window-watcher;1"]
                   .getService(CI.nsIWindowWatcher);
      wwatch.registerNotification(remoteReftestTestDriver.WindowObserver);
      wwatch.openWindow(null,
        testxul, "_blank", "chrome,dialog=no,all,left=0,top=0", null);
    }
    catch (e) {
      // display the exception to the user
      document.getElementById("results").innerHTML = "<p>" + e + "</p>";
      return;
    }

  },
  submitForm: function gtd_submit(evt) {
    dump("---------   " + evt.screenX + "\n");
    // Prevent a new test from starting while one is already running.
    if (gIsTesting)
      return;
    gIsTesting = true;
    document.getElementById("startbutton").setAttribute("disabled", "disabled");
    
    var results = document.getElementById("results");
    results.innerHTML = "<p>REFTEST INFO | Starting tests</p>";
    
    var prefService = CC["@mozilla.org/preferences-service;1"]
                      .getService(CI.nsIPrefBranch)
                      .QueryInterface(CI.nsIPrefService);
    prefService.setBoolPref("haltest.testInProgress", true);
    // Force save to prefs file now, in case of crash
    prefService.savePrefFile(null);

    var cmdline = CC["@mozilla.org/toolkit/command-line;1"]
                  .createInstance(CI.nsICommandLine);
    var jsm = CC["@mozilla.org/extensions/manager;1"]
              .getService(CI.nsIExtensionManager)
              .getInstallLocation(id)
              .getItemFile(id, "modules/halreftest.jsm");
    Components.utils.import(cmdline.resolveURI(jsm.path).spec);
    HalReftestHelper.saveLastCrash();

    var startTest = 0;
    var chunkSize = 9999;

    this.runReftest(startTest, chunkSize);
  }
}

function displaydiv(e) {
  var divs = ["window1", "window2", "window3", "window4"];
  for (var div in divs) {
    var element = document.getElementById(divs[div]);
    element.setAttribute("class", "windowhide");
  }
  var href = e.href.substr(e.href.indexOf("#") + 1);
  element = document.getElementById(href);
  element.setAttribute("class", "window");
  
  var elements = document.getElementsByTagName("a");
  for (element in elements) {
    if (elements[element].className == "liselected") {
      elements[element].setAttribute("class", "liplain");
    }
  }
  e.setAttribute("class", "liselected");
  
  return false;
}