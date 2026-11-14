// BIBLIOTECAS
#include <BLEDevice.h>
#include <BLEServer.h>
#include <Wire.h>
#include <BLE2902.h>
#include "MAX30105.h"
#include <LittleFS.h>
#include "esp_timer.h"

// DEFINICION DE VARIABLES
// para encendido y ble
#define NOMBRE_DISPOSITIVO  "ESP32S3_BLE" // modificar luego
#define SERVICE_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // modificar luego
#define PIN_LED       5 // >>> modificar pines según conexión

// para imus 
#define PIN_SCL  11
#define PIN_SDA 10
#define DIR_IMU_PIE        0x68   // AD0 a GND
#define DIR_IMU_PANTORRILLA 0x69  // AD0 a 3.3V

#define FREC_MUESTREO_HZ  100
#define PERIODO_US        (1000000UL / FREC_MUESTREO_HZ)
#define ALFA_COMPLEMENTARIO 0.98f   // peso del giroscopio en el filtro

#define ACC_LSB_POR_G     8192.0f   // rango ±4g
#define GIRO_LSB_POR_GS   65.5f     // rango ±500 °/s

#define SIGNO_TOBILLO  1 // >>> MODIFICAR: si la dorsiflexión sale negativa, cambia a -1

// para emg
#define PIN_EMG         1      // SIG (a través del divisor)
#define FREC_RED_HZ         60     // >>> Perú: 60 Hz
#define MUESTRAS_POR_CICLO  10
#define FREC_EMG_HZ         (FREC_RED_HZ * MUESTRAS_POR_CICLO)   // 600 Hz
#define PERIODO_EMG_US      (1000000UL / FREC_EMG_HZ)
#define ALFA_EMG            0.05f  // pasa bajos ~5 Hz

// para calibración (antes de cada sesión)
#define T_REPOSO_MS        12000
#define T_ESTABILIZA_MS    3000    // se descartan los primeros 3 s del reposo
#define T_MAXIMA_MS        6000
#define EMG_SUBIDA_MIN_MV  30.0f   // subida mínima absoluta (mV en el pin)
#define EMG_SUBIDA_MIN_STD 5.0f    // y además al menos 5 veces el ruido de reposo
#define K_STD_ONSET        3.0f    // inicio del EMG = base + 3 desviaciones estándar
#define EMG_STD_MIN_MV     2.0f

// para sesión 0
#define REPS_BUENAS_OBJETIVO 10
#define FS_REP_HZ          100
#define N_MAX_REP          1000    // máximo 10 s por repetición
#define UMBRAL_MOV_DEG     8.0f    // empieza la repetición: el pie se aleja más de esto del reposo
#define UMBRAL_FIN_DEG     6.0f    // termina: el pie vuelve a menos de esto del reposo
#define UMBRAL_VEL_DPS     20.0f   // por debajo de esto, el pie se considera quieto
#define T_QUIETO_MS        300
#define T_ESPERA_LADO_MS   1500    // si solo hizo un lado, espera esto antes de cerrar la repetición
#define T_MIN_REP_MS       800     // movimientos más cortos se descartan
#define ROM_MIN_REP_DEG    10.0f   // movimientos con menos rango se descartan
#define T_MAX_ONSET_PREVIO_MS 1000
#define ARCHIVO_CSV        "/dataset.csv"
#define N_VARIABLES        22

// para ppg
#define PIN_SDA_PPG 8
#define PIN_SCL_PPG 9

#define PPG_OBJ_MIN       80000    // rango deseado de la señal IR cruda
#define PPG_OBJ_MAX       200000
#define PPG_SIN_CONTACTO  5000     // por debajo: el sensor no está sobre la piel
#define ALFA_PPG          0.07f    // pasa bajos ~0.3 Hz (a 25 muestras/s)

// para conexión con la app
#define CHAR_RX_UUID  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_TX_UUID  "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  // ESP32 -> App

bool imusCalibradas = false;

struct IMU {
  uint8_t direccion;
  float ax, ay, az;                  // en g
  float gx, gy, gz;                  // en °/s (gy = eje de flexión del tobillo)
  float offsetGx, offsetGy, offsetGz;
  float angulo;                      // ángulo del segmento en grados
  bool  ok;
};

IMU imuPie         = {DIR_IMU_PIE};
IMU imuPantorrilla = {DIR_IMU_PANTORRILLA};
float anguloTobillo = 0;          // pie - pantorrilla
float velTobillo = 0;             // velocidad angular del tobillo (°/s)
unsigned long tUltimaMuestra = 0;

volatile float    emgEnvolvente = 0;
volatile uint32_t muestrasEMG = 0;     // para comprobar la frecuencia real
esp_timer_handle_t timerEMG = nullptr;
float bufCiclo[MUESTRAS_POR_CICLO];
int   idxCiclo = 0;
float sumaCiclo = 0;
float med1 = 0, med2 = 0;
bool  emgIniciado = false;

