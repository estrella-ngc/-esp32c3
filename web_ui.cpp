#include "web_ui.h"
#include "exit.h"
#include "adc.h"
#include "tmp.h"
#include "led.h"
#include "relay.h"
#include "wifi_manager.h"
#include <WebServer.h>
#include <WiFi.h>

static WebServer server(80);

static const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>PCT-100-003</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,Helvetica,Arial,sans-serif;background:#0d1117;color:#c9d1d9;padding:16px;min-height:100vh}
.container{max-width:420px;margin:0 auto}
h1{font-size:20px;font-weight:600;color:#f0f6fc;margin-bottom:16px;display:flex;align-items:center;justify-content:space-between}
.cat{opacity:.6;flex-shrink:0}
.tabs{display:flex;gap:4px;margin-bottom:16px}
.tab{flex:1;padding:8px;text-align:center;border:none;border-radius:6px;font-size:13px;font-weight:500;cursor:pointer;background:#21262d;color:#8b949e}
.tab.active{background:#238636;color:#fff}
.tab-page{display:none}
.tab-page.show{display:block}
.card{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:16px;margin-bottom:12px}
.row{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px}
.row:last-child{margin-bottom:0}
.label{font-size:14px;color:#8b949e}
.value{font-size:14px;font-weight:500}
.badge{display:inline-block;padding:2px 10px;border-radius:10px;font-size:12px;font-weight:600}
.badge-on{background:#238636;color:#fff}
.badge-off{background:#21262d;color:#8b949e;border:1px solid #30363d}
.btn{display:block;width:100%;padding:10px;border:none;border-radius:6px;font-size:14px;font-weight:500;cursor:pointer;text-align:center}
.btn:active{opacity:.7}
.btn-primary{background:#238636;color:#fff}
.btn-primary:hover{background:#2ea043}
.btn-danger{background:#da3633;color:#fff}
.btn-danger:hover{background:#f85149}
.btn-secondary{background:#21262d;color:#c9d1d9;border:1px solid #30363d}
.btn-secondary:hover{background:#30363d}
.btn-group{display:flex;gap:8px;margin-top:8px}
.btn-group .btn{flex:1}
.fn-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}
.fn-btn{padding:12px;border:2px solid #30363d;border-radius:8px;background:#0d1117;color:#c9d1d9;font-size:16px;font-weight:600;cursor:pointer;text-align:center}
.fn-btn.active{border-color:#238636;background:#23863620;color:#238636}
.fn-btn:active{opacity:.7}
.sensor-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.sensor-item{text-align:center;padding:12px;background:#0d1117;border-radius:6px}
.sensor-item .val{font-size:24px;font-weight:700;color:#f0f6fc}
.sensor-item .lbl{font-size:12px;color:#8b949e;margin-top:4px}
.net-info{font-size:13px;line-height:1.8}
.net-info span{color:#8b949e}
.scan-list{margin-top:8px;max-height:240px;overflow-y:auto}
.scan-item{display:flex;justify-content:space-between;align-items:center;padding:8px;border-bottom:1px solid #21262d;cursor:pointer}
.scan-item:hover{background:#21262d}
.scan-item .ssid{font-size:14px;color:#f0f6fc}
.scan-item .rssi{font-size:12px;color:#8b949e}
.scan-item.selected{background:#23863620;border-color:#238636}
.inp{width:100%;padding:8px 10px;border:1px solid #30363d;border-radius:6px;background:#0d1117;color:#f0f6fc;font-size:14px;margin-top:8px}
.inp:focus{border-color:#238636;outline:none}
.help-panel{display:none;background:#0d1117;border:1px solid #30363d;border-radius:6px;padding:12px;margin-top:8px;font-size:12px;line-height:1.8;color:#8b949e}
.help-panel.show{display:block}
.help-panel b{color:#f0f6fc}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#238636;color:#fff;padding:8px 16px;border-radius:6px;font-size:13px;z-index:99;display:none}
.toast.err{background:#da3633}
</style>
</head>
<body>
<div class="container">
<h1>PCT-100-003 <svg class="cat" viewBox="0 0 40 40" width="24" height="24"><path d="M8 28 C8 28,6 18,12 12 C14 10,18 8,20 10 L22 8 L24 12 C28 10,32 12,34 16 C38 22,34 28,32 30" fill="none" stroke="#f0f6fc" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/><path d="M22 16 C22 16,24 14,26 14" fill="none" stroke="#f0f6fc" stroke-width="1.5" stroke-linecap="round"/><ellipse cx="16" cy="18" rx="1.2" ry="1.2" fill="#f0f6fc"/><ellipse cx="26" cy="18" rx="1.2" ry="1.2" fill="#f0f6fc"/><path d="M20 22 C20 22,22 26,24 22" fill="none" stroke="#f0f6fc" stroke-width="1.2" stroke-linecap="round"/></svg></h1>
<div class="tabs">
<button class="tab active" data-tab="ctrl" onclick="switchTab('ctrl')">控制</button>
<button class="tab" data-tab="wifi" onclick="switchTab('wifi')">WiFi</button>
</div>

<div id="page_ctrl" class="tab-page show">
<div class="card" style="padding:8px 16px">
  <input class="inp" id="d_search" placeholder="输入 help 查看说明" style="margin-top:0" onkeydown="if(event.key==='Enter')handleSearch()">
  <div class="help-panel" id="d_help">
    <b>总闸</b> &mdash; 开启/关闭整个系统，同时自动重连 estrella WiFi<br>
    <b>模式</b> &mdash; 自动：光感自动控制灯；手动：按钮循环切换<br>
    <b>光照/温度</b> &mdash; 实时传感器数据<br>
    <b>键盘</b> &mdash; K1=K2=GND时开机<br>
    <b>灯/风扇</b> &mdash; 手动模式下可单独控制<br>
    <b>WiFi</b> &mdash; 自动连接 estrella，失败后自动开放热点 PCT-100-003<br>
    <b>热点</b> &mdash; 密码 12345678，访问 192.168.4.1<br>
    <b>串口</b> &mdash; scan / connect / ip / ap / ap_stop / forget / help
  </div>
</div>
<div class="card">
  <div class="row">
    <span class="label">⚡ 总闸</span>
    <span class="badge" id="d_system">OFF</span>
  </div>
  <button class="btn btn-primary" id="btn_system" onclick="post('/api/system',refresh)">切换</button>
</div>

<div class="card">
  <div class="row">
    <span class="label">📋 模式</span>
    <span class="badge" id="d_mode">自动</span>
  </div>
  <button class="btn btn-secondary" onclick="post('/api/mode',refresh)">切换模式</button>
  <div class="fn-grid" id="fn_grid" style="display:none">
    <button class="fn-btn" data-fn="0" onclick="post('/api/function?mode=0',refresh)">全关</button>
    <button class="fn-btn" data-fn="1" onclick="post('/api/function?mode=1',refresh)">灯亮</button>
    <button class="fn-btn" data-fn="2" onclick="post('/api/function?mode=2',refresh)">风扇</button>
    <button class="fn-btn" data-fn="3" onclick="post('/api/function?mode=3',refresh)">全开</button>
  </div>
</div>

<div class="card">
  <div class="sensor-grid">
    <div class="sensor-item"><div class="val" id="d_light">--</div><div class="lbl">💡 光照</div></div>
    <div class="sensor-item"><div class="val" id="d_temp">--</div><div class="lbl">🌡 温度 (°C)</div></div>
  </div>
</div>

<div class="card">
  <div class="row"><span class="label">💡 灯光</span><span class="badge" id="d_led">OFF</span></div>
  <div class="btn-group">
    <button class="btn btn-primary" onclick="post('/api/led?state=1',refresh)">开灯</button>
    <button class="btn btn-danger" onclick="post('/api/led?state=0',refresh)">关灯</button>
  </div>
</div>

<div class="card">
  <div class="row"><span class="label">🔄 风扇</span><span class="badge" id="d_fan">OFF</span></div>
  <div class="btn-group">
    <button class="btn btn-primary" onclick="post('/api/fan?state=1',refresh)">开风扇</button>
    <button class="btn btn-danger" onclick="post('/api/fan?state=0',refresh)">关风扇</button>
  </div>
</div>
</div>

<div id="page_wifi" class="tab-page">
<div class="card">
  <div class="row"><span class="label">📶 WiFi 状态</span><span class="badge" id="d_wifi_status">已连接</span></div>
  <div class="net-info" id="d_net_info"></div>
  <div class="btn-group" style="margin-top:8px">
    <button class="btn btn-primary" onclick="post('/api/ap_start',function(){updateNetInfo();toast('热点已开启')})">开启热点</button>
    <button class="btn btn-danger" onclick="post('/api/ap_stop',function(){updateNetInfo();toast('热点已关闭')})">关闭热点</button>
  </div>
</div>

<div class="card">
  <div class="row">
    <span class="label">🔍 扫描网络</span>
    <button class="btn btn-primary" style="width:auto;padding:6px 16px" onclick="scanNetworks()">扫描</button>
  </div>
  <div id="d_scanning" style="display:none;color:#8b949e;font-size:13px;text-align:center;padding:8px">扫描中...</div>
  <div class="scan-list" id="d_scan_list"></div>
</div>

<div class="card" id="d_connect_card" style="display:none">
  <div class="row"><span class="label">🔗 连接网络</span></div>
  <div style="font-size:13px;color:#8b949e" id="d_selected_ssid"></div>
  <input class="inp" type="password" id="inp_pass" placeholder="输入WiFi密码">
  <div class="btn-group" style="margin-top:8px">
    <button class="btn btn-primary" onclick="connectWiFi()">连接</button>
    <button class="btn btn-secondary" onclick="cancelConnect()">取消</button>
  </div>
</div>

<div class="card">
  <button class="btn btn-danger" onclick="forgetWiFi()">忘记当前网络</button>
  <div style="font-size:12px;color:#8b949e;text-align:center;margin-top:8px">清除保存的凭据，下次重启不自动连接</div>
</div>
</div>

</div>
<div class="toast" id="d_toast"></div>

<script>
function $(id){return document.getElementById(id)}
function api(path,cb){fetch(path).then(r=>r.json()).then(cb).catch(()=>toast('请求失败',1))}
function post(path,cb){fetch(path,{method:'POST'}).then(r=>r.json()).then(cb).catch(()=>toast('请求失败',1))}

function toast(msg,err){const t=$('d_toast');t.textContent=msg;t.className='toast'+(err?' err':'');t.style.display='block';setTimeout(()=>t.style.display='none',3000)}

function switchTab(name){
  document.querySelectorAll('.tab').forEach(t=>t.classList.toggle('active',t.dataset.tab===name));
  document.querySelectorAll('.tab-page').forEach(p=>p.classList.toggle('show',p.id==='page_'+name));
}

function handleSearch(){
  var v=$('d_search').value.trim().toLowerCase();
  if(v==='help'){$('d_help').classList.toggle('show')}
}
function refresh(){api('/api/status',updateUI)}
function updateUI(d){
  $('d_system').textContent=d.system?'ON':'OFF';
  $('d_system').className='badge '+(d.system?'badge-on':'badge-off');
  $('btn_system').className='btn '+(d.system?'btn-danger':'btn-primary');
  $('btn_system').textContent=d.system?'关闭总闸':'开启总闸';
  $('d_mode').textContent=d.auto?'自动':'手动';
  $('d_mode').className='badge '+(d.auto?'badge-on':'badge-off');
  const fg=$('fn_grid');fg.style.display=d.auto?'none':'grid';
  document.querySelectorAll('.fn-btn').forEach(b=>b.classList.toggle('active',parseInt(b.dataset.fn)===d.fn));
  $('d_light').textContent=d.light;
  $('d_temp').textContent=d.temp.toFixed(1);
  $('d_led').textContent=d.led?'ON':'OFF';
  $('d_led').className='badge '+(d.led?'badge-on':'badge-off');
  $('d_fan').textContent=d.fan?'ON':'OFF';
  $('d_fan').className='badge '+(d.fan?'badge-on':'badge-off');
}

function updateNetInfo(){
  api('/api/network',function(d){
    $('d_wifi_status').textContent=d.connected?'已连接':'未连接';
    $('d_wifi_status').className='badge '+(d.connected?'badge-on':'badge-off');
    if(d.connected){
      if(d.ap){
        $('d_net_info').innerHTML=
          '<span>模式:</span> 热点<br>'+
          '<span>SSID:</span> '+d.ssid+'<br>'+
          '<span>密码:</span> '+d.pass+'<br>'+
          '<span>IPv4:</span> '+d.ip;
      }else{
        $('d_net_info').innerHTML=
          '<span>SSID:</span> '+d.ssid+'<br>'+
          '<span>IPv4:</span> '+d.ip+'<br>'+
          '<span>子网掩码:</span> '+d.mask+'<br>'+
          '<span>网关:</span> '+d.gateway;
      }
    }else{
      $('d_net_info').innerHTML='<span>未连接任何网络</span>';
    }
  })
}

function scanNetworks(){
  $('d_scanning').style.display='block';
  $('d_scan_list').innerHTML='';
  $('d_connect_card').style.display='none';
  api('/api/scan',function(list){
    $('d_scanning').style.display='none';
    if(!list.length){$('d_scan_list').innerHTML='<div style="text-align:center;padding:12px;color:#8b949e;font-size:13px">未发现网络</div>';return}
    let h='';
    list.forEach(function(n,i){
      h+='<div class="scan-item" onclick="selectNetwork(\''+n.ssid.replace(/'/g,"\\'")+'\','+i+')">'+
        '<span class="ssid">'+(n.ssid||'(隐藏)')+'</span>'+
        '<span class="rssi">'+(n.open?'开放':'加密')+' '+n.rssi+'dBm</span></div>';
    });
    $('d_scan_list').innerHTML=h;
  })
}

var selectedSSID='';
function selectNetwork(ssid,idx){
  selectedSSID=ssid;
  document.querySelectorAll('.scan-item').forEach((e,i)=>e.classList.toggle('selected',i===idx));
  $('d_selected_ssid').textContent='选择: '+ssid;
  $('inp_pass').value='';
  $('d_connect_card').style.display='block';
}

function connectWiFi(){
  if(!selectedSSID){toast('请先选择一个网络',1);return}
  var pass=$('inp_pass').value;
  toast('正在连接 '+selectedSSID+'...');
  post('/api/wifi_connect?ssid='+encodeURIComponent(selectedSSID)+'&pass='+encodeURIComponent(pass),function(d){
    if(d.ok){toast('连接成功!');updateNetInfo();$('d_connect_card').style.display='none'}
    else toast('连接失败',1);
  })
}

function cancelConnect(){
  selectedSSID='';
  $('d_connect_card').style.display='none';
  document.querySelectorAll('.scan-item').forEach(e=>e.classList.remove('selected'));
}

function forgetWiFi(){
  if(!confirm('确定忘记当前网络?'))return;
  post('/api/wifi_forget',function(d){
    if(d.ok){toast('已清除,下次重启不自动连接');updateNetInfo()}
  })
}

setInterval(refresh, 2000);
setInterval(updateNetInfo, 5000);
refresh();
updateNetInfo();
</script>
</body>
</html>
)rawliteral";

static void handle_root(void)
{
  wifi_ap_mark_used();
  server.send_P(200, "text/html", html_page);
}

static void handle_status(void)
{
  wifi_ap_mark_used();
  String json = "{";
  json += "\"system\":" + String(system_enabled) + ",";
  json += "\"auto\":" + String(auto_mode) + ",";
  json += "\"fn\":" + String(function_mode) + ",";
  json += "\"light\":" + String(get_adc_value()) + ",";
  json += "\"temp\":" + String(get_temperature()) + ",";
  json += "\"led\":" + String(digitalRead(LED_PIN)) + ",";
  json += "\"fan\":" + String(digitalRead(RELAY_PIN)) + "";
  json += "}";
  server.send(200, "application/json", json);
}

static void handle_system(void)
{
  system_enabled = !system_enabled;
  if (!system_enabled) { LED(LOW); relay_control(LOW); }
  handle_status();
}

static void handle_mode(void)
{
  auto_mode = !auto_mode;
  handle_status();
}

static void handle_function(void)
{
  if (server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    if (m >= 0 && m <= 3) function_mode = m;
  }
  handle_status();
}

static void handle_led(void)
{
  if (server.hasArg("state")) {
    LED(server.arg("state").toInt() ? HIGH : LOW);
  }
  handle_status();
}

static void handle_fan(void)
{
  if (server.hasArg("state")) {
    relay_control(server.arg("state").toInt() ? HIGH : LOW);
  }
  handle_status();
}

static void handle_network(void)
{
  bool ap = (WiFi.getMode() == WIFI_MODE_AP);
  String json = "{";
  json += "\"connected\":" + String(wifi_is_connected() ? "true" : "false") + ",";
  json += "\"ap\":" + String(ap ? "true" : "false") + ",";
  if (ap) {
    json += "\"ssid\":\"PCT-100-003\",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"pass\":\"12345678\"";
  } else {
    json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"mask\":\"" + WiFi.subnetMask().toString() + "\",";
    json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\"";
  }
  json += "}";
  server.send(200, "application/json", json);
}

static void handle_scan(void)
{
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"open\":" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

static void handle_wifi_connect(void)
{
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    wifi_connect(server.arg("ssid").c_str(), server.arg("pass").c_str());
    String json = "{\"ok\":" + String(wifi_is_connected() ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"ok\":false}");
  }
}

static void handle_wifi_forget(void)
{
  wifi_forget();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_ap_start(void)
{
  wifi_start_ap();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_ap_stop(void)
{
  wifi_stop_ap();
  server.send(200, "application/json", "{\"ok\":true}");
}

void web_init(void)
{
  server.on("/", handle_root);
  server.on("/api/status", handle_status);
  server.on("/api/system", HTTP_POST, handle_system);
  server.on("/api/mode", HTTP_POST, handle_mode);
  server.on("/api/function", HTTP_POST, handle_function);
  server.on("/api/led", HTTP_POST, handle_led);
  server.on("/api/fan", HTTP_POST, handle_fan);
  server.on("/api/network", handle_network);
  server.on("/api/scan", handle_scan);
  server.on("/api/wifi_connect", HTTP_POST, handle_wifi_connect);
  server.on("/api/wifi_forget", HTTP_POST, handle_wifi_forget);
  server.on("/api/ap_start", HTTP_POST, handle_ap_start);
  server.on("/api/ap_stop", HTTP_POST, handle_ap_stop);
  server.begin();
  Serial.println("[Web] HTTP server started on port 80");
}

void web_update(void)
{
  server.handleClient();
}
