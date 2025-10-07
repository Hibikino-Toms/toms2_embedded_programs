// Motor Pins
#define motorDirection 9
#define runbrake 10
#define motorEnabled 11
#define pwm 5
#define encoder 1
#define AlarmReset 7
#define m0 8

// Parameters
volatile int pulseCount = 0;  // エンコーダのパルスカウント用変数

void setup() {
    pinMode(motorEnabled, OUTPUT);
    pinMode(runbrake, OUTPUT);
    pinMode(motorDirection, OUTPUT);
    pinMode(pwm, OUTPUT);
    pinMode(AlarmReset, OUTPUT);
    pinMode(m0, OUTPUT);
    digitalWrite(m0, HIGH);
    digitalWrite(motorEnabled, HIGH);  
    digitalWrite(runbrake, HIGH);
    digitalWrite(AlarmReset, LOW);
    pinMode(encoder, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoder), pulseCounter, RISING);  // RISINGエッジで割り込み

    Serial.begin(115200);
}

void loop() {
    // ROS2ノードからPWM値と移動パルス量を受信
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        int comma_index = input.indexOf(',');
        if (comma_index != -1) {
          int targetPulse = input.substring(0, comma_index).toInt();
          int pwm_value = input.substring(comma_index + 1).toInt();

          int initial_count = pulseCount;
          int relative_count = pulseCount -initial_count;
          
          // モーターを動かす
          if (targetPulse > 0){
            Forward_motor(pwm_value);
          } else {
            Reverse_motor(pwm_value);
          }

            // 目標パルス数に達するまでループ
            while (abs(relative_count) < abs(targetPulse)) {
              Serial.print(" | Encoder Pulses: ");
              Serial.println(pulseCount);  // Encoder からのパルス表示
              delay(30);
            }

            Stop_motor();
        }
    }
}

// パルスをカウントする関数
void pulseCounter() {
    pulseCount++;
}

// モーターを前進させる関数
void Forward_motor(int pwm_value) {
    digitalWrite(motorEnabled, LOW);
    digitalWrite(runbrake, LOW);
    digitalWrite(motorDirection, HIGH);
    analogWrite(pwm, pwm_value);
}

// モーターを後退させる関数
void Reverse_motor(int pwm_value) {
    digitalWrite(motorEnabled, LOW);
    digitalWrite(runbrake, LOW);
    digitalWrite(motorDirection, LOW);
    analogWrite(pwm, pwm_value);
}

// モーターを停止させる関数
void Stop_motor() {
    digitalWrite(motorEnabled, HIGH);
    digitalWrite(runbrake, HIGH);
    analogWrite(pwm, 0);
}
