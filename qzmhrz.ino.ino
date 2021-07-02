#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <FS.h>     //库文件
#include "Ticker.h" //引入调度器头文件

const char *AP_NAME = "DDNS"; //wifi名字
//暂时存储wifi账号密码
char sta_ssid[32] = {0};
char sta_password[64] = {0};
//配网页面代码
//String page_html =
//   "<!DOCTYPE html><html><head><title>配网页面</title><meta charset=\"UTF-8\"><meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><style>*{box-sizing:border-box}body{font-family:Arial;margin:0}.header{padding:80px;text-align:center;background:#1abc9c;color:white}.button{background-color:#3CA0D3;border:2px solid#FFFFFF;height:50px;width:200px;justify-content:center;align-items:center;font-size:23px;border-radius:8px;color:#FFFFFF}.header h1{font-size:40px}@media screen and(max-width:700px){.row{flex-direction:column}}@media screen and(max-width:400px){.navbar a{float:none;width:100%}}</style></head><body><div class=\"header\"><h1>欢迎使用小崔科技配网模块</h1><p>关注微信公众号“小崔搞科技”获取更多有趣信息</p><form name=\"my\"action=\"/\"method=\"POST\">WiFi名称：<input type=\"text\"name=\"ssid\"placeholder=\"请输入您WiFi的名称\"id=\"aa\"><br><p>WiFi密码：<input type=\"text\"name=\"password\"placeholder=\"请输入您WiFi的密码\"id=\"bb\"><br><p><input class=\"button\"type=\"submit\"value=\"保存\"></form></div></body></html>";
String page_html =
    "<!DOCTYPE html><html><head><title>配网页面</title><meta charset=\"UTF-8\"><meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><style>*{box-sizing:border-box}body{font-family:Arial;margin:0}.header{padding:80px;text-align:center;background:#1abc9c;color:white}label{display:inline-block;width:80px;text-align:justify;text-align-last:justify;margin-right:10px}.button{background-color:#3CA0D3;border:2px solid#FFFFFF;height:50px;width:200px;justify-content:center;align-items:center;font-size:23px;border-radius:8px;color:#FFFFFF}.header h1{font-size:40px}@media screen and(max-width:700px){.row{flex-direction:column}}@media screen and(max-width:400px){.navbar a{float:none;width:100%}}</style></head><body><div class=\"header\"><h1>欢迎使用小崔科技配网模块</h1><p>关注微信公众号“小崔搞科技”获取更多有趣信息</p><form name=\"my\"action=\"/\"method=\"POST\"><label>WiFi名称：</label><input type=\"text\"name=\"ssid\"placeholder=\"请输入您WiFi的名称\"id=\"aa\"><br><p><label>WiFi密码：</label><input type=\"text\"name=\"password\"placeholder=\"请输入您WiFi的密码\"id=\"bb\"><br><p><label>ID：</label><input type=\"text\"name=\"id\"placeholder=\"请输入您ID\"id=\"bb\"><br><p><label>Token：</label><input type=\"text\"name=\"token\"placeholder=\"请输入您Token\"id=\"bb\"><br><p><label>域名：</label><input type=\"text\"name=\"domain\"placeholder=\"请输入您域名\"id=\"bb\"><br><p><label>记录：</label><input type=\"text\"name=\"subdomain\"placeholder=\"请输入您记录\"id=\"bb\"><br><p><input class=\"button\"type=\"submit\"value=\"保存\"></form></div></body></html>";
String page_ok =
    "<!DOCTYPE html><html><head><title>配网成功</title><meta charset=\"UTF-8\"><meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><style>*{box-sizing:border-box}body{font-family:Arial;margin:0}.header{padding:80px;text-align:center;background:#1abc9c;color:white}.image{width:200px;height:230px}.button{background-color:#3CA0D3;border:2px solid#FFFFFF;height:50px;width:200px;justify-content:center;align-items:center;font-size:23px;border-radius:8px;color:#FFFFFF}.header h1{font-size:40px}@media screen and(max-width:700px){.row{flex-direction:column}}@media screen and(max-width:400px){.navbar a{float:none;width:100%}}</style></head><body><div class=\"header\"><h1>配网成功</h1><p>请等待网页自动关闭......</p></div></body></html>";