MAX30105 sensorPPG;
bool     ppgOK = false;        // el sensor responde por I2C
bool     ppgListo = false;     // LED ajustado y sobre la piel
bool     ppgIniciado = false;
uint8_t  ppgCorriente = 0x1F;  // corriente del LED IR (0-255)
uint32_t ppgCrudo = 0;
float    ppgF1 = 0, ppgFiltrado = 0;

BLECharacteristic* pTxChar = nullptr;
float anguloNeutral = 0;

// comandos (app o Monitor Serie)
String comandoRecibido = "";
volatile bool hayComando = false;

// calibración
enum FaseCal { CAL_INACTIVA, CAL_REPOSO, CAL_MAXIMA };
FaseCal faseCal = CAL_INACTIVA;
unsigned long tInicioFase = 0;
double sumaEmg = 0, sumaEmg2 = 0, sumaAng = 0, sumaPpg = 0;
long   nMuestrasCal = 0;

struct Calibracion {
  float emgBase, emgStd, emgMax, emgUmbralOnset, ppgBasal;
  bool  lista;
} cal = {};

// sesión 0
bool   s0Activa = false, repEnCurso = false, repPendiente = false, esperarNeutro = false;
String pacienteId = "P00";
int    nRep = 0, nBuenas = 0, nMalas = 0;
float  variables[N_VARIABLES];

float bufTheta[N_MAX_REP], bufVel[N_MAX_REP], bufEmg[N_MAX_REP];
float bufTibPitch[N_MAX_REP], bufTibGyro2[N_MAX_REP], bufTibAcc[N_MAX_REP];
int   nBuf = 0;

unsigned long tInicioRep = 0, tNeutroDesde = 0, tEmgSobre = 0;
bool  onsetDetectado = false;
float onsetEmgMs = 0;
float thetaReposo = 0, thetaReposoRep = 0;
bool  visitoDorsi = false, visitoPlantar = false;

const char* NOMBRES_VARIABLES[N_VARIABLES] = {
  "rom_dorsi_deg", "rom_plantar_deg", "rom_total_deg", "duracion_rep_s",
  "t_dorsi_s", "t_plantar_s", "sosten_dorsi_s", "sosten_plantar_s",
  "vel_ang_pico_dps", "vel_ang_media_dps", "n_picos_velocidad",
  "tibia_pitch_std_deg", "tibia_gyro_rms_dps", "tibia_acc_din_rms_g", "tibia_pitch_media_deg",
  "emg_rms_norm", "emg_pico_norm", "emg_area_norm", "emg_duracion_activa_s",
  "emg_onset_a_mov_ms", "corr_emg_vel", "emg_por_grado"
};

BLEServer* pServer = nullptr;
volatile bool conectado = false;
bool estabaConectado = false;
bool sistemaEncendido = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override    { conectado = true;  }
  void onDisconnect(BLEServer* s) override { conectado = false; }
};

// Para recibir comandos de la app
class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String cmd = c->getValue().c_str();
    cmd.replace(String((char)0), "");
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.length() > 0) {
      comandoRecibido = cmd;
      hayComando = true;     // se procesa en loop()
    }
  }
};

void enviarBLE(const String& msg) {
  if (!conectado || pTxChar == nullptr) return;
  pTxChar->setValue((uint8_t*)msg.c_str(), msg.length());
  pTxChar->notify();
  Serial.println("[TX] " + msg);
}

//  IMU: funciones de bajo nivel
void escribirRegistro(uint8_t dir, uint8_t reg, uint8_t valor) {
  Wire.beginTransmission(dir);
  Wire.write(reg);
  Wire.write(valor);
  Wire.endTransmission();
}

bool iniciarIMU(IMU& imu) {
  Wire.beginTransmission(imu.direccion);
  if (Wire.endTransmission() != 0) return false;   // no responde

  escribirRegistro(imu.direccion, 0x6B, 0x01); // salir de sleep, reloj PLL
  delay(50);
  escribirRegistro(imu.direccion, 0x1A, 0x03); // DLPF ~44 Hz
  escribirRegistro(imu.direccion, 0x19, 0x09); // muestreo 1kHz/(1+9) = 100 Hz
  escribirRegistro(imu.direccion, 0x1B, 0x08); // giroscopio ±500 °/s
  escribirRegistro(imu.direccion, 0x1C, 0x08); // acelerómetro ±4 g
  return true;
}

bool leerIMU(IMU& imu) {
  Wire.beginTransmission(imu.direccion);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(imu.direccion, (uint8_t)14) != 14) return false;

  int16_t raw[7];
  for (int i = 0; i < 7; i++) {
    uint8_t alto = Wire.read();
    uint8_t bajo = Wire.read();
    raw[i] = (int16_t)((alto << 8) | bajo);
  }
  imu.ax = raw[0] / ACC_LSB_POR_G;
  imu.ay = raw[1] / ACC_LSB_POR_G;
  imu.az = raw[2] / ACC_LSB_POR_G;
  // raw[3] = temperatura (no se usa)
  imu.gx = raw[4] / GIRO_LSB_POR_GS - imu.offsetGx;
  imu.gy = raw[5] / GIRO_LSB_POR_GS - imu.offsetGy;
  imu.gz = raw[6] / GIRO_LSB_POR_GS - imu.offsetGz;
  return true;
}

