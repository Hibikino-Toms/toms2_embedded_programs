// @author Renshi Kato
// -----------------------------------------
// crawler_motor.ino
// 左右のモータを目標距離まで移動させるための制御プログラム
// エンコーダでカウントした値を基にモータの動作を管理し、ROS2ノードと通信を行う

// エンコーダ用ピン
const int A_pin_left = 6;   // 左エンコーダ B相
const int B_pin_left = 7;   // 左エンコーダ A相
const int A_pin_right = 3;  // 右エンコーダ A相
const int B_pin_right = 2;  // 右エンコーダ B相

// モータ制御用ピン
const int left_dir_pin = 16;   // 左モータ DIRピン
const int left_pwm_pin = 17;   // 左モータ PWMピン
const int right_dir_pin = 20;  // 右モータ DIRピン
const int right_pwm_pin = 21;  // 右モータ PWMピン

// エンコーダカウント変数
volatile int encoder_count_left = 0;   // 左エンコーダのカウント
volatile int encoder_count_right = 0;  // 右エンコーダのカウント
int target_pulses_left = 0;            // 左エンコーダ目標パルス
int target_pulses_right = 0;           // 右エンコーダ目標パルス

// パルス変換関連
// const float pulses_per_mm = 10000.0 / 610.0; // 610mmで10000パルス

// モータ制御関連
int control_delay = 10;  // 制御ループの遅延（ms）

void setup() {
  // 初期設定関数
  // エンコーダピン、モータ制御ピン、シリアル通信を初期化する

  // 左エンコーダのピン設定
  pinMode(A_pin_left, INPUT_PULLUP);
  pinMode(B_pin_left, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A_pin_left), encoderISRLeft, CHANGE);

  // 右エンコーダのピン設定
  pinMode(A_pin_right, INPUT_PULLUP);
  pinMode(B_pin_right, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A_pin_right), encoderISRRight, CHANGE);

  // モータのピン設定
  pinMode(left_dir_pin, OUTPUT);
  pinMode(left_pwm_pin, OUTPUT);
  pinMode(right_dir_pin, OUTPUT);
  pinMode(right_pwm_pin, OUTPUT);

  // シリアル通信の初期化
  Serial.begin(9600);
}

void loop() {
  // メインループ関数
  // 1. シリアル通信でROS2ノードから移動パルス量とPWM値を受信
  // 2. モータを制御して目標距離まで移動
  // 3. エンコーダカウント値をROS2ノードへ送信

  // ROS2ノードからPWM値と移動パルス量を受信
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    int comma_index = input.indexOf(',');
    if (comma_index != -1) {
      int received_pulses = input.substring(0, comma_index).toInt();
      int pwm_value = input.substring(comma_index + 1).toInt();

      target_pulses_left = received_pulses;
      target_pulses_right = received_pulses;

      controlMotorsToTarget(pwm_value);
    }
  }

  // パルスカウント値をROS2ノードに送信
  String encoder_data = String(encoder_count_left) + "," + String(encoder_count_right);
  Serial.println(encoder_data);
  delay(50);  // データ送信間隔
}

void encoderISRLeft() {
  // 左エンコーダの割り込み処理
  // エンコーダの信号を基にカウント値を増減させる

  if (digitalRead(A_pin_left) ^ digitalRead(B_pin_left)) {
    encoder_count_left++;
  } else {
    encoder_count_left--;
  }
}

void encoderISRRight() {
  // 右エンコーダの割り込み処理
  // エンコーダの信号を基にカウント値を増減させる

  if (digitalRead(A_pin_right) ^ digitalRead(B_pin_right)) {
    encoder_count_right++;
  } else {
    encoder_count_right--;
  }
}

void controlMotorsToTarget(int pwm_value) {
  // 両輪のモータを目標パルス数まで制御する関数
  // 各エンコーダのカウント値を監視しながら、目標距離までモータを動かす

  int initial_count_left = encoder_count_left;    // 左エンコーダの初期値を保存
  int initial_count_right = encoder_count_right;  // 右エンコーダの初期値を保存
  bool left_motor_active = true;
  bool right_motor_active = true;

  while (left_motor_active || right_motor_active) {
    // 左モータの制御
    int relative_count_left = encoder_count_left - initial_count_left;  // 相対カウント
    if (left_motor_active && abs(relative_count_left) < abs(target_pulses_left)) {
      digitalWrite(left_dir_pin, target_pulses_left > 0 ? LOW : HIGH);
      analogWrite(left_pwm_pin, pwm_value);
    } else {
      analogWrite(left_pwm_pin, 0);
      left_motor_active = false;  // 左モータ停止
    }

    // 右モータの制御
    int relative_count_right = encoder_count_right - initial_count_right;  // 相対カウント
    if (right_motor_active && abs(relative_count_right) < abs(target_pulses_right)) {
      digitalWrite(right_dir_pin, target_pulses_right > 0 ? HIGH : LOW);
      analogWrite(right_pwm_pin, pwm_value);
    } else {
      analogWrite(right_pwm_pin, 0);
      right_motor_active = false;  // 右モータ停止
    }

    delay(control_delay);
  }
}
