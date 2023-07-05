
#define MAX_VAL 1024

#define PID_FL_TO_FP(f_val)      ((int)((f_val)*PID_FP_ONE))

#define PID_FP_FRAC_BITS        8     //use 8.8 fixed point integers for pid calculations
#define PID_FP_WHOLE_BITS       7     //not including sign bit
#define PID_FP_ONE              (1<<PID_FP_FRAC_BITS)
#define PID_FP_WHOLE_MAX        ((1<<PID_FP_WHOLE_BITS)-1)
#define PID_FP_FRAC_MAX         (PID_FP_ONE-1)
#define PID_FP_MAX              (PID_FL_TO_FP(PID_FP_WHOLE_MAX) + PID_FP_FRAC_MAX)

#define LOOPS 10
int loops;
int data_len = 128;
  

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  
  
  test_float();
  test_int();
}

void loop() {
  // put your main code here, to run repeatedly:

  
}


void test_float()
{
  loops = LOOPS;
  loops *= data_len;
  
  float in[data_len];
  float out[data_len];
  
  float kp = 1.00;
  float ki = 0.10;
  float kd = 0.01;

  float i = 0;
  float p = 0;
  float d = 0;
  int total_loops = loops;
  Serial.println(F("float"));
  for(int n = 0; n < data_len; n++)
  {
    in[n] = float(random(MAX_VAL))/(float)PID_FP_ONE;
    Serial.print(in[n]); Serial.print(F(","));
  }

  long t0 = micros();
  while(loops--)
  {
    int n = (loops%data_len);
    int m = data_len - n;
    float err = (in[n] - in[m]);
    i += err;
    d = err - p;
    p = err;
    float calc = p*kp + i*ki + d*kd;
    out[m] = calc;
  }
  t0 = micros()-t0;
  Serial.println();

  for(int n = 0; n < data_len; n++)
  {
    Serial.print(out[n]); Serial.print(F(","));
  }

  Serial.println();
  Serial.print(F("time     :"));Serial.println(t0);
  Serial.print(F("time each:"));Serial.println(t0/total_loops);
  Serial.println();
}


void test_int()
{
  loops = LOOPS;
  loops *= data_len;
  
  int in[data_len];
  int out[data_len];
  
  int kp = 1.00;
  int ki = 0.10;
  int kd = 0.01;

  int i = 0;
  long p = 0;
  int d = 0;
  
  int total_loops = loops;
  Serial.println(F("int/long"));
  for(int n = 0; n < data_len; n++)
  {
    in[n] = (random(1024));
    Serial.print(in[n]); Serial.print(F(","));
  }

  long t0 = micros();
  while(loops--)
  {
    int n = (loops%data_len); //target index
    int m = data_len - n;     //feedback index
    float err = (in[n] - in[m]);
    i += err;
    d = err - p;
    p = err;
    long calc = (long)p*kp + (long)i*ki + (long)d*kd;
    out[m] = (int) (calc>>PID_FP_FRAC_BITS);
  }
  t0 = micros()-t0;
  Serial.println();

  for(int n = 0; n < data_len; n++)
  {
    Serial.print(out[n]); Serial.print(F(","));
  }

  Serial.println();
  Serial.print(F("time     :"));Serial.println(t0);
  Serial.print(F("time each:"));Serial.println(t0/total_loops);
  Serial.println();
}
