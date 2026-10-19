# 🛩️ Drone Interceptor Sim

Симуляція дрона-перехоплювача в **Gazebo 11**: квадрокоптер **iris** з **ArduPilot**
бачить ціль (літак **Cessna C-172**) фронтальною камерою, **YOLOv8** знаходить її на кадрі,
а C++-вузол `interceptor_node` (ROS 2) керує дроном через **MAVROS**, щоб наздогнати ціль.

| Компонент | Версія |
|---|---|
| ОС | Ubuntu 22.04 (звичайна або WSL2 на Windows) |
| ROS 2 | Humble |
| Симулятор | Gazebo Classic 11 + плагін `ardupilot_gazebo` |
| Польотний контролер | ArduCopter SITL (коміт закріплено у [`versions.env`](versions.env)) |
| Комп'ютерний зір | ultralytics YOLOv8n, OpenCV |

---

## 🚀 Швидкий старт

> **У вас Windows?** Спершу пройдіть [docs/WINDOWS_WSL2.md](docs/WINDOWS_WSL2.md) (≈15 хв),
> потім поверніться сюди. Усі команди нижче виконуються **в терміналі Ubuntu**.

```bash
# 1. Склонувати (саме в домашню папку Linux, НЕ в /mnt/c/...)
cd ~
git clone https://github.com/Sto81k/drone-interceptor-sim.git
cd drone-interceptor-sim

# 2. Встановити все (40–90 хв, один раз; спитає пароль sudo)
./install.sh

# 3. Відкрити НОВИЙ термінал і запустити симуляцію
cd ~/drone-interceptor-sim
./run_sim.sh
```

`run_sim.sh` відкриває tmux із 7 панелями і запускає все в правильному порядку.
Приблизно за 30 с, коли в панелі **7 GCS** з'явиться меню, вводьте:

| Клавіша | Команда |
|---|---|
| `1` + Enter | **СТАРТ**: зліт |
| `2` + Enter | **ТАРАН**: перехоплення цілі |
| `3` + Enter | **ПОВЕРНЕННЯ** |

Зупинити все: `./stop_sim.sh` (або закрийте термінал і виконайте цю команду).

---

## 🧠 Як це працює

```mermaid
flowchart LR
  GZ["Gazebo 11<br/>світ + iris + Cessna"] -- "/front/image_raw" --> YOLO["yolo_tracker_ros.py<br/>YOLOv8"]
  YOLO -- "/vision/target_bbox" --> INT["interceptor_node<br/>(C++, FSM)"]
  GZ -- "/gazebo/model_states" --> GCS["gcs_terminal.py<br/>(радар + меню)"]
  GZ -- "/gazebo/model_states" --> INT
  GCS -- "/gcs/action<br/>TAKEOFF / ENGAGE / ABORT" --> INT
  INT -- "/interceptor/status" --> GCS
  INT -- "/mavros/setpoint_raw/local<br/>arming, takeoff, set_mode" --> MAVROS["MAVROS"]
  MAVROS <-- "MAVLink UDP 14550" --> SITL["ArduCopter SITL"]
  SITL <-- "плагін ArduPilotPlugin" --> GZ
  ANIM["target_animator.py"] -- "/gazebo/set_entity_state" --> GZ
```

Ручний запуск (якщо потрібно без tmux): кожен рядок в окремому терміналі, по черзі.
```bash
gazebo --verbose worlds/interceptor_runway.world -s libgazebo_ros_init.so
cd run && sim_vehicle.py -v ArduCopter -f gazebo-iris --console -w --add-param-file=../config/ardupilot/interceptor.parm
ros2 launch mavros apm.launch fcu_url:=udp://127.0.0.1:14550@14555
cd scripts && python3 yolo_tracker_ros.py
python3 scripts/target_animator.py
ros2 run interceptor_core interceptor_node
python3 scripts/gcs_terminal.py
```

---

## 📁 Що лежить у репозиторії

