#include <SPI.h>
#include <MySensor.h>
#include <DHT.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
#define VBAT_PER_BITS 0.003363075  // Calculated volts per bit from the used battery montoring voltage divider.   Internal_ref=1.1V, res=10bit=2^10-1=1023, Eg for 3V (2AA): Vin/Vb=R1/(R1+R2)=470e3/(1e6+470e3),  Vlim=Vb/Vin*1.1=3.44V, Volts per bit = Vlim/1023= 0.003363075
#define VMIN 1.9  // Battery monitor lower level. Vmin_radio=1.9V
#define VMAX 3.3  //  " " " high level. Vmin<Vmax<=3.44
unsigned long SLEEP_TIME = 1.5 * 60 * 1000; // Sleep time between reads (in milliseconds)

MySensor gw;
DHT dht;
float lastTemp;
float lastHum;
boolean metric = true;

int BATTERY_SENSE_PIN = A0;
int batteryPcnt = 0;
int batLoop = 0;
int batArray[3];

boolean enable_skip_counter = true;
int temp_skip_counter = 0;
int hum_skip_counter = 0;


MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void setup()
{
  analogReference(INTERNAL);    // use the 1.1 V internal reference for battery level measuring
  delay(500); // Allow time for radio if power used as reset <<<<<<<<<<<<<< Experimented with good result

  gw.begin();
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Temperature & Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
}

void loop()
{
  int sensorValue = analogRead(BATTERY_SENSE_PIN);    // Battery monitoring reading
  delay(1000);

  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  } else if (lastTemp != temperature || temp_skip_counter >= 1) {
    lastTemp = temperature;
    temp_skip_counter = 0;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
#ifdef DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
#endif
  } else {
    temp_skip_counter++;
  }

  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum || hum_skip_counter >= 1) {
    lastHum = humidity;
    hum_skip_counter = 0;
    gw.send(msgHum.set(humidity, 1));
#ifdef DEBUG
    Serial.print("H: ");
    Serial.println(humidity);
#endif
  } else {
    hum_skip_counter++;
  }

  float Vbat  = sensorValue * VBAT_PER_BITS;
  int batteryPcnt = static_cast<int>(((Vbat - VMIN) / (VMAX - VMIN)) * 100.);
#ifdef DEBUG
  Serial.print("Battery percent: "); Serial.print(batteryPcnt); Serial.println(" %");
#endif
  batArray[batLoop] = batteryPcnt;

  if (batLoop > 2) {

    batteryPcnt = (batArray[0] + batArray[1] + batArray[2] + batArray[3]);
    batteryPcnt = batteryPcnt / 4;
#ifdef DEBUG
    Serial.print("Battery percent (Avg (4):) "); Serial.print(batteryPcnt); Serial.println(" %");
#endif

    if (batteryPcnt > 100)
      batteryPcnt = 100;

    gw.sendBatteryLevel(batteryPcnt);
    batLoop = 0;
  }
  else
  {
    batLoop = batLoop + 1;
  }

  gw.sleep(SLEEP_TIME); //sleep a bit
}


