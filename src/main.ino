// 150122 10am
#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif

#define CORE_DEBUG_LEVEL 5
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "main.h"

void setup(){
    delay(50);

    M5.begin(); //true, false, true, true, true);
    M5.SHT30.Begin();

    // init M5paper
    M5.EPD.Clear(true);
    M5.RTC.begin();

    // // init screen
    canvas.createCanvas(W_SCREEN, H_SCREEN);
    canvas.setTextSize(5);
    // drawBackground();
    canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
    canvas.deleteCanvas();

    Serial.println("setup wifi... ");
    setupWifi();
    Serial.println("wifi flag: " + String(flg_wifi));
    if (flg_wifi) {
        // init mqtt
        client.setServer(mqtt_server, 1883);  //Sets the server details
        print_info("^o^/w", X_SLPSTRING,Y_BOTSTRING,3);
    } else {
        print_info("o_o!nw", X_SLPSTRING,Y_BOTSTRING,3);
    }

    int server_result = 0;
    load_sd_config();
    server_result = get_server_config();
    Serial.println("http server result: "+ String(server_result));

    //no need for BT yet
    // btStop();
}

void load_sd_config(){
    if (SD.begin()) {  // Initialize the SD card. 初始化SD卡

        Serial.printf("TF card initialized.");
        if (SD.exists("/server_conf.txt")) {  // Check file exist
            Serial.printf("server_conf.txt exists.");
            File myFile = SD.open("/server_conf.txt",
                FILE_READ);  // Open the file in read mode
            if (myFile) {
                Serial.println("/server_conf.txt Content:");
                // Read the data from the file and print it until the reading is
                String sd_load = "";
                while (myFile.available()) {
                    sd_load += (char) myFile.read();
                    // Serial.print(sd_load);
                    // Serial.println(" ");
                }
                myFile.close();

                flg_server_config = sd_load.charAt(0);
                load_config_str(sd_load);
            } else {
                Serial.println("error opening /server_conf.txt");
            }
        } else {
            Serial.printf("server_conf.txt doesn't exist.");
        }
    }
}

int load_config_str(const String &s) {
    int index_payload = 1;
    int flg_sepa = 0;
    int index_btn_conf = 0;
    // clean old settings
    for(int i=0;i<NUM_BUTTONS; i++){
        bt_conf_pic[i] = "";
        bt_conf_func[i] = "";
        bt_conf_topic[i] = "";
        bt_conf_cmd[i] = "";
    }
    // load string to settings
    while (index_payload < s.length()) {
        if (s.charAt(index_payload) == ';') {
            // a cmd finished
            flg_sepa = 0;
            index_btn_conf++;

        } else if (s.charAt(index_payload) == ':') {
            // a separate
            flg_sepa++;

        } else {
            // a normal char
            switch (flg_sepa)
            {
            case 0:
                bt_conf_pic[index_btn_conf] += s.charAt(index_payload);
                break;
            case 1:
                bt_conf_func[index_btn_conf] += s.charAt(index_payload);
                break;
            case 2:
                bt_conf_topic[index_btn_conf] += s.charAt(index_payload);
                break;
            case 3:
                bt_conf_cmd[index_btn_conf] += s.charAt(index_payload);
                break;

            default:
                break;
            }
        }
        index_payload++;
    }


    Serial.print("string analysis finish: ");
    Serial.println(" ");
    for(int i=0;i<4; i++){
        Serial.print("index: " + String(i));
        Serial.println(" ");
        Serial.print("pic: " + bt_conf_pic[i]);
        Serial.println(" ");
        Serial.print("topic: " + bt_conf_topic[i]);
        Serial.println(" ");
        Serial.print("cmd: " + bt_conf_cmd[i]);
        Serial.println(" ");
    }

    return 0;
}