// Offset del giroscopio (pierna quieta)
void calibrarGiroscopios() {
  Serial.println("Calibrando giroscopios... no mover la pierna");
  const int N = 300;
  IMU* imus[2] = { &imuPie, &imuPantorrilla };
  double suma[2][3] = {};

  for (int k = 0; k < 2; k++) {
    imus[k]->offsetGx = imus[k]->offsetGy = imus[k]->offsetGz = 0;
  }

  for (int i = 0; i < N; i++) {
    for (int k = 0; k < 2; k++) {
      leerIMU(*imus[k]);
      suma[k][0] += imus[k]->gx;
      suma[k][1] += imus[k]->gy;
      suma[k][2] += imus[k]->gz;
    }
    digitalWrite(PIN_LED, (i / 25) % 2);   // LED parpadea mientras calibra
    delay(5);
  }

  for (int k = 0; k < 2; k++) {
    imus[k]->offsetGx = suma[k][0] / N;
    imus[k]->offsetGy = suma[k][1] / N;
    imus[k]->offsetGz = suma[k][2] / N;
    leerIMU(*imus[k]);
    imus[k]->angulo = atan2(-imus[k]->ax, imus[k]->az) * RAD_TO_DEG;
  }
  digitalWrite(PIN_LED, HIGH);
  Serial.println("Giroscopios calibrados");
}

//  Filtro complementario 
void actualizarAngulo(IMU& imu, float dt) {
  float anguloAcc = atan2(-imu.ax, imu.az) * RAD_TO_DEG;
  imu.angulo = ALFA_COMPLEMENTARIO * (imu.angulo + imu.gy * dt)
             + (1.0f - ALFA_COMPLEMENTARIO) * anguloAcc;
}

void actualizarIMUs() {
  unsigned long ahora = micros();
  if (ahora - tUltimaMuestra < PERIODO_US) return;
  float dt = (ahora - tUltimaMuestra) / 1000000.0f;
  tUltimaMuestra = ahora;

  if (!leerIMU(imuPie) || !leerIMU(imuPantorrilla)) return;

  actualizarAngulo(imuPie, dt);
  actualizarAngulo(imuPantorrilla, dt);
  anguloTobillo = SIGNO_TOBILLO * (imuPie.angulo - imuPantorrilla.angulo) - anguloNeutral;
  velTobillo = SIGNO_TOBILLO * (imuPie.gy - imuPantorrilla.gy);
}

float mediana3(float a, float b, float c) {
  return fmaxf(fminf(a, b), fminf(fmaxf(a, b), c));
}

// Se ejecuta sola 600 veces por segundo, independiente del loop()
void muestrearEMG(void* arg) {
  float x = analogReadMilliVolts(PIN_EMG);

  if (!emgIniciado) {
    med1 = med2 = x;
    for (int i = 0; i < MUESTRAS_POR_CICLO; i++) bufCiclo[i] = x;
    sumaCiclo = x * MUESTRAS_POR_CICLO;
    emgEnvolvente = x;
    emgIniciado = true;
  }

  // 1) mediana de 3: quita picos sueltos del ADC
  float m = mediana3(x, med1, med2);
  med2 = med1; med1 = x;

  // 2) promedio de 1 ciclo de red: cancela 60 Hz y armónicos
  sumaCiclo += m - bufCiclo[idxCiclo];
  bufCiclo[idxCiclo] = m;
  idxCiclo = (idxCiclo + 1) % MUESTRAS_POR_CICLO;
  if (idxCiclo == 0) {
    sumaCiclo = 0;
    for (int i = 0; i < MUESTRAS_POR_CICLO; i++) sumaCiclo += bufCiclo[i];
  }

  // 3) pasa bajos: envolvente final
  float env = emgEnvolvente + ALFA_EMG * (sumaCiclo / MUESTRAS_POR_CICLO - emgEnvolvente);
  emgEnvolvente = env;
  muestrasEMG++;

  // durante la contracción máxima, guarda el pico en cada muestra
  if (faseCal == CAL_MAXIMA && env > cal.emgMax) cal.emgMax = env;
}

// Revisa la señal del EMG (es analógico: no se puede "detectar", solo evaluar)
const char* verificarEMG() {
  const int N = 500;
  double s = 0, s2 = 0;
  for (int i = 0; i < N; i++) {
    float x = analogReadMilliVolts(PIN_EMG);
    s += x; s2 += x * x;
    delayMicroseconds(1000);
  }
  float media = s / N;
  float ruido = sqrtf(fmaxf(s2 / N - media * media, 0));
  Serial.printf("EMG: media %.0f mV, ruido %.1f mV -> ", media, ruido);
  
  if (media > 2800)            return "SATURADO (revisar divisor, ganancia o alimentacion)";
  if (media < 5 && ruido < 2) return "SIN SENAL (pin a GND o modulo sin alimentacion)";
  if (ruido > 50)             return "RUIDO ALTO (pin al aire o electrodos sueltos)";
  return "OK";
}

