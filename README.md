
## Hardware components and connections

* **Microcontroller:** ESP32-S3-N16R8

* **Display:** 2.8 TFT SPI 240x320 v 1.2 no touch
  * VCC -> 5V
  * Gnd -> Gnd
  * CS -> GPIO10
  * Reset -> GPIO14
  * DC -> GPIO9
  * MOSI -> GPIO11
  * SCK -> GPIO12
  * LED -> 3.3V

* **Micro SD reader:**
  * 3v3 -> 3.3V
  * CS -> GPI8
  * MOSI -> GPIO11
  * CLK -> GPIO12
  * MISO -> GPIO13
  * Gnd -> Gnd

* **Sound:** MAX98357A
  * Vin -> 5V
  * Gnd -> Gnd
  * Din -> GPIO6
  * BCLK -> GPIO5
  * LRC -> GPIO4

* **Buttons:** connect button to Gnd.
  * A       -> GPIO1
  * B       -> GPIO2
  * START  -> GPIO42
  * SELECT -> GPIO41
  * UP     -> GPIO38
  * DOWN   -> GPIO40
  * LEFT   -> GPIO39
  * RIGHT  -> GPIO21
  * EMU    -> GPIO18

* **Power**:
  * **3.3V Regulator:** HT7833
  * **5V Regulator:** MT3608
  * **Power switch:** MSK-22D18 MINI
  * **Battery:** LiPo 1S 2000mAh
  * **Charger:** generic USB C LiPo charger module