int get_server_config(){
    /*
    check and load server config
    return:
    1 - loaded successful
    2 - no loading, use local setting
    */
    int result = 2;
    if ((wifiMulti.run() == WL_CONNECTED)) {  // wait for WiFi connection
        Serial.printf("[HTTP] begin...\n");
        http.begin(
            "http://ophp.h/m5cmd.html");  // configure traged server url
        // Serial.printf("[HTTP] GET...\n");
        int httpCode = http.GET();  // start connection and send HTTP header
        if (httpCode > 0) {  // httpCode will be negative on error.
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            if (httpCode ==
                HTTP_CODE_OK) {  // file found at server.
                String payload = http.getString();
                Serial.print("payload: ");
                Serial.print(payload);
                Serial.println(" ");
                if (payload.charAt(0) == flg_server_config) {
                    //same config from server and SD card, can use SD settings
                    Serial.println("same flag from SD and server, no need to override");
                    result = 2;
                } else {
                    //new config from server - load settings and write to SD
                    // load string to config as function, can load http or sd file

                    Serial.println("loading config from server: ");
                    load_config_str(payload);
                    // write config to sd card
                    if (SD.begin()) {  // Initialize the SD card. 初始化SD卡

                        Serial.printf("TF card initialized.");
                        File myFile = SD.open("/server_conf.txt",
                                            FILE_WRITE);  // Create a new file
                        if (myFile) {  // If the file is open, then write to it.
                            Serial.printf("Writing to test.txt...");
                            myFile.print(payload);
                            myFile.close();  // Close the file. 关闭文件
                            Serial.println("done.");
                        } else {
                            Serial.println("error opening test.txt");
                        }
                        delay(50);
                    }
                }
            } else {
                // http return code error from server
                result = 2;
            }
        } else {
            // httpcode negative
            Serial.printf("[HTTP] GET... failed, error: %s\n",
                          http.errorToString(httpCode).c_str());
            // http failed
            result = 2;
        }
        http.end();
    } else {
        Serial.printf("connect failed");
        // no wifi connection
        result = 2;
    }
    return result;
}