```
drone-interceptor-sim/
├── install.sh                 # встановлення всього з нуля
├── run_sim.sh / stop_sim.sh   # запуск і зупинка симуляції
├── setup_env.bash             # змінні оточення (GAZEBO_MODEL_PATH тощо)
├── versions.env               # закріплені коміти ArduPilot і ardupilot_gazebo
├── requirements.txt           # Python-залежності з точними версіями
│
├── ros2_ws/src/
│   ├── interceptor_core/      # ⭐ C++ мозок перехоплювача
│   └── gcs_core/              # рання C++ версія наземної станції (для історії)
├── scripts/
│   ├── yolo_tracker_ros.py    # ⭐ YOLOv8 -> bbox цілі
│   ├── target_animator.py     # ⭐ рух літака-цілі по квадрату
│   ├── gcs_terminal.py        # ⭐ наземна станція (радар + команди)
│   └── experimental/          # ранні експерименти (вебкамера, OpenCV-трекер...)
├── worlds/interceptor_runway.world   # світ: злітна смуга + iris + Cessna
├── models/                    # моделі, яких більше немає в онлайн-базі Gazebo
├── patches/
│   ├── ardupilot_gazebo-interceptor.patch  # ⭐ ваша фізика дрона + камера
│   └── full_copies/                         # ті самі файли повністю (для читання)
├── config/ardupilot/
│   └── interceptor.parm       # ⭐ повний набір параметрів ArduCopter
└── docs/                      # інструкції
```

**Що НЕ лежить у репозиторії і звідки береться**

| Що | Розмір | Звідки |
|---|---|---|
| ROS 2, Gazebo, MAVROS | ~4 ГБ | `apt` (install.sh, крок 3) |
| ArduPilot | ~1.5 ГБ | GitHub, точний коміт (крок 4–5) |
| ardupilot_gazebo | ~40 МБ | GitHub, точний коміт + наш патч (крок 6) |
| PyTorch, ultralytics | 0.5–3 ГБ | `pip` (крок 7) |
| `yolov8n.pt` | 6 МБ | ultralytics скачує сам при першому запуску |

Чому так: репозиторій важить ~15 МБ і клонується за секунди, а **точні версії**
всіх залежностей закріплені, тож кожен отримує ту саму систему, що й автор.

---

## ⚙️ Що змінено відносно «коробкової» версії

Деталі в [docs/MODIFICATIONS.md](docs/MODIFICATIONS.md). Коротко:
- **Фізика iris** (`model.sdf`): тяга моторів `838 → 2500`, маса корпусу `1.5 → 0.6 кг`.
- **Камера**: на дрон додано фронтальну камеру 640×480 @15 Гц з ROS-плагіном.
- **Параметри ArduCopter**: агресивніші прискорення, кут нахилу до 75°, швидкість до 45 м/с тощо.
  Більшість змін робилася через `param set` і жила тільки в `eeprom.bin` автора;
  тепер вони у `config/ardupilot/interceptor.parm` і завантажуються при кожному старті.

---

## 🩹 Якщо щось не працює

| Симптом | Рішення |
|---|---|
| Gazebo довго висить на старті | Переконайтеся, що виконано `source setup_env.bash` (він вимикає мертву онлайн-базу моделей). |
| Gazebo: чорне вікно / помилка OpenGL (WSL) | У `setup_env.bash` розкоментуйте `export LIBGL_ALWAYS_SOFTWARE=1`. |
| `sim_vehicle.py: command not found` | Відкрийте новий термінал (після install.sh) або `source setup_env.bash`. |
| MAVROS не бачить дрона | Зачекайте, поки в панелі 2 з'явиться `APM: EKF3 IMU0 is using GPS`, потім перезапустіть панель 3. |
| Дрон не реагує на команди | Перевірте: `ros2 topic hz /mavros/local_position/pose` має давати дані. |
| YOLO не бачить літак | Літак має бути в кадрі камери: `ros2 run rqt_image_view rqt_image_view` → `/front/image_raw`. |
| Компіляція ArduPilot падає з нестачею пам'яті | Дайте WSL більше пам'яті (див. docs/WINDOWS_WSL2.md) або `cd external/ardupilot && ./waf copter -j2`. |
| Повний перезапуск | `./stop_sim.sh && ./run_sim.sh` |

Для розробників: як оновлювати патч і параметри див. [docs/MAINTAINER.md](docs/MAINTAINER.md).
