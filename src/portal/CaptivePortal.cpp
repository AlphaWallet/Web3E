#include "CaptivePortal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "ProvisioningWifiStore.h"
#include "PortalBoot.h"
#include "../Web3Log.h"

static DNSServer dnsServer;
static WebServer webServer(80);
static ProvisioningWifiStore portalWifiStore;
static PortalBoot* gPortalBoot = nullptr;

static bool scanInProgress = false;
static unsigned long lastScanTime = 0;
static String cachedScanJson = "{\"scanning\":false,\"networks\":[]}";
static bool doneRequested = false;
static bool credentialsSaved = false;
static String gDeviceAddress;
static uint8_t gPortalReason = 0;

static bool apHasConnectedClients()
{
  return WiFi.softAPgetStationNum() > 0;
}

static void restoreApOnlyMode()
{
  if (WiFi.getMode() == WIFI_AP_STA && WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_AP);
    WEB3E_PRINTLN("[Portal] Restored WIFI_AP mode after scan");
  }
}

static String buildScanJsonFromResults(int n)
{
  String json = "{\"scanning\":false,\"networks\":[";
  if (n <= 0)
  {
    json += "]}";
    return json;
  }

  bool seen[n];
  for (int i = 0; i < n; i++)
    seen[i] = false;

  bool first = true;
  for (int i = 0; i < n; i++)
  {
    if (seen[i])
      continue;
    String ssid = WiFi.SSID(i);
    int bestRSSI = WiFi.RSSI(i);
    int bestIdx = i;
    bool encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    for (int j = i + 1; j < n; j++)
    {
      if (!seen[j] && WiFi.SSID(j) == ssid)
      {
        seen[j] = true;
        if (WiFi.RSSI(j) > bestRSSI)
        {
          bestRSSI = WiFi.RSSI(j);
          bestIdx = j;
        }
      }
    }
    seen[bestIdx] = true;
    if (!first)
      json += ",";
    first = false;
    json += "{\"ssid\":\"";
    json += ssid;
    json += "\",\"rssi\":";
    json += String(bestRSSI);
    json += ",\"encrypted\":";
    json += encrypted ? "true" : "false";
    json += "}";
  }
  json += "]}";
  return json;
}

