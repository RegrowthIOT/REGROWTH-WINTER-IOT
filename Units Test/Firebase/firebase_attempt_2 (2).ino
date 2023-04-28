//This Program is created by JCBROLABS,Please use these links-> For Web:www.jcbrolabs.org,For Mail->info@jcbrolabs.org to get in touch with us.

#include <WiFi.h>
#include <FirebaseESP32.h>

//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY ""

/* 3. Define the RTDB URL */
#define DATABASE_URL "regrowth-sendingtofirebase-default-rtdb.europe-west1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app


//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;


String main="";

String device_name,animal_ID, animal_type;
int animal_weight, temperature;
float humidity;
String name,ID, type;
int weight,temp;
float hum;


void setup()
{

  Serial.begin(115200);
delay(2000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  config.database_url = DATABASE_URL;



  //////////////////////////////////////////////////////////////////////////////////////////////
  //Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  //otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(DATABASE_URL, API_KEY);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
 // Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);

}

void loop()
{


 
    name="NODE8"; 
    ID="ID3048265389262";
    type="PIG";
    weight=123;
    temp=13;
    hum=0.76;
        
  if (Firebase.ready()) 
  {
    
    //Firebase.setInt(fbdo, main, 5);
    Firebase.setString(fbdo, "/test/device_name", name);
    Firebase.setString(fbdo, "/test/animal_ID", ID);
    Firebase.setString(fbdo, "/test/animal_type", type);
    Firebase.setInt(fbdo, "/test/animal_weight", weight);
    Firebase.setInt(fbdo, "/test/temperature", temp);
    Firebase.setFloat(fbdo, "/test/humidity", hum);

    delay(200);

     Serial.printf("Get Device Name  %s\n", Firebase.getString(fbdo, "/test/device_name") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
     device_name=fbdo.to<String>();
    Serial.printf("Get Animal Name %s\n", Firebase.getString(fbdo, "/test/animal_ID") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
     animal_ID=fbdo.to<String>();
    Serial.printf("Get Animal Type  %s\n", Firebase.getString(fbdo, "/test/animal_type") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
     animal_type=fbdo.to<String>();
     Serial.printf("Get Animal Weight  %s\n", Firebase.getInt(fbdo, "/test/animal_weight") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
     animal_weight=fbdo.to<int>();
      Serial.printf("Get Temperature  %s\n", Firebase.getInt(fbdo, "/test/temperature") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
     temperature=fbdo.to<int>();
      Serial.printf("Get Humidity  %s\n", Firebase.getFloat(fbdo, "/test/humidity") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
     humidity=fbdo.to<float>();
     
     

  Serial.println(); 
   Serial.print("name:");
  Serial.print(device_name); 
  Serial.print("ID:");
  Serial.print(animal_ID);
  Serial.print("  type: ");
  Serial.print(animal_type);
   Serial.print("  weight: ");
  Serial.print(animal_weight);
   Serial.print("  temperature: ");
  Serial.print(temperature);
   Serial.print("  humidity: ");
  Serial.print(humidity);
  
  Serial.println();
  Serial.println("------------------");
  Serial.println();
  

  delay(2500);
  }
}