void loop(){

    // Serial.println("start loop");
    if((int_rwake != WAKE_TIMER) && (wifiMulti.run() == WL_CONNECTED)) {
        /* display the wifi info*/
        // Serial.println("normall start boot and wifi connected, normal check in every loop");
        if(1) { //debug information
            // Serial.printf("Temperatura: %2.2f*C  Humedad: %0.2f%%\r\n", tem, hum);
        }
        // check mqtt connection
        if(!client.connected()) {
            flg_mqtt = 0;
            reConnectMQTT();
        } else {
            checkMQTT();
        }
        // publish ip ends to mqtt
        if((flg_print == 0) && (flg_mqtt == 1)) {

            Serial.println("if need to publish the ip to MQTT");
            // sprintf(th,"Temperatura: %2.2f*C  Humedad: %0.2f%%\r\n", tem, hum);
            // print_info(String(th),X_HTTSTRING, Y_HTSTRING - 4*H_LETTER);
            print_info("ip ends: ", X_BTYSTRING, Y_BOTSTRING - 1*H_LETTER,3);
            print_info(String(WiFi.localIP()[3]), X_BTYSTRING + 8*W_LETTER, Y_BOTSTRING - 1*H_LETTER,3);
            flg_print = 1;
            str_msg = "wifi-" + String(WiFi.localIP()[3]);
            client.publish("cqw/m5", str_msg.c_str());
            // test pub a msg
        }
        /**/
        if((flg_mqtt == 1) && (decount++ >= (TIME_MQTT_HEAT_DELAY_MS))) {
            Serial.println("if need to publish a heatbeat message");
            str_msg = "heat-" + String(mcount);
            mcount += 1;
            client.publish("cqw/m5", str_msg.c_str());
            decount = 0;
        }

    } else if(int_rwake == WAKE_TIMER){
        //wakeup from timer

    } else {
        // wifi not connected

    }

    // check touchscreen
    if(M5.TP.available()){
        // Serial.println("check touch screen");
        if(!M5.TP.isFingerUp()){
            M5.TP.update();

            // Serial.println("not finger up and update touchscreen");
            bool is_update = false;
            for(int i=0;i<2; i++){
                tp_finger_t FingerItem = M5.TP.readFinger(i);
                if((t_point[i][0]!=FingerItem.x)||(t_point[i][1]!=FingerItem.y)){
                    Serial.println("finger updated");
                    is_update = true;
                    t_point[i][0] = FingerItem.x;
                    t_point[i][1] = FingerItem.y;

                    String str_print = "ID:";
                    str_print += String(FingerItem.id) + ",X: " + String(FingerItem.x) + ",Y: " + String(FingerItem.y) + ",S: " + String(FingerItem.size);
                    unsigned int str_y = Y_TOUCHINFO + FingerItem.id * H_LETTER;

                    print_info(str_print, X_BTYSTRING, str_y,3);
                }
            }
            if(is_update){
                /**/
                // Serial.println("touch is updated");
                // don't sleep if touch the screen
                wake_count = 0;
                if(check_click(&ButtTH)){
                        // htcount = TIMER_MAX_HT + 1;
                        // bacount = TIMER_MAX_BATTERY +1;
                    Serial.println("TH butt clicked");
                }else {
                    bool B1click = check_click(&Butt1);
                    bool B2click = check_click(&Butt2);
                    bool B3click = check_click(&Butt3);
                    bool B4click = check_click(&Butt4);
                    if (B1click || B2click || B3click || B4click){
                        // TODO wake code might not working
                        // can check wifi status and reconnect to wifi
                        if(int_rwake == WAKE_TIMER) {
                            // touch screen triggered while wake-up from timer
                            // re init wifi and mqtt
                            int_rwake = WAKE_MANUAL;
                            setupWifi();
                            // init mqtt
                            client.setServer(mqtt_server, 1883);  //Sets the server details
                            client.setCallback(callback); //Sets the message callback function
                            if(!client.connected()) {
                                flg_mqtt = 0;
                                reConnectMQTT();
                            }
                        }
                        checkMQTT();
                        if (flg_mqtt) {
                            // TODO link buttom click to mqtt command loading by file?

                            String str_topic = bt_conf_topic[0];
                            String str_payload = bt_conf_cmd[0];
                            if(B1click){
                                Serial.println("B1 butt clicked");
                            } else if(B2click) {
                                str_topic = bt_conf_topic[1];
                                str_payload = bt_conf_cmd[1];
                                Serial.println("B2 butt clicked");
                            } else if(B3click){
                                str_topic = bt_conf_topic[2];
                                str_payload = bt_conf_cmd[2];
                                Serial.println("B3 butt clicked");
                            } else if(B4click) {
                                str_topic = bt_conf_topic[3];
                                str_payload = bt_conf_cmd[3];
                                Serial.println("B4 butt clicked");
                            }

                            const int topic_length = str_topic.length();
                            const int payload_length = str_payload.length();
                            char c_topic[topic_length];
                            char c_payload[payload_length];
                            strcpy(c_topic, str_topic.c_str());
                            strcpy(c_payload, str_payload.c_str());
                            client.publish(c_topic, c_payload);
                        }
                    }
                }
            }
        }
    } else {
        // if not touched
        // Serial.println("tp not avaliable!");
    }

    // update battery statue
    /*
    if((bacount > TIMER_MAX_BATTERY) || (int_rwake == WAKE_TIMER)) {
        Serial.println("update battery");
        bacount = 0;
        uint32_t t_b = 0;
        t_b = M5.getBatteryVoltage();
        // Serial.println("get battery voltage" + String(t_b));
        if(battery_level != t_b){
            Serial.println("new battery voltage" + String(t_b));
            battery_level = t_b;
            print_info("Battery: " + String(battery_level) + " mV", X_SLPSTRING, Y_BTYSTRING);
            if (int_rwake != WAKE_TIMER) {
                Serial.println("check mqtt for battery");
                checkMQTT();
                if(flg_mqtt == 1) {
                    Serial.println("flg mqtt == 1");
                    float vbattery = (float) battery_level / 1000;
                    char cvbattery[5];
                    dtostrf(vbattery, 4,2,cvbattery);
                    client.publish("m5p/battery", cvbattery);
                }
            }
        }
    }
    */

    // update temperature and humidity
    if((htcount > TIMER_MAX_HT)|| (int_rwake == WAKE_TIMER)) {
        Serial.println("update temperature and humidity");
        htcount = 0;
        M5.SHT30.UpdateData();
        tem = M5.SHT30.GetTemperature();
        hum = M5.SHT30.GetRelHumidity();
        Serial.printf("Temperature: %2.2f*C  Humedad: %0.2f%%\r\n", tem, hum);
        dtostrf(tem, 2, 1 , temStr);
        dtostrf(hum, 2, 1 , humStr);

        print_info(" " + String(temStr) + "C", X_HTTSTRING, Y_HTSTRING,8);
        print_info(" " + String(humStr) + "%", X_HTHSTRING, Y_HTSTRING,8);
        Serial.println("check mqtt for Temperature");
        if(int_rwake != WAKE_TIMER) {
            checkMQTT();
            if(flg_mqtt == 1) {
                client.publish("m5p/temperature", temStr);
                client.publish("m5p/humidity", humStr);
            }
        }
    }

    // ------update display------
    // 1. date time every minute
    // 2. heating mode
    // 3. heating working statue
    // 4. heating temperature set if changed by server
    // 5. hot warter working statue
    // 6. weather?
    // 7. switch LED when sunset

    if(int_rwake == WAKE_TIMER) {
        delay(5);
        // start_sleep();
    }
    // IO trigger wake up will check for TIME_TO_SLEEP long before back to sleep
    delay(TIME_LOOP_DELAY_MS);
    // bacount +=1;
    htcount +=1;
    // wake_count +=1;
    wificount +=1;

    // Serial.println("loop end...\n\n");
    /*
    if(wake_count % 100 == 0){
        Serial.printf("wake_count: %d\n", wake_count/100);
        if(wake_count >= TIME_TO_SLEEP - 100){
            print_info("Sleep zZZ.", X_BTYSTRING,Y_BTYSTRING);
        }
        if(wake_count >= TIME_TO_SLEEP){
            start_sleep();
        }
    }
    */

}

