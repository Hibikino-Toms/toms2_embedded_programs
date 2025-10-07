//Raspberry Pi pico W
// タイマー設定
#include <elapsedMillis.h>
elapsedMillis timer;

//edfの設定
#include <Servo.h>
int esc_pin = 19;      //ESCへの出力ピン
int edf_val = 1400; //edfの吸い込む値
#define MAX_SIGNAL 1500  //PWM信号における最大のパルス幅[マイクロ秒]
#define MIN_SIGNAL 1000  //PWM信号における最小のパルス幅[マイクロ秒]
Servo esc;             //Servoオブジェクトを作成

//eeのあごの状態読み取り用ボタン
int upper_button_pin = 18;    //上顎用ボタン入力ピン
int under_button_pin = 17;    //下顎用ボタン入力ピン

//モータ制御用ピン
int motor_dir_pin = 20;       //回転方向
int motor_pwm_pin = 21;       //PWM値

//フォトリフレクタ用ピン
int reflective_sensor_pin = 26;

//エンドエフェクタの状態
volatile int ee_state = 0;
//エンドエフェクタのモード
volatile int mode = 0;
volatile int foto_val = 900;

/*
エンドエフェクタの状態一覧
0:下あご開く(edf停止)
1:トマト吸引(edf起動)
2:トマト吸引＋果梗切断(edf起動)
3:トマト吸引＋下あご開く(edf起動)
コマンド例：0,950,1350
各モードでタイムアウトした場合，666を返すようになっている
           モード，フォトリフレクタしきい値，EDF値
*/

void setup() {
  Serial.begin(115200);        //シリアル通信初期化
  pinMode(upper_button_pin, INPUT);
  pinMode(under_button_pin, INPUT);
  pinMode(motor_dir_pin, OUTPUT);
  pinMode(motor_pwm_pin, OUTPUT);
  esc.attach(esc_pin);
  esc.writeMicroseconds(MIN_SIGNAL);
  delay(1000);
  //esc.writeMicroseconds(0);        // edf停止
  analogWrite(motor_pwm_pin, 0);   // モータ停止
  //Serial.println("0終了");
  
}

/***************************************/
//下あご用モータ制御関数
void finger_open(){
  digitalWrite(motor_dir_pin, HIGH);
  analogWrite(motor_pwm_pin, 255);
}

void finger_close(){
  digitalWrite(motor_dir_pin, LOW);
  analogWrite(motor_pwm_pin, 255);
}

void finger_stop(){
  digitalWrite(motor_dir_pin, LOW);
  analogWrite(motor_pwm_pin, 0);
}
/***************************************/

//送られてきたデータを配列に変換
int split(String data, char delimiter, String *dst){
  int index = 0;
  int arraySize = (sizeof(data))/sizeof((data[0]));
  int datalength = data.length();
  
  for(int i = 0; i < datalength; i++){
    char tmp = data.charAt(i);
    if( tmp == delimiter ){
      index++;
      if( index > (arraySize - 1)) return -1;
    }
    else dst[index] += tmp;
  }
  return (index + 1);
}

void loop() {
   String cmds[3];
  if(Serial.available() > 0){
    timer = 0;
    String input_cmd = Serial.readStringUntil('\n');
    int index = split(input_cmd, ',', cmds);
    mode = cmds[0].toInt();
    foto_val = cmds[1].toInt();
    edf_val  = cmds[2].toInt();
    //Serial.println(foto_val);
    //Serial.println(edf_val);

    if(mode==0){
      ee_state = 0;
      esc.writeMicroseconds(MIN_SIGNAL);   // edf停止
      while(digitalRead(under_button_pin)==1){
        finger_open();              // 下あご開く
        if (timer > 7000) {
          finger_stop();
          ee_state = 666;
          timer = 0;           // Reset timer after error
          break;
        }
      }
      finger_stop();
      String str = String(ee_state) + " ";
      Serial.println(str);
    }

    else if(mode==1){
      ee_state = 1;
      while(analogRead(reflective_sensor_pin)>=foto_val){
        esc.writeMicroseconds(edf_val);  //edf起動
        if (timer > 5000) {
          finger_stop();
          ee_state = 666;
          timer = 0;           // Reset timer after error
          break;
        }
      }
      //Serial.println(analogRead(reflective_sensor_pin));
      String str = String(ee_state) + " ";
      Serial.println(str);
    }
      
    else if(mode==2){
      ee_state = 2;
      while(digitalRead(upper_button_pin)==1){
        finger_close();
        if (timer > 7000) {
          finger_stop();
          ee_state = 666;
          timer = 0;           // Reset timer after error
          break;
        }
      }
      finger_stop();
      String str = String(ee_state) + " ";
      Serial.println(str);
    }
      
    else if(mode==3){
      ee_state = 3;
      while(digitalRead(under_button_pin)==1){
        finger_open();
        if (timer > 7000) {
          finger_stop();
          ee_state = 666;
          timer = 0;           // Reset timer after error
          break;
        }
      }
      finger_stop();
      String str = String(ee_state) + " ";
      Serial.println(str);
    }
  }
}