static void startWifiScan(bool userRequested)
{
  if (scanInProgress)
    return;

  if (!userRequested && apHasConnectedClients())
  {
    WEB3E_PRINTLN("[Portal] Skipping auto Wi-Fi scan: AP client connected");
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.scanNetworks(true);
  scanInProgress = true;
  lastScanTime = millis();
  WEB3E_PRINT("[Portal] Wi-Fi scan started (userRequested=");
  WEB3E_PRINT(userRequested ? "true" : "false");
  WEB3E_PRINTLN(")");
}

static void checkScanProgress()
{
  if (!scanInProgress)
    return;

  int n = WiFi.scanComplete();
  if (n >= 0)
  {
    scanInProgress = false;
    cachedScanJson = buildScanJsonFromResults(n);
    WEB3E_PRINT("[Portal] Wi-Fi scan complete: ");
    WEB3E_PRINT(n);
    WEB3E_PRINTLN(" networks");
    WiFi.scanDelete();
    restoreApOnlyMode();
  }
  else if (n == WIFI_SCAN_FAILED)
  {
    scanInProgress = false;
    WEB3E_PRINTLN("[Portal] Wi-Fi scan failed");
    restoreApOnlyMode();
  }
}

static String scanNetworksJson()
{
  if (scanInProgress)
    return "{\"scanning\":true,\"networks\":[]}";
  return cachedScanJson;
}

static void handleRoot()
{
  String appName = String(APP_NAME);
  String html = F("<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>");
  html += appName;
  html += F(" Setup</title><style>"
    "body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5}"
    ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}"
    "h1{color:#333;margin-top:0;margin-bottom:5px}"
    ".subtitle{color:#666;font-size:14px;margin-top:0;margin-bottom:20px}"
    "h2{color:#666;font-size:18px;margin-top:30px;border-bottom:2px solid #ddd;padding-bottom:5px}"
    "button{background:#007bff;color:white;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;font-size:14px;margin:5px}"
    "button:hover{background:#0056b3}"
    "button.danger{background:#dc3545}"
    "button.success{background:#28a745}"
    "input,select{width:100%;padding:10px;margin:5px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}"
    ".pwd-wrapper{position:relative;display:block;margin-bottom:10px}"
    ".pwd-wrapper input{width:100%;padding-right:45px}"
    ".pwd-toggle{position:absolute;right:5px;top:50%;transform:translateY(-50%);background:#6c757d;color:white;border:none;padding:5px 10px;border-radius:3px;cursor:pointer;font-size:12px}"
    "label{display:block;margin-top:10px;color:#555;font-weight:bold}"
    ".saved-item{padding:10px;margin:5px 0;background:#fff3cd;border-radius:4px;display:flex;justify-content:space-between;align-items:center}"
    ".status{padding:10px;margin:10px 0;border-radius:4px;display:none}"
    ".status.success{background:#d4edda;color:#155724;display:block}"
    ".status.error{background:#f8d7da;color:#721c24;display:block}"
    ".status.info{background:#d1ecf1;color:#0c5460;display:block}"
    ".hidden{display:none}"
    ".device-info{background:#e7f3ff;padding:10px;border-radius:4px;margin-bottom:20px;font-size:12px;color:#004085}"
    ".scanning{color:#6c757d;font-style:italic;font-size:14px}"
    "</style></head><body><div class=\"container\">"
    "<h1>");
  html += appName;
  html += F(" Setup</h1>"
    "<p class=\"subtitle\">Offline Wi-Fi reconfiguration (KeyID preserved)</p>"
    "<div class=\"device-info\"><strong>Device Address:</strong> ");
  html += gDeviceAddress.length() ? gDeviceAddress : String("(unavailable)");
  html += F("</div>");
  
  if (gPortalReason == PORTAL_REASON_LOST_WIFI) {
    html += F("<div class=\"status info\" style=\"display:block\">Portal opened: saved Wi-Fi networks not visible. "
      "If your network reappears, the portal will auto-exit and reconnect.</div>");
  } else {
    html += F("<div class=\"status info\" style=\"display:block\">Setup portal held open "
      "(triple power-cycle). Saved Wi-Fi is kept — review and tap Done to "
      "reconnect and resume normal operation.</div>");
  }
  
  html += F("<div id=\"status\" class=\"status\"></div>"
    "<h2>Select Network</h2>"
    "<label>Available Networks <span id=\"scan-status\" class=\"scanning\">Scanning...</span></label>"
    "<select id=\"network-select\" onchange=\"onNetworkSelected()\">"
    "<option value=\"\">-- Select a network --</option></select>"
    "<button type=\"button\" onclick=\"refreshNetworks()\" style=\"margin-top:8px\">Refresh Networks</button>"
    "<div id=\"network-pwd-section\" class=\"hidden\">"
    "<label>Password</label>"
    "<div class=\"pwd-wrapper\"><input type=\"password\" id=\"network-password\" placeholder=\"Enter password\">"
    "<button class=\"pwd-toggle\" type=\"button\" onclick=\"togglePassword('network-password')\">show</button></div>"
    "<button type=\"button\" onclick=\"addSelectedNetwork()\">Add Network</button></div>"
    "<h2>Add Hidden Network</h2>"
    "<label>SSID</label><input type=\"text\" id=\"hidden-ssid\" placeholder=\"Network name\">"
    "<label>Password</label>"
    "<div class=\"pwd-wrapper\"><input type=\"password\" id=\"hidden-password\" placeholder=\"Leave empty for open\">"
    "<button class=\"pwd-toggle\" type=\"button\" onclick=\"togglePassword('hidden-password')\">show</button></div>"
    "<button type=\"button\" onclick=\"addHiddenNetwork()\">Add to List</button>"
    "<h2>Saved Networks</h2><div id=\"saved-networks\" class=\"network-list\"></div>"
    "<button id=\"done-btn\" class=\"success\" type=\"button\" onclick=\"commitAndReboot()\" "
    "style=\"width:100%;margin-top:20px;font-size:16px\">Done - Save &amp; Reboot</button>"
    "</div><script>"
    "let networksBySsid={};let selectFocused=false;let scanPollInterval=null;"
    "function showStatus(msg,type){const s=document.getElementById('status');s.textContent=msg;"
    "s.className='status '+(type||'success');setTimeout(()=>{s.className='status'},5000);}"
    "function togglePassword(id){const i=document.getElementById(id);i.type=i.type==='password'?'text':'password';}"
    "document.getElementById('network-select').addEventListener('focus',()=>selectFocused=true);"
    "document.getElementById('network-select').addEventListener('blur',()=>selectFocused=false);"
    "function applyNetworkList(data){const status=document.getElementById('scan-status');"
    "const sel=document.getElementById('network-select');"
    "if(data.scanning){status.textContent='Scanning...';return;}status.textContent='';"
    "if(!data.networks){return;}const cur=sel.value;networksBySsid={};"
    "sel.innerHTML='<option value=\"\">-- Select a network --</option>';"
    "data.networks.forEach(net=>{networksBySsid[net.ssid]=net;"
    "const signal=net.rssi>-50?'****':(net.rssi>-70?'***':'**');"
    "const opt=document.createElement('option');opt.value=net.ssid;"
    "opt.textContent=net.ssid+' '+signal+(net.encrypted?' [locked]':'');"
    "opt.dataset.encrypted=net.encrypted;sel.appendChild(opt);});"
    "if(cur&&networksBySsid[cur])sel.value=cur;"
    "if(data.networks.length===0){status.textContent='No networks — tap Refresh';}}"
    "function pollNetworks(){if(selectFocused)return;fetch('/scan').then(r=>r.json()).then(data=>{"
    "applyNetworkList(data);"
    "if(!data.scanning&&scanPollInterval){clearInterval(scanPollInterval);scanPollInterval=null;}"
    "}).catch(()=>{});}"
    "function refreshNetworks(){document.getElementById('scan-status').textContent='Scanning...';"
    "fetch('/refresh',{method:'POST'}).then(r=>r.json()).then(()=>{"
    "if(!scanPollInterval){scanPollInterval=setInterval(pollNetworks,1000);}pollNetworks();"
    "}).catch(()=>showStatus('Refresh failed','error'));}"
    "function onNetworkSelected(){const sel=document.getElementById('network-select');"
    "const sec=document.getElementById('network-pwd-section');"
    "if(!sel.value){sec.classList.add('hidden');return;}sec.classList.remove('hidden');"
    "const net=networksBySsid[sel.value];document.getElementById('network-password').value='';"
    "document.getElementById('network-password').placeholder=(net&&String(net.encrypted)==='false')?'(open network)':'Enter password';}"
    "function addSelectedNetwork(){const ssid=document.getElementById('network-select').value;"
    "const password=document.getElementById('network-password').value;if(!ssid)return;"
    "fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
    "body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(password)})"
    ".then(r=>r.text()).then(t=>{showStatus(t,'success');loadSavedNetworks();})"
    ".catch(()=>showStatus('Save failed','error'));}"
    "function addHiddenNetwork(){const ssid=document.getElementById('hidden-ssid').value;"
    "const password=document.getElementById('hidden-password').value;if(!ssid)return;"
    "fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
    "body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(password)})"
    ".then(r=>r.text()).then(t=>{showStatus(t,'success');document.getElementById('hidden-ssid').value='';"
    "document.getElementById('hidden-password').value='';loadSavedNetworks();})"
    ".catch(()=>showStatus('Save failed','error'));}"
    "function loadSavedNetworks(){fetch('/list').then(r=>r.json()).then(data=>{"
    "const div=document.getElementById('saved-networks');"
    "if(!data.length){div.innerHTML='<p>No networks saved yet.</p>';return;}"
    "div.innerHTML='';data.forEach((net,idx)=>{const item=document.createElement('div');item.className='saved-item';"
    "item.innerHTML='<span>'+net.ssid+'</span><button class=\"danger\" type=\"button\" onclick=\"removeNetwork('+idx+')\">Remove</button>';"
    "div.appendChild(item);});}).catch(()=>{});}"
    "function removeNetwork(idx){fetch('/remove',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
    "body:'index='+idx}).then(()=>loadSavedNetworks());}"
    "function commitAndReboot(){const btn=document.getElementById('done-btn');btn.disabled=true;"
    "btn.textContent='Saving...';fetch('/done',{method:'POST'}).then(r=>r.text()).then(t=>{"
    "showStatus(t,'info');btn.textContent='Rebooting...';}).catch(()=>{"
    "showStatus('Failed','error');btn.disabled=false;btn.textContent='Done - Save & Reboot';});}"
    "pollNetworks();scanPollInterval=setInterval(pollNetworks,1000);"
    "setTimeout(()=>{if(scanPollInterval){clearInterval(scanPollInterval);scanPollInterval=null;}},20000);"
    "loadSavedNetworks();"
    "</script></body></html>");

  webServer.send(200, "text/html", html);
}

