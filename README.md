# WeighingScale_HX711
A simple Arduino prototype, Weighing Scale (in gram) with HX711 ADC module and 1KG load cell bar. Self-calibration will be done if weight is not 0g without tare items.

## Steps
1. Powered up without tare items
2. Wait unitl 0g is measured
3. Place items on scale and weighing it
4. Do not weight item during self-calibration.

## Required Hardware
- ESP-01s
- HX711 ADC
- Load cell bar 1KG
- 4x 1.5V Alkaline Battery
- LM1117 Voltage Regulator
- 2x Capacitor (10 uF and 35uF)

## Library
- olkal/HX711_ADC - https://github.com/olkal/HX711_ADC
- olikraus/u8g2 - https://github.com/olikraus/u8g2

## Limitation
1. Need to placed on hard and stable surface
2. Measure items with 5g above
3. Items must be removed during self-calibration 

## License
The repositories is under [MIT license](https://github.com/fsdev256/WeighingScale_HX711/blob/master/LICENSE).