String Error_html = "";
const byte DNS_PORT = 53;       //DNS端口号
IPAddress apIP(192, 168, 4, 1); //esp8266-AP-IP地址
DNSServer dnsServer;            //创建dnsServer实例
ESP8266WebServer server(80);    //创建WebServer

//*******获取公网ip参数*************
const char *host = "ip.dhcp.cn"; //需要访问的域名
const int port = 80;             // 需要访问的端口
const String url = "/?ip";       // 需要访问的地址

//**********DDNS更新参数***********
const String ddnshost = "139.159.215.25"; //需要访问的域名
const int ddnsport = 3000;                 // 需要访问的端口
const String ddnsurl = "/dnspod/updnns";   // 需要访问的地址

//*************非常量**************
String id = "";         //ID
String token = "";      //token
String domain = "";     //域名
String sub_domain = ""; //记录
String netIP = "";      //公网IP
String data = "";

String file_name = "/maker/conf.txt"; //被读取的文件位置和名称

//延迟时间
unsigned long lastTime = 0;
unsigned long timerDelay = 1200000;//20分钟

Ticker myTicker; //建立一个需要定时调度的对象
WiFiClient client;

void connectNewWifi(void)
{
  if(SPIFFS.begin()){ // 启动SPIFFS
    Serial.println("SPIFFS Started.");
  } else {
    Serial.println("SPIFFS Failed to Start.");
  }
  WiFi.mode(WIFI_STA);       //切换为STA模式
  WiFi.setAutoConnect(true); //设置自动连接
  WiFi.begin();              //连接上一次连接成功的wifi
  Serial.println("");
  Serial.print("Connect to wifi");
  int count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {

    delay(500);
    count++;
    if (count > 10)
    { //如果5秒内没有连上，就开启Web配网 可适当调整这个时间
      initSoftAP();
      initWebServer();
      initDNS();
      Serial.println("SPIFFS format start");
      SPIFFS.format(); // 格式化SPIFFS
      Serial.println("SPIFFS format finish");
      break; //跳出 防止无限初始化
    }
    Serial.print(".");
  }
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
  { //如果连接上 就输出IP信息 防止未连接上break后会误输出
    if (SPIFFS.begin())
    { // 启动SPIFFS
      Serial.println("SPIFFS Started.");
    }
    else
    {
      Serial.println("SPIFFS Failed to Start.");
    }
      //确认闪存中是否有file_name文件
     if (SPIFFS.exists(file_name)){
       Serial.print(file_name);
       Serial.println("有文件");
       data=ReadData();
       Serial.print("文件=>:");
       Serial.println(data);
     } else {
       Serial.print(file_name);
       Serial.print(" 没有文件");
     }
    Serial.println("WIFI已连接！");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP()); //打印esp8266的IP地址
    server.stop();
  }
}

void handleRoot()
{ //访问主页回调函数
  Serial.println("Connect html");
  server.send(200, "text/html", page_html);
}

void handleRootPost()
{ //Post回调函数
  Serial.println("handleRootPost");
  if (server.hasArg("ssid"))
  { //判断是否有账号参数
    Serial.print("got ssid:");
    strcpy(sta_ssid, server.arg("ssid").c_str()); //将账号参数拷贝到sta_ssid中
    Serial.println(sta_ssid);
  }
  else
  { //没有参数
    Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid"); //返回错误页面
    return;
  }
  //密码与账号同理
  if (server.hasArg("password"))
  {
    Serial.print("got password:");
    strcpy(sta_password, server.arg("password").c_str());
    Serial.println(sta_password);
  }
  else
  {
    Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }

  id = server.arg("id");
  token = server.arg("token");
  domain = server.arg("domain");
  sub_domain = server.arg("subdomain");
  data = "login_token=" + id + "," + token + "&form=json&domain=" + domain + "&sub_domain=" + sub_domain + "&record_line=默认&value=";
  WriteData(data);
  WiFi.begin(sta_ssid, sta_password);

  server.send(200, "text/html", page_ok); //返回保存成功页面
  delay(2000);
  //连接wifi
  connectNewWifi();
}

void initBasic(void)
{ //初始化基础
  Serial.begin(115200);
  WiFi.hostname("Smart-ESP8266"); //设置ESP8266设备名
}

void initSoftAP(void)
{ //初始化AP模式
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (WiFi.softAP(AP_NAME))
  {
    Serial.println("ESP8266 SoftAP is right");
  }
}