// ================= PPG =================
bool iniciarPPG() {
  if (!sensorPPG.begin(Wire1, I2C_SPEED_FAST)) return false;
  // corriente, promedio 4, modo rojo+IR, 100 Hz, ancho 411 us, rango ADC 16384
  sensorPPG.setup(ppgCorriente, 4, 2, 100, 411, 16384);
  sensorPPG.setPulseAmplitudeRed(0);   // rojo apagado, solo IR
  return true;
}

// Ajusta la corriente del LED para que la señal no sature ni quede débil
bool ajustarLedPPG() {
  ppgListo = false;
  if (!ppgOK) return false;

  ppgCorriente = 0x1F;
  uint32_t ir = 0;
  for (int i = 0; i < 12; i++) {
    sensorPPG.setPulseAmplitudeIR(ppgCorriente);
    delay(150);
    sensorPPG.clearFIFO();
    ir = sensorPPG.getIR();
    Serial.printf("PPG ajuste: corriente=%d IR=%lu\n", ppgCorriente, ir);

    if (ir > PPG_OBJ_MAX && ppgCorriente > 1) {
      ppgCorriente = ppgCorriente * 7 / 10;
    } else if (ir < PPG_OBJ_MIN && ppgCorriente < 255) {
      ppgCorriente = (uint8_t)min(255, ppgCorriente * 14 / 10 + 1);
    } else {
      break;
    }
  }

  ppgListo = (ir > PPG_SIN_CONTACTO);
  ppgIniciado = false;
  Serial.println(ppgListo ? "PPG listo" : "PPG: sensor sin contacto con la piel");
  return ppgListo;
}

void actualizarPPG() {
  if (!ppgListo) return;
  sensorPPG.check();                    // trae las muestras nuevas del FIFO
  while (sensorPPG.available()) {
    ppgCrudo = sensorPPG.getFIFOIR();
    sensorPPG.nextSample();

    if (!ppgIniciado) {
      ppgF1 = ppgFiltrado = ppgCrudo;
      ppgIniciado = true;
    }
    // dos pasa bajos en cascada: quitan el pulso cardiaco y dejan la tendencia venosa
    ppgF1       += ALFA_PPG * (ppgCrudo - ppgF1);
    ppgFiltrado += ALFA_PPG * (ppgF1 - ppgFiltrado);
  }
}

// ================= CALIBRACIÓN (antes de cada sesión) =================
void iniciarCalibracion() {
  if (!imuPie.ok || !imuPantorrilla.ok) { enviarBLE("CAL_ERROR_IMU"); return; }
  enviarBLE("CAL_REPOSO");
  cal = {};

  if (!ajustarLedPPG()) { enviarBLE("CAL_ERROR_PPG"); return; }

  anguloNeutral = 0;
  calibrarGiroscopios();          // ~1.5 s, pierna quieta
  tUltimaMuestra = micros();
  imusCalibradas = true;

  sumaEmg = sumaEmg2 = sumaAng = sumaPpg = 0;
  nMuestrasCal = 0;
  faseCal = CAL_REPOSO;
  tInicioFase = millis();
}

void finalizarCalibracion() {
  faseCal = CAL_INACTIVA;
  cal.emgUmbralOnset = cal.emgBase + K_STD_ONSET * cal.emgStd;

  Serial.printf("CAL -> EMG base:%.1f std:%.1f max:%.1f onset:%.1f | PPG basal:%.0f | neutro:%.1f\n",
                cal.emgBase, cal.emgStd, cal.emgMax, cal.emgUmbralOnset,
                cal.ppgBasal, anguloNeutral);

  float subidaMinima = fmaxf(EMG_SUBIDA_MIN_MV, EMG_SUBIDA_MIN_STD * cal.emgStd);
  Serial.printf("Subida EMG: %.1f mV (minimo requerido: %.1f mV)\n",
                cal.emgMax - cal.emgBase, subidaMinima);
  if (cal.emgMax - cal.emgBase < subidaMinima) {
    enviarBLE("CAL_ERROR_EMG");
  } else {
    cal.lista = true;
    enviarBLE("CAL_OK");
  }
}