static void handleScan()
{
  webServer.send(200, "application/json", scanNetworksJson());
}

static void handleRefresh()
{
  if (scanInProgress)
  {
    webServer.send(200, "application/json", "{\"ok\":true,\"scanning\":true}");
    return;
  }
  startWifiScan(true);
  webServer.send(200, "application/json",
                 scanInProgress ? "{\"ok\":true,\"scanning\":true}"
                                : "{\"ok\":false,\"scanning\":false,\"reason\":\"scan_not_started\"}");
}

static void handleSave()
{
  if (!webServer.hasArg("ssid"))
  {
    webServer.send(400, "text/plain", "Missing SSID");
    return;
  }
  String ssid = webServer.arg("ssid");
  String password = webServer.hasArg("password") ? webServer.arg("password") : "";
  if (portalWifiStore.appendNetwork(ssid.c_str(), password.c_str()))
    webServer.send(200, "text/plain", "Saved " + ssid);
  else
    webServer.send(500, "text/plain", "Failed to add network");
}

static void handleList()
{
  WifiCredential networks[MAX_WIFI_NETWORKS];
  int count = portalWifiStore.readNetworks(networks, MAX_WIFI_NETWORKS);
  String json = "[";
  for (int i = 0; i < count; i++)
  {
    if (i)
      json += ",";
    json += "{\"ssid\":\"";
    json += networks[i].ssid;
    json += "\",\"pwLen\":";
    json += String(networks[i].pwLen);
    json += "}";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

static void handleRemove()
{
  if (!webServer.hasArg("index"))
  {
    webServer.send(400, "text/plain", "Missing index");
    return;
  }
  int index = webServer.arg("index").toInt();
  if (portalWifiStore.removeNetwork(index))
    webServer.send(200, "text/plain", "Network removed");
  else
    webServer.send(500, "text/plain", "Failed to remove network");
}

static void handleDone()
{
  int count = portalWifiStore.getNetworkCount();
  if (count == 0)
  {
    webServer.send(400, "text/plain", "No networks saved. Add at least one network first.");
    return;
  }

  if (gPortalBoot && gPortalBoot->isInitialized())
  {
    gPortalBoot->clearStayInPortal();
    WEB3E_PRINTLN("[Portal] Cleared stay_portal flag (user confirmed Done)");
  }

  webServer.send(200, "text/plain", "Saved. Rebooting into normal operation...");
  credentialsSaved = true;
  doneRequested = true;
}

static void handleNotFound()
{
  webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  webServer.send(302, "text/plain", "");
}

static void setupAccessPoint()
{
  WiFi.mode(WIFI_AP);
  String apSSID = String(AP_SSID_PREFIX) + "-";
  uint32_t mac = (uint32_t)ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", (unsigned)(mac & 0xFFFF));
  apSSID += suffix;

  String apPassword = String(AP_PASSWORD);
  if (apPassword.length() > 0)
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  else
    WiFi.softAP(apSSID.c_str());

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  dnsServer.start(53, "*", apIP);

  WEB3E_PRINT("[Portal] AP started: ");
  WEB3E_PRINT(apSSID.c_str());
  WEB3E_PRINT("  IP=");
  WEB3E_PRINTLN(WiFi.softAPIP().toString().c_str());
}

bool captivePortalRun(PortalBoot* portalBoot, const char* deviceAddress)
{
  WEB3E_PRINTLN("[Portal] Starting offline captive portal (no firmware OTA)");
  gPortalBoot = portalBoot;
  gDeviceAddress = deviceAddress ? deviceAddress : "";
  gPortalReason = portalBoot ? portalBoot->getPortalReason() : 0;
  doneRequested = false;
  credentialsSaved = false;
  scanInProgress = false;
  cachedScanJson = "{\"scanning\":false,\"networks\":[]}";

  if (!portalWifiStore.begin())
  {
    WEB3E_PRINTLN("[Portal] ProvisioningWifiStore begin failed");
    return false;
  }

  setupAccessPoint();

  webServer.on("/", handleRoot);
  webServer.on("/scan", handleScan);
  webServer.on("/refresh", HTTP_POST, handleRefresh);
  webServer.on("/save", HTTP_POST, handleSave);
  webServer.on("/list", handleList);
  webServer.on("/remove", HTTP_POST, handleRemove);
  webServer.on("/done", HTTP_POST, handleDone);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

  startWifiScan(false);

  while (!doneRequested)
  {
    dnsServer.processNextRequest();
    webServer.handleClient();
    checkScanProgress();
    if (gPortalBoot)
      gPortalBoot->clearBootCountIfStable(millis());
    delay(10);
  }

  webServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  WEB3E_PRINT("[Portal] Done — credentialsSaved=");
  WEB3E_PRINTLN(credentialsSaved ? "1" : "0");
  return credentialsSaved;
}
