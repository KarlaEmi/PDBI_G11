# Estado del Arte

# Patentes

| Nombre del producto | Descripción | Enlace a la patente | Comentarios relevantes (huecos o faltas qué podemos agregar) |
| --- | --- | --- | --- |
| **WEARABLE NON - INVASIVE APPARATUS FOR AND METHOD OF ENHANCING LOWER LIMBS VENOUS RETURN OF A SUBJECT** | Dispositivo no invasivo, colocado en el tobillo. Estimulación eléctrica no invasiva de la bomba muscular de la pantorrilla/pie para mejorar el retorno venoso. | [US20200298004A1 - Wearable non-invasive apparatus for and method of enhancing lower limbs venous return of a subject - Google Patents](https://patents.google.com/patent/US20200298004A1/en) | Reemplaza la contracción voluntaria del paciente por electroestimulación: no verifica la ejecución activa del ejercicio ankle pump.             Sin IMU/EMG/PPG, sin ML de personalización, sin gamificación. |
| **MONITORING COMPLIANCE USING VENOUS REFILL DETECTION** | Prenda de compresión con sensor de presión que mide el tiempo de refill venoso (VRT) para verificar cumplimiento de uso. | [US20230079468A1 - Monitoring compliance using venous refill detection - Google Patents](https://patents.google.com/patent/US20230079468A1/en) | Mide cumplimiento de una prenda de compresión, no técnica de ejercicio. Utiliza sensor de presión para estimar el VRT. Sin IMU/EMG, sin ML, sin adherencia conductual. |
| **Wearable and portable smart actuation device for DVT risk mitigation: Deep Vein Thrombosis Prevention Device (DVT-PD)** | Wearable portátil con sensores y modelo de machine learning que clasifica el movimiento del usuario y actúa una compresión adaptativa. | [US20240252389A1 - Wearable and portable smart actuation device for dvt risk mitigation: deep vein thrombosis prevention device (dvt-pd) - Google Patents](https://patents.google.com/patent/US20240252389A1/en) | Usa acelerómetro (no IMU completa con giroscopio) + EMG (MyoWare) + ESP32, pero no tiene PPG  y su ML está orientado a decidir compresión automática, no a evaluar técnica de un ejercicio activo ni dar feedback al paciente. |

# Artículos Clínicos

| Título del artículo | Descripción del artículo | Enlace a artículo | Comentarios relevantes |
| --- | --- | --- | --- |
| **Proyecto Europeo ThrombUS+** | Macroproyecto orientado al desarrollo de un wearable autónomo para prevención de TVP que integra ultrasonido, EIP, LRR, sensado cinemático y realidad extendida (XR). | [https://spj.science.org/doi/10.34133/csbj.0082](https://spj.science.org/doi/10.34133/csbj.0082) | Valida la tendencia internacional hacia sistemas multi-sensores y respalda el uso de *serious games* para asegurar la adherencia. |
| **Activity Segmentation Using Wearable Sensors for DVT/PE Risk Detection** | Investigación sobre segmentación de actividades mediante acelerometría (IMU) y electromiografía (EMG) con algoritmos de ML (>95% precisión). | [https://pmc.ncbi.nlm.nih.gov/articles/PMC7884185/](https://pmc.ncbi.nlm.nih.gov/articles/PMC7884185/) | Demuestra que el EMG es indispensable junto a la IMU (incrementa la precisión en 30%) para confirmar contracción activa y descartar movimientos pasivos. |
| **An Examination System to Detect DVT Using Light Reflection Rheographyc** | Fundamentación técnica de Reografía de Reflexión de Luz (LRR/PPG) para evaluar cambios de volumen sanguíneo y tiempo de rellenado venoso. | [An Examination System to Detect Deep Vein Thrombosis of a Lower Limb Using Light Reflection Rheography](https://www.mdpi.com/1424-8220/21/7/2446) | Sustenta técnicamente la inclusión del sensor óptico PPG para verificar el vaciado venoso efectivo en tiempo real durante el ejercicio. |
| **Effects of different frequencies of ankle pump exercise on lower limb hemodynamics: a systematic review and meta-analysis (ENCONTRAR UNO CON DURACIÓN DE TERAPIA Y CUÁNTAS VECES AL DÍA)** | Meta-análisis sistemático que determina que la frecuencia óptima del *ankle pump* es de 1 repetición cada 3–4 s (15–20 rep/min) para maximizar el retorno venoso. | [https://www.frontiersin.org/journals/physiology/articles/10.3389/fphys.2026.1880108/full](https://www.frontiersin.org/journals/physiology/articles/10.3389/fphys.2026.1880108/full) | Aporta la métrica clínica patrón para entrenar los algoritmos de cadencia y guía en tiempo real del usuario. |

# Páginas de fabricantes

# Revistas de catálogos de componentes

| **Componente** | **Características** | **Precio** | **Enlace** |
| --- | --- | --- | --- |
| **Captura de Cinemática Articular (Registro de Movimiento)** |  |  |  |
| **Sensor de Flexión Resistivo (Sensor Flex 2.2")** | • Tipo de salida: Analógica (variación resistiva por deformación).
• Rango de resistencia: ~25 kΩ (plano) a 60–110 kΩ (flectado a 180°).
• Alimentación: 3.3 V – 5.0 V (mediante divisor de tensión).
• Dimensiones y vida útil: Longitud activa de 55.88 mm (2.2"); > 1,000,000 ciclos de flexión. | **S/ 86.00** | **https://naylampmechatronics.com/sensores/522-sensor-flex-22.html** |
| Unidad de Medida Inercial IMU 6 ejes
**(IMU LSM6DSV)** | • Sensores: Acelerómetro 3D (±2g a ±16g) + Giroscopio 3D (±125 a ±4000 dps).
• Interfaz de comunicación: I²C, SPI, I³C.
• Alimentación y consumo: 1.71 V – 3.6 V (Ultra bajo consumo: ~0.65 mA en modo activo).
Procesamiento interno: Núcleo de Machine Learning (MLC) y Sensor Fusion integrado en hardware. | **S/17.49** | **https://es.aliexpress.com/item/1005011593067355.html** |
| **Unidad de Medida Inercial IMU 9 ejes
(IMU -BNO085)** | • **Sensores:** Acelerómetro 3D + Giroscopio 3D + Magnetómetro 3D (9 DOF).
• **Procesamiento interno:** Co-procesador ARM Cortex-M0+ con firmware SH-2 (salida directa de Cuaterniones y Ángulos Euler a 100–200 Hz sin carga de CPU).
• **Interfaz de comunicación:** I²C (hasta 400 kHz), SPI, UART.
**Alimentación:** 2.4 V – 3.6 V. | **S/. 36.24** | **https://es.aliexpress.com/item/1005008406240619.html** |
| **Monitoreo de Perfusión y Hemodinámica Venosa** |  |  |  |
| **Pulsioxímetro
(MAX30102)** | • **Tecnología:** PPG de reflectancia con LEDs integrados (Rojo 660 nm e Infrarrojo 880 nm) y fotodetector.
• **Interfaz de comunicación:** I²C digital (ADC interno de 18 bits, tasa de muestreo de 50 a 3200 SPS).
• **Alimentación:** 1.8 V (lógica) y 3.3 V – 5.0 V (LED drivers).
**Funcionalidad extra:** Cancelación de luz ambiental y sensor de temperatura integrado. | **S/10** | **https://nanoparuro.com/shop/max30102-modulo-sensor-de-pulso-de-ritmo-cardiaco-pulsioximetro-pulsimetro-oximetro-max30102-1672#attr=** |
| **Sensor de pulso fotoeléctrico
( Pulse Sensor Amped)** | • **Tecnología:** PPG óptico de reflexión simple (LED verde 565 nm + fototransistor).
• **Tipo de salida:** Señal analógica 0 – VCC (pre-filtrada y amplificada en placa).
• **Alimentación y consumo:** 3.3 V – 5.0 V (~4 mA).
• **Requerimientos:** Exige digitalización externa (ADC de MCU) y filtrado por software. | **S/ 15** | **https://sawers.com.pe/biomedica/sensores-ecg-emg-fuerza/kit-sensor-de-pulso-cardiaco** |
| **Convertidor de Impedancia de Múltiples Frecuencias
(AD5933)** | • **Función:** Pletismografía de Bioimpedancia (IPG) mediante inyección de CA programable (1 kHz a 100 kHz).
• **Procesamiento interno:** ADC de 12 bits a 1 MSPS + motor DSP para Transformada Discreta de Fourier (DFT).
• **Interfaz de comunicación:** I²C.
**Alimentación:** 2.7 V – 5.5 V (~10 mA). | **S/ 107.28** | **https://www.mouser.pe/en/new/analog-devices/adi-ad5933-impedance-converter/** |
| **Detección de Activación Muscular** |  |  |  |
| **Sensor EMG** | • Tipo de salida: Analógica (Señal envolvente rectificada/integrada y/o señal EMG cruda RAW).
• Captura: 3 electrodos de superficie (Ag/AgCl) mediante conector de 3.5 mm.
• Acondicionamiento de señal: Amplificador de instrumentación con CMRR alto y filtro pasabanda (20 Hz – 500 Hz).
Alimentación: Fuente simétrica ±9 V DC (o circuitos integrados con inversor de voltaje a 3.3V/5V). | **S/ 100 (kit)** | **https://mtlab.pe/producto/sensor-emg-de-senal-muscular-con-sonda-de-3-electrodos-9-v/?srsltid=AU7gw4UKly9kRrmp7l3dM4tlsP8p-FYZ6ShfnlcB9P5-fkp3rVKxvOa8** |
| **Sensor Elastomérico de Estiramiento (Stretch Sensor)** | • **Principio físico:** Variación de resistencia eléctrica por elongación mecánica lineal.
• **Resistencia base:** ~1 kΩ por centímetro en estado de reposo.
• **Tipo de salida:** Analógica (requiere divisor resistivo o puente de Wheatstone con amplificador).
**Aplicación:** Medición directa de cambios perimétricos en el gemelo/pantorrilla | **$9.95** | **https://www.adafruit.com/product/519?srsltid=AU7gw4W4xrKSOLKKqm0zhVS1muzvCrklqxrhFFnXWh7Q-mMlPMQs36sz** |
| **Gestión de Energía y Alimentación** |  |  |  |
| **Bateria Li-ion** | • **Química y formato:** Litio-Polímero (LiPo) flexible/plana o celda cilíndrica Li-Ion 18650.
• **Voltaje:** Nominal 3.7 V (Carga completa: 4.2 V, Corte de descarga: 2.75 V).
• **Capacidad y Densidad:** 1000 mAh (LiPo ligera para vestibles) a 3000 mAh (18650).
**Seguridad:** Circuito PCM interno contra cortocircuito, sobrecarga y sobredescarga. | **S/20.0** | **https://sawers.com.pe/bateria-recargable-icr-18650--37v-3000-mah?search=batería%20recargable** |
| Modulo Cargador Bateria Litio 3.7v
**(Tp4056)** | • Función: Circuito dual, es tanto cargador como protector para baterías de ion-litio 18650. 
• Voltaje Alimentación: 5V. 
• Corriente de Carga: 1A. 
• Protecciones para la Batería de Ion-Litio: -Protección de sobrecarga (Corte en 4.2V ±1%). 
• Protección de sobre descarga (Corte en 2.5V) Rango de trabajo: 2.5v - 4.2v (3.7v nominales). 
• Protección de corriente de SALIDA: 3A máx. 
Adicionales: -LED indicadores de espera y fin de carga. -Conexión de alimentación: Puerto USB TipoC o pines Condición: NUEVO | **$0.50 – $2.00 USD** | **https://www.mercadolibre.com.pe/tp4056-modulo-cargador-bateria-litio-37v-usb-tipo-c/up/MPEU2462159634#polycard_client=search-desktop&be_origin=backend&overlay_label=not_apply&search_layout=grid&position=1&type=product&tracking_id=0a815b7f-607e-462d-81e9-b85e4f221c15&wid=MPE439350751&sid=search** |
| **Regulador de Tensión LDO (3.3V)** | • **Tipo y salida:** Regulador lineal de muy baja caída (LDO) fija a 3.3 V DC.
• **Capacidad de corriente:** Hasta 600 mA continuos (AP2112K) con caída máxima de 250 mV a 600 mA.
• **Consumo propio:** Corriente de reposo ultrabaja (Iq ~ 55 µA).
**Encapsulado y estabilidad:** SOT-25 / SOT-89, bajo nivel de ruido eléctrico (PSRR 65dB), ideal para biopotenciales analógicos sensibles (EMG, PPG). | **$0.30 – $1.00 USD** | **https://www.mouser.com/datasheet/2/830/TP4056-2487427.pdf** |
| **Procesamiento Local y Comunicaciones** |  |  |  |
| **ESP32-S3-WROOM-1** | • **Procesador:** Dual-Core Xtensa LX7 de 32 bits a 240 MHz con instrucciones vectoriales para IA.
• **Conectividad:** Wi-Fi 802.11 b/g/n (2.4 GHz) + Bluetooth 5.0 (BLE / Mesh).
• **Memoria:** 512 KB SRAM + 8 MB PSRAM + 8 MB Flash.
**Factor de forma y extras:** Dimensión ultracompacta (21 x 17.5 mm en versión XIAO), antena de chip integradora, cargador Li-Po en placa. | **S/ 50.00 - S/ 60.00** | **https://mtlab.pe/producto/esp32-s3-wroom-1-usb-c-dual-core-16mb-flash-8mb-psram/** |