// GPIO interrupt check if it is a long press?
// 1. check which GPIO for from config file
// 2. local GPIO response light / heating / PIR sensor
// 3. send MQTT message, projector control / curve light control / heater control
//

// sub functions:

void start_sleep() {
    // ----- disable sleep mode -----
    // delay(TIME_LOOP_DELAY_MS);
    // Serial.println("Going to sleep now");
    // esp_deep_sleep_start();
}
// keep MQTT connected
void reConnectMQTT() {
    int mqtt_count = 0;
    while ((!client.connected()) && (mqtt_count++ < 10)){
        String clientId = "M5S-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect.
        if (client.connect(clientId.c_str()/*, "36241673", "8g0vC26vg1kqPWoR"*/)) {
            client.publish("m5p", "hello world");
            // mqtt connected
            flg_mqtt = 1;
        } else {
            client.print("failed, rc=");
            client.print(client.state());
            client.println("try again in 5 seconds");
            delay(5000);
        }
    }
    if (!client.connected()){
        //mqtt connection failed, internet down?
        flg_mqtt = 0;
    }
}

// callback for MQTT message arrived
void callback(char* topic, byte* payload, unsigned int length) {
  String msg_print = "Message arrived [";
  msg_print += String(topic);
  msg_print += "] ";

  for (int i = 0; i < length; i++) {
    msg_print += String((char)payload[i]);
  }
  print_info(msg_print, X_BTYSTRING, Y_HTSTRING - 3*H_LETTER,3);
}

