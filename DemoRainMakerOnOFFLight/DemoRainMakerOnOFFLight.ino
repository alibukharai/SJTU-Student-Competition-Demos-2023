//This example demonstrates the ESP RainMaker with a standard Bulb device.
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"

#define DEFAULT_POWER_MODE true
const char *service_name = "PROV_1234";
const char *pop = "abcd1234";

//GPIO for push button to reset the wifi if needed
#if CONFIG_IDF_TARGET_ESP32C3
static int gpio_0 = 9;
static int gpio_bulb = LED_BUILTIN;

#else
//GPIO for virtual device
static int gpio_0 = 0;
static int gpio_bulb = 16;
#endif


/* Variable for reading pin status*/
bool bulb_state = true;

//The framework provides some standard device types like switch, lightbulb, fan, temperaturesensor.
static LightBulb my_bulb;

void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32S2
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
      printQR(service_name, pop, "softap");
#else
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
      printQR(service_name, pop, "ble");
#endif
      break;
    case ARDUINO_EVENT_PROV_INIT:
      wifi_prov_mgr_disable_auto_stop(10000);
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      wifi_prov_mgr_stop_provisioning();
      break;
    default:;
  }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();

  if (strcmp(param_name, "Power") == 0) {
    Serial.printf("Received value = %s for %s - %s\n", val.val.b ? "true" : "false", device_name, param_name);
    bulb_state = val.val.b;
    if (bulb_state == false) {
      digitalWrite(gpio_bulb, LOW);
    } else {
      digitalWrite(gpio_bulb, HIGH);
    }
    param->updateAndReport(val);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(gpio_0, INPUT | PULLUP);

  Node my_node;
  my_node = RMaker.initNode("ESP RainMaker Node");

  //Initialize switch device
  my_bulb = LightBulb("LightBulb", &gpio_bulb);

  //Standard switch device
  my_bulb.addCb(write_callback);

  //Add switch device to the node
  my_node.addDevice(my_bulb);

  //This is optional
  RMaker.enableOTA(OTA_USING_TOPICS);
  //If you want to enable scheduling, set time zone for your region using setTimeZone().
  //The list of available values are provided here https://rainmaker.espressif.com/docs/time-service.html
  // RMaker.setTimeZone("Asia/Shanghai");
  // Alternatively, enable the Timezone service and let the phone apps set the appropriate timezone
  RMaker.enableTZService();

  RMaker.enableSchedule();

  RMaker.enableScenes();

  RMaker.start();

  WiFi.onEvent(sysProvEvent);
#if CONFIG_IDF_TARGET_ESP32S2
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#else
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#endif
}

void loop() {
  if (digitalRead(gpio_0) == LOW) {  //Push button pressed for reset wifi or factory reset
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_0) == LOW) {
      Serial.print("Button is Pressed\n");
      delay(50);
    }
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      // If key pressed for more than 10secs, reset all
      Serial.printf("Reset to factory.\n");
      RMakerFactoryReset(2);
    } else if ((endTime - startTime) > 3000) {
      Serial.printf("Reset Wi-Fi.\n");
      // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
      RMakerWiFiReset(2);
    } else {
      Serial.print("First Connect to the Wifi\n");
      delay(10);
    }
  } else {
    delay(100);
  }
}