void actualizarCalibracion() {
  static unsigned long tMuestra = 0;
  if (millis() - tMuestra < 10) return;    // 100 Hz
  tMuestra = millis();
  unsigned long t = millis() - tInicioFase;

  switch (faseCal) {
    case CAL_REPOSO:
      if (t >= T_ESTABILIZA_MS) {          // se descartan los primeros segundos
        sumaEmg  += emgEnvolvente;
        sumaEmg2 += emgEnvolvente * emgEnvolvente;
        sumaAng  += anguloTobillo;
        sumaPpg  += ppgFiltrado;
        nMuestrasCal++;
      }
      if (t >= T_REPOSO_MS && nMuestrasCal > 0) {
        cal.emgBase   = sumaEmg / nMuestrasCal;
        float var     = sumaEmg2 / nMuestrasCal - cal.emgBase * cal.emgBase;
        cal.emgStd    = fmaxf(sqrtf(fmaxf(var, 0)), EMG_STD_MIN_MV);
        anguloNeutral = sumaAng / nMuestrasCal;    // desde aquí, reposo = 0°
        cal.ppgBasal  = sumaPpg / nMuestrasCal;
        cal.emgMax    = cal.emgBase;
        enviarBLE("CAL_MAXIMA");
        faseCal = CAL_MAXIMA;
        tInicioFase = millis();
      }
      break;

    case CAL_MAXIMA:
      if (t >= T_MAXIMA_MS) finalizarCalibracion();
      break;

    default: break;
  }
}

// ================= CSV (memoria interna) =================
void asegurarEncabezadoCSV() {
  if (LittleFS.exists(ARCHIVO_CSV)) return;
  File f = LittleFS.open(ARCHIVO_CSV, FILE_WRITE);
  if (!f) return;
  f.print("paciente,rep,etiqueta");
  for (int i = 0; i < N_VARIABLES; i++) { f.print(','); f.print(NOMBRES_VARIABLES[i]); }
  f.println();
  f.close();
}

void volcarCSV() {
  File f = LittleFS.open(ARCHIVO_CSV, FILE_READ);
  if (!f) { Serial.println("No hay CSV"); return; }
  Serial.println("----- INICIO CSV -----");
  while (f.available()) Serial.write(f.read());
  Serial.println("----- FIN CSV -----");
  f.close();
}

void borrarCSV() {
  LittleFS.remove(ARCHIVO_CSV);
  asegurarEncabezadoCSV();
  Serial.println("CSV borrado");
  enviarBLE("CSV_BORRADO");
}

// ================= SESIÓN 0 =================
void iniciarSesion0(const String& id) {
  if (!cal.lista) { enviarBLE("S0_ERROR_CAL"); return; }
  pacienteId = (id.length() > 0) ? id : "P00";
  nRep = nBuenas = nMalas = 0;
  repEnCurso = repPendiente = false;
  esperarNeutro = true;
  thetaReposo = 0;
  tEmgSobre = 0;
  s0Activa = true;
  enviarBLE("S0_INICIO");
  Serial.println("Sesion 0 iniciada - paciente " + pacienteId);
}

void finalizarSesion0() {
  s0Activa = false;
  repEnCurso = repPendiente = false;
  enviarBLE("S0_FIN:" + String(nBuenas) + "," + String(nMalas));
  Serial.printf("Sesion 0 terminada: %d buenas, %d malas\n", nBuenas, nMalas);
}

void iniciarRep() {
  repEnCurso = true;
  nBuf = 0;
  tInicioRep = millis();
  tNeutroDesde = 0;
  thetaReposoRep = thetaReposo;
  visitoDorsi = visitoPlantar = false;

  // ¿el EMG ya estaba activo antes del movimiento?
  if (tEmgSobre != 0) {
    onsetDetectado = true;
    onsetEmgMs = -fminf((float)(millis() - tEmgSobre), (float)T_MAX_ONSET_PREVIO_MS);
  } else {
    onsetDetectado = false;
  }
}

