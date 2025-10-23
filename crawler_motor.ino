// @author Renshi Kato
// -----------------------------------------
// crawler_motor_twist_control.ino
// ROS2ノードから左右のPWM値を受信し、モータをリアルタイムに制御するプログラム
// エンコーダカウント値はROS2ノード(接続しているPC)にシリアルで送信する

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
volatile long encoder_count_left = 0;   // 左エンコーダのカウント
volatile long encoder_count_right = 0;  // 右エンコーダのカウント

// 受信したPWM値を格納する変数
int received_pwm_left = 0;
int received_pwm_right = 0;

// シリアル通信のタイムアウト管理
unsigned long last_command_time = 0;
const unsigned long command_timeout_ms = 500; // 0.5秒コマンドが来なければ停止

void setup() {
  // エンコーダピン設定
  pinMode(A_pin_left, INPUT_PULLUP);
  pinMode(B_pin_left, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A_pin_left), encoderISRLeft, CHANGE);

  pinMode(A_pin_right, INPUT_PULLUP);
  pinMode(B_pin_right, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A_pin_right), encoderISRRight, CHANGE);

  // モータのピン設定
  pinMode(left_dir_pin, OUTPUT);
  pinMode(left_pwm_pin, OUTPUT);
  pinMode(right_dir_pin, OUTPUT);
  pinMode(right_pwm_pin, OUTPUT);

  // シリアル通信の初期化
  Serial.begin(115200); // ROS2ノード側のボーレートと一致させる
  
  // 初期状態でモーターを停止
  controlMotors(0, 0);
  last_command_time = millis(); // 現在時刻を記録
}

void loop() {
  // 1. シリアル通信でROS2ノードから左右のPWM値を受信
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    int comma_index = input.indexOf(',');
    
    if (comma_index != -1) {
      received_pwm_left = input.substring(0, comma_index).toInt();
      received_pwm_right = input.substring(comma_index + 1).toInt();
      
      // 受信したPWM値を直接モーターに適用
      controlMotors(received_pwm_left, received_pwm_right);
      last_command_time = millis(); // コマンド受信時刻を更新
    }
  }

  // コマンドタイムアウト処理
  if (millis() - last_command_time > command_timeout_ms) {
    controlMotors(0, 0); // コマンドが途絶えたらモーターを停止
    // Serial.println("Timeout! Motors stopped."); // デバッグ用
  }

  // 2. エンコーダカウント値をROS2ノードに送信
  // この送信間隔はROS2ノードのオドメトリ更新レートなどに影響します。
  // 必要に応じて調整してください。
  static unsigned long last_encoder_send_time = 0;
  const unsigned long encoder_send_interval_ms = 50; // 50msごとに送信

  if (millis() - last_encoder_send_time >= encoder_send_interval_ms) {
    String encoder_data = String(encoder_count_left) + "," + String(encoder_count_right);
    Serial.println(encoder_data);
    last_encoder_send_time = millis();
  }
  
  // 短い遅延を挟むことで、CPU負荷を軽減し、安定性を向上させる
  delay(1); 
}

void encoderISRLeft() {
  // 左エンコーダの割り込み処理
  if (digitalRead(A_pin_left) == HIGH) { // A相がHIGHの時
    if (digitalRead(B_pin_left) == LOW) { // B相がLOWなら正転
      encoder_count_left++;
    } else { // B相がHIGHなら逆転
      encoder_count_left--;
    }
  } else { // A相がLOWの時
    if (digitalRead(B_pin_left) == HIGH) { // B相がHIGHなら正転
      encoder_count_left++;
    } else { // B相がLOWなら逆転
      encoder_count_left--;
    }
  }
}

void encoderISRRight() {
  // 右エンコーダの割り込み処理
  if (digitalRead(A_pin_right) == HIGH) { // A相がHIGHの時
    if (digitalRead(B_pin_right) == HIGH) { // B相がHIGHなら正転 (注意: 回転方向の定義による)
      encoder_count_right--;
    } else { // B相がLOWなら逆転
      encoder_count_right++;
    }
  } else { // A相がLOWの時
    if (digitalRead(B_pin_right) == LOW) { // B相がLOWなら正転
      encoder_count_right--;
    } else { // B相がHIGHなら逆転
      encoder_count_right++;
    }
  }
}


void controlMotors(int pwm_left, int pwm_right) {
  // 左右のモータを直接PWM値で制御する関数
  // pwm_left, pwm_right の範囲は -100 から 100 を想定 (ROS2ノード側でクリップ済み)

  // 左モータの制御
  if (pwm_left > 0) { // 前進
    digitalWrite(left_dir_pin, LOW); // モータドライバに合わせてHIGH/LOWを調整
    analogWrite(left_pwm_pin, map(pwm_left, 0, 100, 0, 255)); // 0-100を0-255にマッピング
  } else if (pwm_left < 0) { // 後退
    digitalWrite(left_dir_pin, HIGH); // モータドライバに合わせてHIGH/LOWを調整
    analogWrite(left_pwm_pin, map(abs(pwm_left), 0, 100, 0, 255));
  } else { // 停止
    analogWrite(left_pwm_pin, 0);
  }

  // 右モータの制御
  if (pwm_right > 0) { // 前進
    digitalWrite(right_dir_pin, HIGH); // モータドライバに合わせてHIGH/LOWを調整 (左右でHIGH/LOWが逆の場合が多い)
    analogWrite(right_pwm_pin, map(pwm_right, 0, 100, 0, 255));
  } else if (pwm_right < 0) { // 後退
    digitalWrite(right_dir_pin, LOW); // モータドライバに合わせてHIGH/LOWを調整
    analogWrite(right_pwm_pin, map(abs(pwm_right), 0, 100, 0, 255));
  } else { // 停止
    analogWrite(right_pwm_pin, 0);
  }
}
