#include <Arduino.h>

#include "WiFi.h"
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#define WLAN_SSID       "***********"         // Baglanti kurulacak Wi-Fi agi adi
#define WLAN_PASS       "***********"         // Baglanti kurulacak Wi-Fi agi sifresi
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                  
#define AIO_USERNAME    "************"         // Adafruit IO kullanici ismi
#define AIO_KEY         "************"         // Adafruit IO kullanici anahtari

int prob=A0; // sensorun A0 Pinine baglanması gerekmektedir
int nem_degeri; // bu degisken ile nem degeri tutulacaktir.

int su_pompasi=D0; // su pompa motoru D0 pininde bulunacaktir.

int kirmizi_led= D1; //Kirmizi ked D1 pinine baglanacaktir.
int sari_led= D4; //Sari ked D4 pinine baglanacaktir.
int yesil_led= D9; //Yesil ked D9 pinine baglanacaktir.

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY); // Adafruit IO'ya baglanma islemleri bu kisimda yapilmaktadir.
Adafruit_MQTT_Publish nem = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/toprak_nemi"); //Kartimizin toprak_nemi feed'ine nem degerini yayinlamasi saglanacaktir
Adafruit_MQTT_Subscribe buton_durumu = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/buton-durumu"); //kartimizin adafruit io'dan gelen buton komutlarini takip etmesi saglanacaktir.

/*MQTT bağlatısı gerçekleştiriliyor*/
void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) 
  {
    return;
  }
  
  Serial.print("Bulut sistemine baglaniliyor... ");
  uint8_t count = 3;
  
  while ((ret = mqtt.connect()) != 0) 
  {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("3 saniye icinde tekrar baglanti denemesi gerceklesecek...");
    mqtt.disconnect();
    delay(3000); 
    count = count - 1 ;

    if (count == 0) 
    {
      esp_restart();
    }
  }
  
  Serial.println("Bulut sistemine baglanildi!");
}


void setup() {

pinMode(su_pompasi, OUTPUT);

pinMode(kirmizi_led, OUTPUT); // led pinlerimiz output olarak belirlenmistir.
pinMode(sari_led, OUTPUT);
pinMode(yesil_led, OUTPUT);

  Serial.begin(115200); // serial ile arduino seri monitorunde gelen veriler takip edilebilecektir.
  delay(10);

  /*WiFi bağlatısı gerçekleştiriliyor*/
  Serial.println(); Serial.println();
  Serial.print(WLAN_SSID);
  Serial.print(" kablosuz agina baglaniliyor");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) //WiFi'a baglanma sureci bu asamada gerceklestirilecektir.
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Kablosuz aga baglanildi!");
  Serial.print("IP adresi: "); Serial.println(WiFi.localIP());

  mqtt.subscribe(&buton_durumu);
}

void loop() {
 MQTT_connect();
 Adafruit_MQTT_Subscribe *subscription;
 
nem_degeri=analogRead(prob);    //analog okuma yapılıp nem_degeri değişkenine atanır
Serial.println("toprak nem degeri:");
Serial.println(nem_degeri);// serial monitöre okunan değer yazdırılır

if (nem_degeri<2200){// toprakta suyun fazlaca oldugu zaman
  digitalWrite(yesil_led,HIGH);//yesil ledi yak ve bitkinin suya ihtiyaci olmadigini goster.
  Serial.println("Bitkinin yeterince suyu var");
}
else if(nem_degeri>2200 && nem_degeri<2700){// topragin hafif nemli oldugu zaman
  digitalWrite(sari_led,HIGH);//sari ledi yak ve topragin hafif nemli oldugunu goster
  Serial.println("Toprak hafif nemli");
}
if (nem_degeri>2700){// toprakta suyun olmadigi zaman
  digitalWrite(kirmizi_led,HIGH);//kirmizi ledi yak ve bitkinin suya ihtiyaci oldugunu goster.
  Serial.println("Bitkinin suya ihtiyaci var");
}

while ((subscription = mqtt.readSubscription(2000))) 
  {
    /*Buton değeri alınıyor ve ona göre led yanıp söndürülüyor */
    if(subscription == &buton_durumu) 
    {
      Serial.println();
      Serial.print(F("Gelen mesaj: "));
      Serial.println((char *)buton_durumu.lastread);

      if (!strcmp((char*) buton_durumu.lastread, "Sula")) // sulama komutunun gelip gelemdigi kontrol ediliyor
      {
        digitalWrite(su_pompasi,HIGH);// Sula komutu geldigi zaman bitkinin sulanmasi saglancaktir.
      }
      else
      {
        digitalWrite(su_pompasi,LOW);//sula komutu gelmediyse su pompasini calistirma
      }
    }

  if(!nem.publish(nem_degeri))  // nem degerinin adafruit io'ya gonderilme islemi
  {
    Serial.println(F(" Gonderilemedi!"));
  }
  else {
    Serial.println(F(" Gonderildi"));
  }
  delay(250);

}
}