void calcularVariables(int n) {
  const float dt = 1.0f / FS_REP_HZ;
  float th0 = thetaReposoRep;

  // --- Ángulo ---
  int iMax = 0, iMin = 0;
  for (int i = 1; i < n; i++) {
    if (bufTheta[i] > bufTheta[iMax]) iMax = i;
    if (bufTheta[i] < bufTheta[iMin]) iMin = i;
  }
  float thMax = bufTheta[iMax], thMin = bufTheta[iMin];
  float rango = thMax - thMin;
  float umbralSostD = th0 + 0.9f * (thMax - th0);
  float umbralSostP = thMin + 0.1f * rango;

  // --- Velocidad ---
  int nSostD = 0, nSostP = 0;
  float velMax = 0, sumaVel = 0;
  for (int i = 0; i < n; i++) {
    if (bufTheta[i] > umbralSostD) nSostD++;
    if (bufTheta[i] < umbralSostP) nSostP++;
    if (bufVel[i] > velMax) velMax = bufVel[i];
    sumaVel += bufVel[i];
  }
  float umbralPico = fmaxf(UMBRAL_VEL_DPS, 0.3f * velMax);
  int nPicos = 0;
  bool arriba = false;
  for (int i = 0; i < n; i++) {
    if (!arriba && bufVel[i] > umbralPico)            { arriba = true; nPicos++; }
    else if (arriba && bufVel[i] < 0.6f * umbralPico) { arriba = false; }
  }

  // --- Tibia ---
  double sP = 0, sP2 = 0, sG2 = 0, sA2 = 0;
  for (int i = 0; i < n; i++) {
    sP  += bufTibPitch[i];
    sP2 += bufTibPitch[i] * bufTibPitch[i];
    sG2 += bufTibGyro2[i];
    sA2 += bufTibAcc[i] * bufTibAcc[i];
  }
  float pMedia = sP / n;
  float pStd   = sqrtf(fmaxf(sP2 / n - pMedia * pMedia, 0));

  // --- EMG (normalizado: 0 = reposo, 1 = contracción máxima) ---
  float rangoEmg = fmaxf(cal.emgMax - cal.emgBase, 1.0f);
  double sE = 0, sE2 = 0, sV = 0, sV2 = 0, sEV = 0;
  float ePico = 0;
  int nAct = 0;
  for (int i = 0; i < n; i++) {
    float en = fmaxf((bufEmg[i] - cal.emgBase) / rangoEmg, 0);
    float v  = bufVel[i];
    sE += en; sE2 += en * en;
    sV += v;  sV2 += v * v;
    sEV += en * v;
    if (en > ePico) ePico = en;
    if (bufEmg[i] > cal.emgUmbralOnset) nAct++;
  }
  double mE = sE / n, mV = sV / n;
  double varE = sE2 / n - mE * mE, varV = sV2 / n - mV * mV;
  float corr = (varE > 1e-9 && varV > 1e-9) ? (sEV / n - mE * mV) / sqrt(varE * varV) : NAN;
  float area = sE * dt;

  variables[0]  = thMax - th0;                 // rom_dorsi_deg
  variables[1]  = th0 - thMin;                 // rom_plantar_deg
  variables[2]  = rango;                       // rom_total_deg
  variables[3]  = n * dt;                      // duracion_rep_s
  variables[4]  = iMax * dt;                   // t_dorsi_s (inicio -> máximo)
  variables[5]  = abs(iMin - iMax) * dt;       // t_plantar_s (máximo -> mínimo)
  variables[6]  = nSostD * dt;                 // sosten_dorsi_s
  variables[7]  = nSostP * dt;                 // sosten_plantar_s
  variables[8]  = velMax;                      // vel_ang_pico_dps
  variables[9]  = sumaVel / n;                 // vel_ang_media_dps
  variables[10] = nPicos;                      // n_picos_velocidad
  variables[11] = pStd;                        // tibia_pitch_std_deg
  variables[12] = sqrt(sG2 / n);               // tibia_gyro_rms_dps
  variables[13] = sqrt(sA2 / n);               // tibia_acc_din_rms_g
  variables[14] = pMedia;                      // tibia_pitch_media_deg
  variables[15] = sqrt(sE2 / n);               // emg_rms_norm
  variables[16] = ePico;                       // emg_pico_norm
  variables[17] = area;                        // emg_area_norm
  variables[18] = nAct * dt;                   // emg_duracion_activa_s
  variables[19] = onsetDetectado ? onsetEmgMs : NAN;   // emg_onset_a_mov_ms
  variables[20] = corr;                        // corr_emg_vel
  variables[21] = (rango > 1.0f) ? area / rango : NAN; // emg_por_grado
}

void terminarRep(bool porQuietud, unsigned long msQuieto = T_QUIETO_MS) {
  repEnCurso = false;
  int n = nBuf;
  if (porQuietud) n -= msQuieto * FS_REP_HZ / 1000;      // quita el tramo quieto del final
  else            esperarNeutro = true;
  tNeutroDesde = 0;

  int duracionMs = n * 1000 / FS_REP_HZ;
  if (n < 2 || duracionMs < T_MIN_REP_MS) {
    Serial.printf("[S0] descartado: muy corto (%d ms)\n", duracionMs);
    return;
  }

  float thMax = bufTheta[0], thMin = bufTheta[0];
  for (int i = 1; i < n; i++) {
    if (bufTheta[i] > thMax) thMax = bufTheta[i];
    if (bufTheta[i] < thMin) thMin = bufTheta[i];
  }
  if (thMax - thMin < ROM_MIN_REP_DEG) {
    Serial.printf("[S0] descartado: rango pequeno (%.1f deg)\n", thMax - thMin);
    return;
  }

  calcularVariables(n);
  nRep++;
  repPendiente = true;
  // envía a la app las 22 variables de la repetición, en el mismo orden que el CSV
  String datos = "DAT:";
  for (int i = 0; i < N_VARIABLES; i++) {
    if (i > 0) datos += ',';
    datos += String(variables[i], 1);
  }
  enviarBLE(datos);
  enviarBLE("REP:" + String(nRep));
  Serial.printf("[S0] Rep %d lista (%d ms, ROM %.1f deg, EMG pico %.2f) - esperando etiqueta\n",
                nRep, duracionMs, variables[2], variables[16]);
}

