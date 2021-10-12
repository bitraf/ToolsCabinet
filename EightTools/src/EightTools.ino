/*
 * Project EightTools
 * Description: Base firmware for controlling the Particle P1 based PCBs used for the Bitraf Tools system
 * Author: Jensa
 * Date: May 2020
 */
#include "config.h"

void setup() {
  Serial.begin(9600);
  delay(1000); // Give Serial a break, so we get debug output
  Serial.println("hey");
  System.on( all_events, handle_system_events );
  setupHardware();

  // apply unicorn behavior to first lock if it's cabinet 1
  if( strcmp( MQTT_DEVICE_NAME, "workshop-cabinet2-1") == 0 )
  {
    eightLocks[0].setNumLeds( 256 );
  }
  reconnectToWifi();
}

void loop() {
  Particle.process();
  if( stayConnected() )
  {
    for( int i=0; i<DOORS_IN_CABINET;i++)
    {
      eightLocks[i].maintain();
    }
    counter++;
    
    return; // let's just quit here for now

    if( counter%30000==0 ){
      IPAddress addr (85,90,244,199);
      uint32_t  ret =  WiFi.ping( addr );
      Serial.printlnf("Ping returned %d", ret);
    }
  }
}

void handle_system_events(system_event_t event, int param)
{
  if( event == button_click )
  {
    int times = system_button_clicks(param);
    Serial.printlnf("button was clicked %d times", times);
    if( times == 2 && !Particle.connected() )
    {
      Particle.connect();
      Serial.println("Connect to cloud!");
    }
  }
  else if( event == firmware_update )
  {
    setAllLocks( FIRMWARE_PENDING );
  }
   else if( event == firmware_update_complete )
  {
    Serial.println("firmware_update_complete!");
  }
  else if( event == network_status && param == network_status_off )
  {
    Serial.println("network_status_off!");
    isConnected = false;
    reconnectTimer.reset();
    reconnectTimer.start();
  }
  else if( event == network_status && param == network_status_connected )
  {
    isConnected = true;
    reconnectTimer.stop();
    setAllLocks( CONNECTION_RESTORED );
    Serial.println("network_status_connected!");
    time_t time = Time.now();
    Time.format(time, TIME_FORMAT_DEFAULT);
    Serial.println( Time.format(Time.now(), TIME_FORMAT_DEFAULT) );
  }
  else if( event == network_status && param == network_status_disconnecting )
  {
    Serial.println("network_status_disconnecting!");
  }
  else if( event == network_status && param == network_status_powering_off )
  {
    Serial.println("network_status_powering_off!");
  }
  else if( event == network_status && param == network_status_disconnected )
  {
    Serial.println("network_status_disconnected!");
    setAllLocks( LOCK_UNKOWN );
    time_t time = Time.now();
    Time.format(time, TIME_FORMAT_DEFAULT);
    Serial.println( Time.format(Time.now(), TIME_FORMAT_DEFAULT) );
  }
  else if( event == network_status && param == network_status_powering_on )
  {
    Serial.println("network_status_powering_on!");
  }
  else if( event == network_status && param == network_status_on )
  {
    Serial.println("network_status_on!");
  }
  else if( event == network_status && param == network_status_connecting )
  {
    Serial.println("network_status_connecting!");
  }

  else if( event == network_status && param == cloud_status_connected )
  {
    Serial.println("cloud_status_connected!");
  }
  else if( event == network_status && param == cloud_status_disconnected )
  {
    Serial.println("cloud_status_disconnected!");
  }
  else if( event == network_status && param == cloud_status_connecting )
  {
    Serial.println("cloud_status_connecting!");
  }
  else if( event == network_status && param == cloud_status_disconnecting )
  {
    Serial.println("cloud_status_disconnecting!");
  }
  else
  {
    Serial.print("Unknown event: ");
    Serial.print(event);
    Serial.print(" param: ");
    Serial.println(param);
  }
}

bool stayConnected() {
  bool ret = false;
  if( !isConnected )
  {
    // just let the system do it's thing
  }
  else if( !mqttClient.isConnected() )
  {
    Serial.printlnf("MQTT not connected, connecting to %s", MQTT_SERVER);
    setAllLocks( CONNECTION_LOST );
    if( mqttClient.connect( MQTT_DEVICE_NAME ,MQTT_USERNAME,MQTT_PASSWORD) )
    {
      Serial.printlnf("%s is now connected to %s",MQTT_DEVICE_NAME, MQTT_SERVER);
      subscribeToServer();
      setAllLocks( CONNECTION_RESTORED );
    }
    else
    {
      // wait 2 seconds before trying again
      Serial.printlnf("Waiting 1 second to reconnect to %s", MQTT_SERVER);
      setAllLocks( MQTT_CONNECTIING );
      delay(1000);
    }
  }
  else
  {
    mqttClient.loop();
    ret = true;
  }
  
  return ret;
}

void reconnectToWifi()
{
  Serial.println( "reconnectToWifi! ");
  setAllLocks( CONNECTION_LOST );
  WiFi.connect();
}

