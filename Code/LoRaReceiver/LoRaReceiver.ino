#include <LoRa.h>
#include <SSD1306.h>
#include <SPI.h>
#include "rs8.h"
#include "images.h"
#include <RH_RF95.h>
#include <RHSoftwareSPI.h>

// Pinos do display (comunicação i2c)
const int DISPLAY_ADDRESS_PIN = 0x3c;
const int DISPLAY_SDA_PIN = 4;
const int DISPLAY_SCL_PIN = 15;
const int DISPLAY_RST_PIN = 16;

// LoRa Settings (SPI)
const int LORA_SCK = 5;
const int LORA_MISO = 19;
const int LORA_MOSI = 27;
const int LORA_SS = 18;
const int LORA_RST = 15;
const int LORA_DI00 = 26;
const int LORA_FREQ = 915.0;
const int LORA_SF = 7;
const int LORA_CODING_RATE = 5;
const int LORA_BANDWIDTH = 62.5E3;

// Altura da fonte (correspondente a fonte ArialMT_Plain_16)
const int fontHeight = 16; 

// Objeto do display
SSD1306 display(DISPLAY_ADDRESS_PIN, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);

String rssi = "RSSI --";
String packSize = "--";
String packet ;

void cbk(int packetSize) {
  packet ="";
  packSize = String(packetSize,DEC); //transforma o tamanho do pacote em String para imprimirmos
  for (int i = 0; i < packetSize; i++) { 
    packet += (char) LoRa.read(); //recupera o dado recebido e concatena na variável "packet"
  }
  rssi = "RSSI=  " + String(LoRa.packetRssi(), DEC)+ "dB"; //configura a String de Intensidade de Sinal (RSSI)
  //mostrar dados em tela
  loraData();
}

// Função que inicializa o display
bool displayBegin()
{
  // Reiniciamos o display
  pinMode(DISPLAY_RST_PIN, OUTPUT);
  digitalWrite(DISPLAY_RST_PIN, LOW);
  delay(1);
  digitalWrite(DISPLAY_RST_PIN, HIGH);
  delay(1);

  return display.init(); 
}

// Função que faz algumas configuções no display
void displayConfig()
{
  // Invertemos o display verticalmente
  display.flipScreenVertically();
  // Setamos a fonte
  display.setFont(ArialMT_Plain_16);
  // Alinhamos a fonta à esquerda
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}


void loraInit()
{
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);
  delay(100);
  digitalWrite(LORA_RST, HIGH);

  sx1278_spi.setPins(LORA_MISO, LORA_MOSI, LORA_SCK);

  if (!rf95.init()) 
    Serial.println("LoRa Radio: init failed.");
  else
    Serial.println("LoRa Radio: init OK!");

  RH_RF95::ModemConfig myconfig =  {RH_RF95_BW_62_5KHZ, RH_RF95_CODING_RATE_4_5, RH_RF95_SPREADING_FACTOR_128CPS};
  rf95.setModemRegisters(&myconfig);

  float Freq = LORA_FREQ;

  if (!rf95.setFrequency(LORA_FREQ))
    Serial.println("LoRa Radio: setFrequency failed.");
  else
    Serial.printf("LoRa Radio: freqency set to %.1f MHz\n", Freq);

  Serial.printf("LoRa Radio: Max Msg size: %u Bytes\n", RH_RF95_MAX_MESSAGE_LEN);

  rf95.setModeRx();
  rf95.setTxPower(20, false);
  rf95.setSpreadingFactor(LORA_SF);
  rf95.setSignalBandwidth(LORA_BANDWIDTH);
  rf95.setCodingRate4(LORA_CODING_RATE);
  rf95.setPayloadCRC(true);

  //rf95.printRegisters();
  
}

void setup() 
{
  // Iniciamos a serial com velocidade de 9600
  Serial.begin(57600);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.enableCrc();

  // Exibimos "Starting..." na serial (debug)
  Serial.println("Starting...");

  // Iniciamos o display
  if(!displayBegin())
  {
    // Se não deu certo, exibimos falha de display na serial
    Serial.println("Display failed!");
    // E deixamos em loop infinito
    while(1);
  }

  // Configuramos o posicionamento da tela, fonte e o alinhamento do texto
  displayConfig();
  
  // Iniciamos o lora
  if(!loraBegin()) 
  {
    // Se não deu certo, exibimos falha de lora na serial
    Serial.println("LoRa failed!");
    // E deixamos em loop infinito
    while (1);
  }

  //LoRa.onReceive(cbk);
  LoRa.receive(); //habilita o Lora para receber dados

  display.clear();
  display.drawString(0, 0, "LoRa Initial success!");
  display.drawString(0, 10, "Wait for incomm data...");
  display.display();
  delay(1000);

  
}

void loop() 
{  
  //parsePacket: checa se um pacote foi recebido
  //retorno: tamanho do pacote em bytes. Se retornar 0 (ZERO) nenhum pacote foi recebido
  int packetSize = LoRa.parsePacket();
  //caso tenha recebido pacote chama a função para configurar os dados que serão mostrados em tela
  if (packetSize) { 
    cbk(packetSize);  
  }
  delay(10);
}

void loraData(){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0 , 18 , "Rx "+ packSize + " bytes");
  Serial.println("Rx "+ packSize + " bytes");
  display.drawStringMaxWidth(0 , 39 , 128, packet);
  Serial.println(packet);
  display.drawString(0, 0, rssi);
  Serial.println(rssi);  
  display.display();
}