void guardarEtiqueta(int etiqueta) {
  if (!repPendiente) return;

  File f = LittleFS.open(ARCHIVO_CSV, FILE_APPEND);
  if (!f) { enviarBLE("ERROR_CSV"); return; }
  f.print(pacienteId); f.print(',');
  f.print(nRep);       f.print(',');
  f.print(etiqueta);
  for (int i = 0; i < N_VARIABLES; i++) { f.print(','); f.print(variables[i], 3); }
  f.println();
  f.close();

  if (etiqueta == 1) nBuenas++; else nMalas++;
  repPendiente = false;
  esperarNeutro = true;
  enviarBLE("G:" + String(nBuenas) + "," + String(nMalas));

  if (nBuenas >= REPS_BUENAS_OBJETIVO) finalizarSesion0();
}

void actualizarSesion0() {
  static unsigned long tMuestra = 0;
  if (millis() - tMuestra < 1000 / FS_REP_HZ) return;   // 100 Hz
  tMuestra = millis();

  float dTheta   = anguloTobillo - thetaReposo;            // distancia al reposo actual
  bool  quieto   = fabsf(velTobillo) < UMBRAL_VEL_DPS;
  bool  enReposo = quieto && fabsf(dTheta) < UMBRAL_FIN_DEG;

  // seguimiento del inicio del EMG (también antes del movimiento)
  if (emgEnvolvente > cal.emgUmbralOnset) { if (tEmgSobre == 0) tEmgSobre = millis(); }
  else tEmgSobre = 0;

  if (repPendiente) return;                    // esperando la etiqueta del terapeuta

  // Espera a que el pie se quede quieto (en cualquier posición) y toma esa posición como reposo
  if (esperarNeutro) {
    if (quieto) {
      if (tNeutroDesde == 0) tNeutroDesde = millis();
      else if (millis() - tNeutroDesde >= T_QUIETO_MS) {
        esperarNeutro = false;
        thetaReposo = anguloTobillo;
        tNeutroDesde = 0;
        Serial.printf("[S0] listo: reposo en %.1f deg\n", thetaReposo);
      }
    } else {
      tNeutroDesde = 0;
    }
    return;
  }

  if (!repEnCurso) {
    if (fabsf(dTheta) < UMBRAL_MOV_DEG) {
      // en reposo: el reposo se ajusta lentamente si el pie se acomoda un poco
      if (quieto) thetaReposo += 0.02f * (anguloTobillo - thetaReposo);
      return;
    }
    iniciarRep();
    Serial.printf("[S0] inicio de repeticion (reposo %.1f deg)\n", thetaReposo);
  }

  // --- guardar muestra ---
  bufTheta[nBuf]    = anguloTobillo;
  bufVel[nBuf]      = fabsf(velTobillo);
  bufEmg[nBuf]      = emgEnvolvente;
  bufTibPitch[nBuf] = imuPantorrilla.angulo;
  bufTibGyro2[nBuf] = imuPantorrilla.gx * imuPantorrilla.gx
                    + imuPantorrilla.gy * imuPantorrilla.gy
                    + imuPantorrilla.gz * imuPantorrilla.gz;
  bufTibAcc[nBuf]   = sqrtf(imuPantorrilla.ax * imuPantorrilla.ax
                          + imuPantorrilla.ay * imuPantorrilla.ay
                          + imuPantorrilla.az * imuPantorrilla.az) - 1.0f;
  nBuf++;

  if (!onsetDetectado && emgEnvolvente > cal.emgUmbralOnset) {
    onsetDetectado = true;
    onsetEmgMs = millis() - tInicioRep;
  }

  // revision de lados visitados
  if (dTheta >  UMBRAL_MOV_DEG) visitoDorsi = true;
  if (dTheta < -UMBRAL_MOV_DEG) visitoPlantar = true;

  // si ya fue a los dos lados, cierra rápido; si solo a uno, espera para dar tiempo al otro
  unsigned long espera = (visitoDorsi && visitoPlantar) ? T_QUIETO_MS : T_ESPERA_LADO_MS;
  if (enReposo) {
    if (tNeutroDesde == 0) tNeutroDesde = millis();
    else if (millis() - tNeutroDesde >= espera) { terminarRep(true, espera); return; }
  } else {
    tNeutroDesde = 0;
  }
  if (nBuf >= N_MAX_REP) terminarRep(false);
}

// COMANDOS 
void procesarComando(const String& cmd) {
  Serial.println("[RX] " + cmd);
  bool libre = sistemaEncendido && faseCal == CAL_INACTIVA && !s0Activa;

  if (cmd == "CAL") {
    if (libre) iniciarCalibracion();
  }
  else if (cmd.startsWith("S0")) {               // "S0" o "S0:P01"
    int p = cmd.indexOf(':');
    String id = (p > 0) ? cmd.substring(p + 1) : "";
    if (libre) iniciarSesion0(id);
  }
  else if (cmd.startsWith("ETQ:")) {             // "ETQ:1" buena / "ETQ:0" mala
    if (s0Activa) guardarEtiqueta(cmd.substring(4).toInt() == 1 ? 1 : 0);
  }
  else if (cmd == "FIN") {
    if (s0Activa) finalizarSesion0();
  }
  else if (cmd == "CSV")        volcarCSV();
  else if (cmd == "CSV_BORRAR") borrarCSV();
}

