

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDA , 0x4E, 0xD9 , 0x51, 0x29 , 0x26 };


// VARIABLE POUR GET ET PUT DES REQUETES DE LA LAMPE 
const char hueUsername[] = "j841PSFiqfK2mBft7XvohGDT5VrJ1GzwjeBm0MmK";  // Hue username
const char hueHubIP[] = "192.168.20.222";  // Hue hub IP

String hueOn = ""; // état de la lampe ON/OFF : "true" ou "false"
String hueBri = ""; // luminosité de la lampe : échelle : 0-255
String hueHue = ""; // couleur de la lampe : echelle : 0-15000
String hueSat = "";
String parameters[3] = {" "," "," "};
const int hueHubPort=80;
String Lvalues[4];



// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,20,222);
EthernetClient client;
EthernetClient client2;

// Variable Connection MQTT
const char* mqttServer = "tailor.cloudmqtt.com";
const int mqttPort = 10853;
const char* mqttUser = "bnsdakja";
const char* mqttPassword = "QM8SmAIwiu8T";
const char* mqttId = "cat";

const char* inTopic1 = "Project";


PubSubClient pubClient;

unsigned long startTime;
unsigned long currentTime;
unsigned long period=10000; 


void reconnect(){
  while (!pubClient.connected()) {
    Serial.print("Connecting to MQTT...");
 
    if (pubClient.connect(mqttId, mqttUser, mqttPassword )) {
      Serial.println("connected");
      pubClient.subscribe(inTopic1);
 
    } else {
      Serial.print("failed with state ");
      Serial.println(pubClient.state());
      delay(2000);
    }
   }
}





void callback(char* topic, byte* payload, unsigned int length)
{
  
  
  String command1;
  String chaine = "";
  Serial.println("Received message: ");

  for (int i=0;i<length;i++) {
    chaine = chaine + (char)payload[i];
  }
  Serial.println(chaine);
  int num=1;
  int part=0;


  int p;
  int startIndex = 0;
  int endIndex = 0;
  int lightIndex = 1;
  int i;
  
  for( i=0; i<chaine.length(); i++)
  {
    
    if(chaine[i] == '{')                 //'{' separate every lamp
    {
      
      p=0;
      startIndex = i+1;
      
      if(i != 0)
      {
        lightIndex++;
      }
    }
      else if (chaine[i] == '/')      //'/' separate every values
      {
        if(p!=0)
        {
         
         startIndex=endIndex+1;
        }
        endIndex=i;
        
        parameters[p]=chaine.substring(startIndex,endIndex); //differentiate the values 
         p++;
      }
      else if(chaine[i] == '}')     //end of a Lamp
      {
        hueOn = parameters[0];
        hueBri = parameters[1];
        hueHue = parameters[2];
        
        if(parameters[0] == "on") // si le message contient "on"
        {
          hueOn = "true"; // on initialise la valeur à de la lampe à "true"
        }
        else if(parameters[0] == "off")
        {
          hueOn = "false";
        }

        if (hueHue=="red")       //true the app name of colours are send, we "translate" them in hue values
        {
          hueHue = "0";
        }
        else if (hueHue=="pink")
        {
          hueHue = "60000";
        }

        else if (hueHue=="purple")
        {
          hueHue = "51104";
        }
        else if (hueHue=="blue")
        {
          hueHue = "46920";
        }
        else if (hueHue=="green")
        {
          hueHue = "25500";
        }

        else if (hueHue=="yellow")
        {
          hueHue = "16500";
        }
        else if (hueHue=="orange")
        {
          hueHue = "10000";
        }
        
        
        command1="{\"on\":";                            // création d'une commande ensuite envoyée à la lampe via la fonction SetHue()
        command1+=hueOn;
        command1+=",\"sat\":254, \"bri\":";
        command1+=hueBri;
        command1+=",\"hue\":";
        command1+=hueHue;
        command1 +="}";
        
        SetHue(lightIndex,command1);

        endIndex++;
      }
    }
    delay(2000); 
    client.stop();
    reconnect();
}

