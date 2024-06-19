#include <Wire.h>
#include <MPU6050_light.h>
#include <tcs3200.h>

#define num_of_colors 5

// distinctRGB[] array declares calibration values for each declared color in distinctColors[] array
int distinctRGB[num_of_colors][3] = {{40, 37, 125}, {4, 3, 13}, {25, 9, 38}, {13, 21, 50}, {12, 10, 41}};
// distinctColors[] array declares values to be returned from closestColor() function if specified color is recognised
String distinctColors[num_of_colors] = {"#FFFFFF", "#000000", "#FF0000", "#00FF00", "#0000FF"};

int red, green, blue;

MPU6050 mpu(Wire);
tcs3200 tcs(4, 5, 6, 7, 8); // (S0, S1, S2, S3, output pin)
unsigned long timer = 0;
bool transmitData = false;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    byte status = mpu.begin();
    while (status != 0) { } // stop everything if could not connect to MPU6050
    // delay(1000);
    mpu.calcOffsets(); // gyro and accelero
    Serial.println("done");
}

void loop() {
    if (millis() - timer > 100) { 
      timer = millis();
      mpu.update();
    }
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim(); // Remove any leading or trailing whitespace
        if (command.equalsIgnoreCase("c")) {
          Serial.println(tcs.closestColor(distinctRGB, distinctColors, num_of_colors));
        } else if (command.equalsIgnoreCase("a")) {
            float angleZ = mpu.getAngleZ();
            if (angleZ >= 0) {
                Serial.print(" ");
            }
            Serial.println(angleZ);
        } else if (command.equalsIgnoreCase("p")) {
          Serial.println("done");
        }
    }

    // if (transmitData) {
    //     mpu.update();
    //     if ((millis() - timer) > 10) { // print data every 10ms
    //         Serial.print("Theta : ");
    //         float angleZ = mpu.getAngleZ();
    //         if (angleZ >= 0) {
    //             Serial.print(" ");
    //         }
    //         Serial.print(angleZ);
    //         timer = millis();
    //     }
    //     Serial.print(" | Color: ");
    //     Serial.print(tcs.closestColor(distinctRGB, distinctColors, num_of_colors));
    //     Serial.print("\n");
    //     // red = tcs.colorRead('r');   //reads color value for red
    //     // green = tcs.colorRead('g');   //reads color value for green
    //     // blue = tcs.colorRead('b');    //reads color value for blue

    //     delay(10);
    // }
}