// connect to home wifi
void setupWifi() {
  delay(10);
  WiFi.mode(WIFI_STA);  //Set the mode to WiFi station mode
  wifiMulti.addAP("xxHome", "KillAll4FreeLoader");
  int wl_count = 0;
  while ((wifiMulti.run() != WL_CONNECTED) && (wl_count++ < 6)) {
    delay(500);
  }
  int wifi_result = wifiMulti.run();
  if (wifi_result != WL_CONNECTED) {
      // wifi connection failed
      Serial.println("wifi result: " + String(wifi_result));
      flg_wifi = 0;
  } else {
      flg_wifi = 1;
  }
  flg_mqtt = 0;
}
void checkMQTT() {
    // Serial.println("flg mqtt: " + String(flg_mqtt));
    if(flg_mqtt != 1) {
        if(wificount > TIMER_MAX_WIFI) {
            wificount = 0;
            setupWifi();
            reConnectMQTT();
        }
    }
    if(flg_mqtt == 1) {
        // Serial.print("wifi and MQTT reconnected");
        if (flg_his_mqtt != 1) {
            print_info("MQTT good", X_BTYSTRING,Y_BTYSTRING,3);
            flg_his_mqtt = 1;
        }
    } else {
        // no internet
        if (flg_his_mqtt != 0) {
            print_info("Inet down", X_BTYSTRING,Y_BTYSTRING,3);
            flg_his_mqtt = 0;
        }
    }
    // flg_mqtt = 0;
}
// void drawBackground() {
//     canvas.drawJpg(humi, 6604, X_HTHSTRING, Y_HTICON, 125, 125);
//     // canvas.drawJpg(lamp, 8632, X_HTTSTRING, Y_BTR1ICON, 125, 125);
//     canvas.drawPngFile(SD, "/bed_icon_left_s.png", X_HTTSTRING, Y_BTR1ICON);
//     canvas.drawPngFile(SD, "/bed_icon_right_s.png", X_HTHSTRING, Y_BTR1ICON);
//     canvas.drawPngFile(SD, "/room_s.png", X_HTTSTRING, Y_BTR2ICON);
//     canvas.drawPngFile(SD, "/fan_s.png", X_HTHSTRING, Y_BTR2ICON);
//     // canvas.drawJpg(projector, 9718, X_HTHSTRING, Y_BTR1ICON, 125, 125);
//     canvas.drawJpg(temperature, 9005, X_HTTSTRING, Y_HTICON, 125, 125);
// }

bool check_click(const struct t_area* ar_touched) {
    bool flg_click = false;
    if ((t_point[0][0]>ar_touched->x1) && (t_point[0][0]<ar_touched->x2)) {
        if ((t_point[0][1]>ar_touched->y1) && (t_point[0][1]<ar_touched->y2)) {
            flg_click = true;
        }
    }
    return flg_click;
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
int print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :
        {
            Serial.println("Wakeup caused by external signal using RTC_IO");
            //mark the wake up reason as manually triggered -> connect to wifi, check MQTT, send command, keep live for a while
            return 1;
        }break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER :
        {
            Serial.println("Wakeup caused by timer");
            //mark the wake up reason as time out, only need to update enviroment information
            return 2;
        }break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : {
        Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);
    }break;
  }
  return 0;
}

// void wakeup_setup(){
//     esp_sleep_enable_timer_wakeup(TIME_TO_WAKE * uS_TO_S_FACTOR);
//     gpio_pullup_dis(down_buttom_gpio_num);
//     gpio_pulldown_en(down_buttom_gpio_num);
//     esp_sleep_enable_ext0_wakeup(down_buttom_gpio_num,LOW);
//     gpio_hold_en(GPIO_NUM_2); //M5EPD_MAIN_PWR_PIN
//     // gpio_deep_sleep_hold_en();
// }

void print_info(String msg, unsigned int x, unsigned int y, unsigned int tsize) {
    Serial.printf("print %s to eink\n", msg);
    canvas.clear();
    canvas.deleteCanvas();
    canvas.createCanvas(W_SCREEN, H_LETTER);
    canvas.setTextSize(tsize);
    canvas.drawString(msg, 0, 0);
    canvas.pushCanvas(x,y,UPDATE_MODE_A2);
}