void SetHue(int light, String command) // deux paramètres : l'ID de la lampe et la commande à envoyer à la lampe
{
  
  if (client.connect(hueHubIP, hueHubPort))    // connexion
  {      
    while (client.connected())
    {
      client.print("PUT /api/");    // envoie les données à la lampe 
      client.print(hueUsername); 
      client.print("/lights/");
      client.print(String(light));
      client.println("/state HTTP/1.1");
      client.println("keep-alive");
      client.print("Host: ");
      client.println(hueHubIP);
      client.print("Content-Length: ");
      client.println(command.length());
      client.println("Content-Type: text/plain;charset=UTF-8");
      client.println();
      client.println(command);
    }
  }
}






void GetHue(int light)
{
  if (client.connect(hueHubIP, hueHubPort)) {
    client.print("GET /api/");
    client.print(hueUsername);
    client.print("/lights/");
    client.print(String(light));
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(hueHubIP);
    client.println("Content-type: application/json");
    client.println("keep-alive");
    client.println();
    delay(500);
    while (client.connected())
    {
      if (client.available())
      {
                                                               //we're using finduntil() and readstringuntil() to catch the hue bri and state values  
        client.findUntil("\"on\":", "\"effect\":");
        hueOn = client.readStringUntil(','); 
        client.findUntil("\"bri\":", "\"effect\":");
        hueBri = client.readStringUntil(','); // 
        client.findUntil("\"hue\":", "\"effect\":");     
        hueHue = client.readStringUntil(','); // 
        
        Lvalues[light] = hueOn + "," + hueBri + "," + hueHue;

        if (hueOn=="true")
        {
          hueOn="on";
        }
        else
        {
          hueOn="off";
        }

        if (hueHue=="0")                      //in the app hue is set in color names, so here we're "translating them"
        {
          hueHue = "red";
        }
        else if (hueHue=="60000")
        {
          hueHue = "pink";
        }

        else if (hueHue=="51104")
        {
          hueHue="purple";
        }
        else if (hueHue=="46920")
        {
          hueHue = "blue";
        }
        else if (hueHue=="25500")
        {
          hueHue = "green";
        }
        else if (hueHue=="16500")
        {
          hueHue = "yellow";
        }
        else if (hueHue=="10000")
        {
          hueHue = "orange";
        }
        Lvalues[light] = "{"+ hueOn + "/" + hueBri + "/" + hueHue + "/}"; //putting the values in the right order
        
        break;
      }
    }
  }
  client.stop();
}

void setUpConnection(){
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(hueHubIP);
}




void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  setUpConnection();
  pubClient.setClient(client2);
  pubClient.setServer(mqttServer, mqttPort);
  pubClient.setCallback(callback);
  
  delay(1000);
  startTime=millis();
}


void loop() {
    if(!pubClient.connected()){
      reconnect();
    }
    currentTime=millis();
    String com1= "";
    if((currentTime-startTime)>=period){
      
      Serial.println("20 sec pass...");
      startTime=currentTime;
      GetHue(1);
      GetHue(2);
      GetHue(3);
    }
    com1 = Lvalues[1]+Lvalues[2]+Lvalues[3];
     Serial.println("Message sent : ");
     Serial.println(com1);
     pubClient.publish(inTopic1, com1.c_str()); //here
    pubClient.loop();
    client.connect(hueHubIP, 80);
    delay(1000);

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println("disconnected.");
  }
}


1.	Ethernet library, Arduino Reference
https://www.arduino.cc/en/reference/ethernet
2.	Arduino Client for MQTT - API Documentation
https://pubsubclient.knolleary.net/api.html#PubSubClient
3.	Forum Arduino - Topic: Timer1 and millis conflicts
https://forum.arduino.cc/index.php?topic=544924.0 
4.	Flutter: Create an MQTT app (23 nov 2019)
https://www.youtube.com/watch?v=9d8s3zFTPCM
5.	flutter_mqtt_app
https://github.com/anoop4real/Flutter_MQTT 