void initWebServer(void)
{ //初始化WebServer
  //server.on("/",handleRoot);
  //上面那行必须以下面这种格式去写否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);      //设置主页回调函数
  server.onNotFound(handleRoot);             //设置无法响应的http请求的回调函数
  server.on("/", HTTP_POST, handleRootPost); //设置Post请求回调函数
  server.begin();                            //启动WebServer
  Serial.println("Web服务器已启动！");
}

void initDNS(void)
{ //初始化DNS服务器
  if (dnsServer.start(DNS_PORT, "*", apIP))
  { //判断将所有地址映射到esp8266的ip上是否成功
    Serial.println("启动dnsserver成功");
  }
  else
    Serial.println("启动dnsserver失败");
}

void setup()
{
  initBasic();
  connectNewWifi();
}

void loop()
{
  server.handleClient();
  dnsServer.processNextRequest();
  if ((millis() - lastTime) > timerDelay)
  {
    String ip = GetInternetIP();
    if (ip != netIP)
    {
      netIP = ip;
      PostDDNS(data,ip); //更新DDNS信息
    }
    lastTime = millis();
  }
}
void PostDDNS(String ddnsdata,String IP)
{
//  ddnsdata.replace("\n","");
  IP.replace("\n","");
  Serial.println("连接 GO 服务器");
  Serial.print("DDNSData=>");
  Serial.println(ddnsdata+IP);
  
  if (!client.connect(ddnshost, ddnsport))
  {
    Serial.println("服务器连接失败");
    return;
  }
  client.println("POST " + ddnsurl + " HTTP/1.1");
  client.println("Host: " + ddnshost + "");
  client.println("Accept: */*");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println((ddnsdata+IP).length());
  client.println();
  client.print(ddnsdata+IP);
  
  delay(600);
  //处理返回信息
  String line = client.readStringUntil('\r');
  line = client.readStringUntil('\r');
  line = client.readStringUntil('\r');
  line = client.readStringUntil('\r');
  line = client.readStringUntil('\r');
  line = client.readStringUntil('\r');
  Serial.println(line);
  client.stop();
}
/**************************************************
 * 函数名称：GetInternetIP
 * 函数功能：获取公网IP
 * 参数说明：string[0.0.0.0]
**************************************************/
String GetInternetIP()
{
  Serial.print("请求公网IP :");
  if (!client.connect(host, port))
  {
    Serial.println("服务器连接失败");
    return "";
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + //请求行  请求方法 ＋ 请求地址 + 协议版本
               "Content-Type:  application/json;charset=utf-8\r\n" +
               "Host: " + host + "\r\n" + //请求头部
               "Connection: close\r\n" +  //处理完成后断开连接
               "\r\n");                   //空行
  delay(100);
  String line = client.readStringUntil('\r'); //读一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  line = client.readStringUntil('\r');        //读下一行
  client.stop(); //断开与服务器连接以节约资源
  return line;
}
/*
 * 往ROM中写入数据
 */
 void WriteData(String value){
  Serial.print("写入内存");
  Serial.println(value);
  Serial.println("SPIFFS format start");
  SPIFFS.format();                            // 格式化SPIFFS
  Serial.println("SPIFFS format finish");
  File dataFile = SPIFFS.open(file_name, "w");// 建立File对象用于向   SPIFFS中的file对象（即/notes.txt）写入信息
  dataFile.println(value);                     // 向dataFile写入字符串信息
  dataFile.close();                           // 完成文件写入后关闭文件
  Serial.println("Finished Writing data ");

 }
 /*
  * 从ROM中读取数据
  */
String ReadData(){
  String value="";
  //读取文件内容并且通过串口监视器输出文件信息
  File data = SPIFFS.open(file_name, "r"); 
  for(int i=0;i<data.size();i++)
  {
    value+=(char)data.read();  
  }
  Serial.print("value=>>");
  Serial.print(value);  
  data.close(); 
  return value;     
}

void Removedata(){
   //从闪存中删除file_name文件
  if (SPIFFS.remove(file_name)){
    
    Serial.print(file_name);
    Serial.println(" remove sucess");
    
  } else {
    Serial.print(file_name);
    Serial.println(" remove fail");
  }         
}
