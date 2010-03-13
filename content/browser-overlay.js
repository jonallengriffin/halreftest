function onHaltestMenuCommand(event) {
  var newtab;
  if (gBrowser) {
    newtab = gBrowser.addTab("chrome://halreftest/content/start-testing.html");
    gBrowser.selectedTab = newtab;
  } else {
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    var mainWindow = wm.getMostRecentWindow("navigator:browser");
    newtab = mainWindow.getBrowser().addTab("chrome://halreftest/content/start-testing.html");
    mainWindow.getBrowser().selectedTab = newtab;
  }
}
