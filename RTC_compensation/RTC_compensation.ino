//This sketch attempts to calibrate the RTC crystal on a Teensy 3
//using a PPS signal from a GPS receiver

//enables RTC on Teensy 3:
#include <TimeLib.h>  

//determine what Teensy 3 pin the PPS Signal goes to:
const byte PPS = 14; 

void setup() {
  pinMode(PPS,INPUT); //PPS Signal input
  Serial.begin(115200);

  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);
  while (!Serial);  // Wait for Arduino Serial Monitor to open  

  delay(100);
  if (timeStatus()!= timeSet) {
    Serial.println("Unable to sync with the RTC");
  } 
  else {
    Serial.println("RTC has set the system time");
  }
}



void loop() {
    if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  
  //how many seconds is the time interval long over which the
  //MCU and RTC will be measured:
  int time_interval = 10; //seconds
  Teensy3Clock.compensate(-2);

  //now determine how many microseconds the MCU takes to count to x
  unsigned long time_interval_mcu = mcu_time_interval_calc(time_interval);

  //now determine how many microseconds the RTC takes to count to x
  unsigned long time_interval_rtc = rtc_time_interval_calc(time_interval);

  long PPM_delta_MCU= time_interval_mcu-1000000L*(long)time_interval;
  Serial.print("i.e. ");
  Serial.print((float)PPM_delta_MCU/(float)time_interval,2);
  Serial.print("PPM for the MCU and ");
  long PPM_delta_RTC= time_interval_rtc-1000000L*(long)time_interval;
  Serial.print((float)PPM_delta_RTC/(float)time_interval,2);
  Serial.println("PPM for the RTC.");

  //because the combination of both formulas cancel out the interval time, omit
  float RTC_cumulative_PPM = (float)PPM_delta_MCU*(float)1000000/(float)time_interval_rtc;

  Serial.print("Taken together, combined RTC offset appears to be ");
  Serial.print(RTC_cumulative_PPM,2);
  Serial.println("PPM for the RTC.");

  int RTC_compensate =(int)(-RTC_cumulative_PPM/.1192);

  Serial.print("This suggests a ");
  Serial.print(RTC_compensate);
  Serial.println(" rtc_compensate factor...");

}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

  if(Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L; // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}

unsigned long mcu_time_interval_calc(int time_interval)
{
  unsigned long time_begin, time_end, delta;
  while(digitalRead(PPS)==LOW){
  } //wait until it goes high but ignore the first one
  delay(200); //delay 200ms to let PPS signal die  
  while(digitalRead(PPS)==LOW){
  } //wait until it goes high, second time is a charm
  time_begin=micros();  
  for (int i=0;i<time_interval;i++)
  {
    delay(1000); //delay 200ms to let PPS signal die
    while(digitalRead(PPS)==LOW){
    } //wait x rounds
  } 
  time_end=micros();
  delta=time_end-time_begin;

  Serial.print("Over a ");
  Serial.print(time_interval);
  Serial.print(" second interval, the MCU took ");
  Serial.print(delta);
  Serial.println(" 'MCU micro-seconds'.");   
  return (delta); //return the interval that the MCU will run
}

unsigned long rtc_time_interval_calc(int time_interval)
{
  unsigned long time_begin, time_end, delta;
  int current_second=RTC_TSR;
  while(current_second==RTC_TSR){
  } //wait until it changes
  time_begin=micros(); // start clock
  current_second=RTC_TSR;
  for (int i=0;i<time_interval;i++)
  {
    while(current_second==RTC_TSR){
    } //wait x rounds
    current_second=RTC_TSR;//each time increment the seconds
  } 
  time_end=micros();
  delta=time_end-time_begin;

  Serial.print("Over a ");
  Serial.print(time_interval);
  Serial.print(" second interval, the RTC took ");
  Serial.print(delta);
  Serial.println(" 'MCU micro-seconds'.");   
  return (delta); //return the mcu us the RTC took
}
