#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ======================================================================
// Sesame Motor Tester -- XIAO ESP32-C3 + PCA9685 (this fork)
// ----------------------------------------------------------------------
// Use this sketch during calibration (build guide Phase 5), BEFORE the
// joints are attached and while every motor shaft can spin freely.
//
// Commands (Serial Monitor, 115200 baud):
//   <pca channel>,<angle>   e.g. "0,90"  -> moves the servo on PCA ch 0
//   <joint>,<angle>         e.g. "L1,90" -> same thing, by joint name
//   all,<angle>             e.g. "all,90"
//   stop                    -> turns all channels off (motors go limp)
//
// PCA9685 channel map for this build:
//   ch 0 = L1    ch 1 = L3    ch 2 = L2    ch 3 = L4
//   ch 4 = R1    ch 5 = R3    ch 6 = R2    ch 7 = R4
// Servo power must come from the PCA9685 V+ terminal (separate 5 V
// supply) -- never from the XIAO. See
// docs/wiring-guide/xiao-esp32c3-pca9685/README.md
// ======================================================================

#define I2C_SDA 6   // XIAO ESP32-C3 D4
#define I2C_SCL 7   // XIAO ESP32-C3 D5
#define PCA9685_I2C_ADDR 0x40
#define SERVO_FREQ 50
#define MIN_PULSE 732    // 0 deg   (same mapping as the main firmware)
#define MAX_PULSE 2929   // 180 deg (same mapping as the main firmware)

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);

// Joint name for each PCA channel, in channel order
const char* channelJoint[8] = {"L1", "L3", "L2", "L4", "R1", "R3", "R2", "R4"};

int jointToChannel(const String& joint) {
  for (int ch = 0; ch < 8; ch++) {
    if (joint.equalsIgnoreCase(channelJoint[ch])) return ch;
  }
  return -1;
}

void moveChannel(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulseUs = map(angle, 0, 180, MIN_PULSE, MAX_PULSE);
  pwm.writeMicroseconds(ch, pulseUs);
  Serial.print("OK: PCA ch ");
  Serial.print(ch);
  Serial.print(" (");
  Serial.print(channelJoint[ch]);
  Serial.print(") -> ");
  Serial.print(angle);
  Serial.println(" deg");
}

void stopMotors() {
  Serial.println("Stopping (all channels off)...");
  for (int ch = 0; ch < 8; ch++) pwm.setPWM(ch, 0, 0);
  Serial.println("Motors are now OFF (limp).");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Wire.begin(I2C_SDA, I2C_SCL);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  stopMotors();  // outputs off until the first command, like upstream

  Serial.println("-----------------------------------");
  Serial.println("  Sesame Motor Tester (C3 + PCA9685)");
  Serial.println("-----------------------------------");
  Serial.println("Channel map: 0=L1 1=L3 2=L2 3=L4 4=R1 5=R3 6=R2 7=R4");
  Serial.println("Commands:");
  Serial.println("  0,90    -> PCA channel 0 (L1) to 90 deg");
  Serial.println("  L1,90   -> joint L1 to 90 deg");
  Serial.println("  all,90  -> every channel to 90 deg");
  Serial.println("  stop    -> all channels off");
  Serial.println("Status: Motors are currently OFF (Limp).");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    if (input.equalsIgnoreCase("stop")) {
      stopMotors();
      return;
    }

    int commaIndex = input.indexOf(',');
    if (commaIndex == -1) {
      Serial.println("Error: Invalid format. Use '0,90', 'L1,90', 'all,90', or 'stop'.");
      return;
    }

    String cmd = input.substring(0, commaIndex);
    int angle = input.substring(commaIndex + 1).toInt();

    if (cmd.equalsIgnoreCase("all")) {
      Serial.print("Moving ALL to ");
      Serial.println(constrain(angle, 0, 180));
      for (int ch = 0; ch < 8; ch++) moveChannel(ch, angle);
      return;
    }

    int ch = jointToChannel(cmd);
    if (ch == -1 && cmd.length() > 0 && isDigit(cmd.charAt(0))) {
      ch = cmd.toInt();
      if (ch < 0 || ch > 7) {
        Serial.println("Error: PCA channel must be 0-7");
        return;
      }
    }
    if (ch == -1) {
      Serial.println("Error: Unknown motor. Use a channel 0-7 or a joint name (L1-L4, R1-R4).");
      return;
    }
    moveChannel(ch, angle);
  }
}