void subscribeToServer()
{
  int i;
  sprintf( selfSubscribeTopic, "bitraf/tool/%s", MQTT_DEVICE_NAME );
  int rez = mqttClient.subscribe( selfSubscribeTopic );
  Serial.printlnf( "Subscribed to: %s? %d",selfSubscribeTopic, rez);

  for( i = 0; i < DOORS_IN_CABINET; i++)
  {
    sprintf( MQTT_OPEN_TOPIC, "bitraf/tool/%s/unlock", TOOL_TOPICS[i]);
    rez = mqttClient.subscribe( MQTT_OPEN_TOPIC );
    Serial.printlnf("Subscribed to: %s? %d", MQTT_OPEN_TOPIC,rez);

    sprintf( MQTT_OPEN_TOPIC, "bitraf/tool/%s/lock", TOOL_TOPICS[i]);
    rez = mqttClient.subscribe( MQTT_OPEN_TOPIC );
    Serial.printlnf("Subscribed to: %s? %d", MQTT_OPEN_TOPIC,rez);
  }
}
// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Got a call! ");
  Serial.println(topic);

  uint i = 0;
  uint tLen = 0;
  int secondSlash = 0;
  int thirdSlash = 0;
  int slashCount = 0;
  String topicAsString = (char *)topic;
  tLen = topicAsString.length();

  // find the slashes in the topic, so we can parse it
  for(i=0;i<tLen;i++)
  {
    char c = topicAsString[i];
    if( c == '/')
    {
      if( slashCount == 0){
        // foo
      } else if( slashCount == 1){
        secondSlash = i;
      } else if( slashCount == 2){
        thirdSlash = i;
      }
      slashCount++;
    }
  }

  if( strcmp( topic, selfSubscribeTopic) == 0)
  {
    // call to this specific device
    Particle.connect();  // Just connect to Particle cloud
  } else {
    // Locate the tool in our array of tool names/topics
    String toolNameRequested = topicAsString.substring(secondSlash+1,thirdSlash);
    int doorNumber = -1;
    for(i=0;i<DOORS_IN_CABINET;i++)
    {
        String toolToCompare = TOOL_TOPICS[i];
        if( toolToCompare == toolNameRequested )
        {
            doorNumber = i;
        }
    }

    // parse out the command sent
    String commandRequested = topicAsString.substring(thirdSlash+1, topicAsString.length());
    if( commandRequested == "unlock")
    {
      toolState = TOOL_STATE_CHECKOUT;
    }
    else if( commandRequested == "lock")
    {
      toolState = TOOL_STATE_RETURNED;
    }
    else
    {
      Serial.print("Unknown command: ");
      Serial.println(commandRequested);
      toolState = TOOL_STATE_UNKNOWN;
    }

    Serial.print(commandRequested);
    Serial.printlnf(" door %d ", doorNumber);
    
    if( doorNumber < DOORS_IN_CABINET )
    {
      // DEVICE_HAS_CHECKOUT
      if( toolState == TOOL_STATE_CHECKOUT )
      {
          eightLocks[doorNumber].doOpen();
          eightLocks[doorNumber].isCheckedOut( true );
      }
      else if( toolState == TOOL_STATE_RETURNED )
      {
        if( DEVICE_HAS_CHECKOUT ) // A tool that does not use checkout - does not need pop open the lock on return 
        {
          eightLocks[doorNumber].doOpen();
        }
        eightLocks[doorNumber].isCheckedOut( false );
      }
    }
  }
}

void setupHardware()
{
  Wire.begin();
  Wire.beginTransmission( TCA6408_ADDR1 );
  int ioExpanderInputsPresent = Wire.endTransmission();
  if( ioExpanderInputsPresent == 0 )
  {
    ioExpanderInputs.begin( TCA6408_ADDR1 );
    ioExpanderInputs.writeByte( 0xFF, TCA6408_CONFIGURATION ); // set all channels as outputs
    ioExpanderInputs.writeByte( 0x00, TCA6408_INPUT ); // 0xff turn all channels on
  }
  else
  {
      while(1)
      {
        Particle.process();
        Serial.printlnf("Couldn't locate the ioExpanderInputs?");
      }
  }

  // if we cannot locate the I2C based I/O, we cannot operate the device
  Wire.beginTransmission( TCA6408_ADDR2 );
  int ioExpanderOutPresent = Wire.endTransmission();
  if( ioExpanderOutPresent == 0 )
  {
    ioExpanderOut.begin( TCA6408_ADDR2 );
    ioExpanderOut.writeByte( 0x00, TCA6408_CONFIGURATION ); // set all channels as outputs
    ioExpanderOut.writeByte( 0x00, TCA6408_OUTPUT ); // 0xff turn all channels on
  }
  else
  {
    while(1)
    {
      Particle.process();
      Serial.printlnf("Couldn't locate the ioExpanderOutputs?");
    }
  }

  for( int i=0; i<DOORS_IN_CABINET;i++)
  {
    char * tName = TOOL_TOPICS[i];
    eightLocks[i].begin( 7-i, tName, &ioExpanderInputs, &ioExpanderOut, &eightleds[i] );
  }
  setAllLocks( CONNECTION_LOST );

  Serial.printlnf("Required I2C hardware is present on address %#x & %#x", TCA6408_ADDR1, TCA6408_ADDR2);
}

void setAllLocks( lockStates newState )
{
  for( int i=0; i<DOORS_IN_CABINET;i++)
  {
    eightLocks[i].setState( newState );
  }
}