// ENCENDIDO Y CONEXION DE BLE
void encender() {
  sistemaEncendido = true;
  digitalWrite(PIN_LED, HIGH);
  if (ppgOK) {
    sensorPPG.wakeUp();
  }
  tUltimaMuestra = micros();
  BLEDevice::startAdvertising();
  Serial.println("Encendido - esperando conexion..."); // MODIFICAR DESPUES
}

void apagar() {
  sistemaEncendido = false;
  imusCalibradas = false;
  faseCal = CAL_INACTIVA;
  cal.lista = false;
  ppgListo = false;
  s0Activa = false;
  repEnCurso = repPendiente = false;
  if (ppgOK) sensorPPG.shutDown();
  digitalWrite(PIN_LED, LOW);
  BLEDevice::stopAdvertising();
  if (conectado) {
    pServer->disconnect(pServer->getConnId());
  }
  Serial.println("Apagado"); // MODIFICAR DESPUES 
}

void setup() {
  Serial.begin(115200);

  Serial.setTimeout(50);
  if (!LittleFS.begin(true)) Serial.println("Error al montar LittleFS");
  else                       asegurarEncabezadoCSV();

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  analogReadResolution(12);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);   // >>> bajar a 100000 si los cables son largos y hay errores

  Wire1.begin(PIN_SDA_PPG, PIN_SCL_PPG);
  Wire1.setClock(400000);

  imuPie.ok         = iniciarIMU(imuPie);
  imuPantorrilla.ok = iniciarIMU(imuPantorrilla);
  Serial.printf("IMU pie: %s | IMU pantorrilla: %s\n",
                imuPie.ok ? "OK" : "NO DETECTADA",
                imuPantorrilla.ok ? "OK" : "NO DETECTADA");

  ppgOK = iniciarPPG();
  Serial.printf("PPG: %s\n", ppgOK ? "OK" : "NO DETECTADO");
  Serial.println(verificarEMG());

  esp_timer_create_args_t argsEMG = {};
  argsEMG.callback = &muestrearEMG;
  argsEMG.name = "emg";
  esp_timer_create(&argsEMG, &timerEMG);
  esp_timer_start_periodic(timerEMG, PERIODO_EMG_US);

  if (ppgOK) sensorPPG.shutDown();      // LED apagado hasta encender el sistema

  BLEDevice::init(NOMBRE_DISPOSITIVO);
  BLEDevice::setMTU(185);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pService->createCharacteristic(CHAR_RX_UUID, BLECharacteristic::PROPERTY_WRITE)
          ->setCallbacks(new RxCallbacks());
  pTxChar = pService->createCharacteristic(CHAR_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pTxChar->addDescriptor(new BLE2902());
  pService->start();

  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID); // No se anuncia aquí, solo cuando el sistema está encendido
  encender();
}

void loop() {
  if (conectado && !estabaConectado) {
    Serial.println("App conectada");
    estabaConectado = true;
  }

  if (!conectado && estabaConectado) {
    Serial.println("App desconectada");
    estabaConectado = false;
    if (sistemaEncendido) {
      delay(300);
      pServer->startAdvertising();
    }
  }

  //  Comandos (desde la app o desde el Monitor Serie) 
  if (hayComando) {
    hayComando = false;
    procesarComando(comandoRecibido);
  }
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    s.toUpperCase();
    if (s.length() > 0) procesarComando(s);
  }
  if (faseCal != CAL_INACTIVA) actualizarCalibracion();
  if (s0Activa)                actualizarSesion0();

    // --- Lectura de sensores ---
  if (sistemaEncendido) {
    if (imusCalibradas) actualizarIMUs();
    actualizarPPG();

    // --- Solo para prueba: borrar después ---
    static unsigned long tPrint = 0;
    if (millis() - tPrint > 20) {
      tPrint = millis();
      //Serial.printf("fase:%d,envolvente:%.0f\n", (int)faseCal * 500, emgEnvolvente);
      // Serial.printf("Pie:%.1f  Pant:%.1f  Tobillo:%.1f\n", imuPie.angulo, imuPantorrilla.angulo, anguloTobillo);
      // Serial.printf("crudo:%lu,filtrado:%.0f\n", ppgCrudo, ppgFiltrado);
    }

    static unsigned long tRate = 0;
    static uint32_t muestrasAntes = 0;
    if (millis() - tRate >= 1000) {
      tRate = millis();
      //Serial.printf("[EMG] %lu muestras/s\n", (unsigned long)(muestrasEMG - muestrasAntes));
      muestrasAntes = muestrasEMG;
    }
  }
}