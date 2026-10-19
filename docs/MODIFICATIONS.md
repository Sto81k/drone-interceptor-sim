# ⚙️ Зміни відносно стандартних ArduPilot / ardupilot_gazebo

Ці зміни робилися вручну, щоб дрон літав достатньо швидко й агресивно для перехоплення.
Без них проєкт **не працює як задумано**, тому вони збережені в репозиторії і застосовуються автоматично.

## 1. Модель дрона iris: `patches/ardupilot_gazebo-interceptor.patch`

Застосовується до `khancyr/ardupilot_gazebo` @ `a28cab4` (див. `versions.env`).
Повні копії змінених файлів лежать у `patches/full_copies/`.

### `models/iris_with_ardupilot/model.sdf`
| Що | Було | Стало | Навіщо |
|---|---|---|---|
| `<multiplier>` 4 моторів (ArduPilotPlugin) | `±838` | `±2500` | ~3× більша тяга: дрон розганяється до швидкості цілі |
| Новий `camera_link` + `front_camera` | немає | 640×480, 15 Гц, FOV 1.4 рад, `libgazebo_ros_camera.so` | відео для YOLO (`/front/image_raw`) |
| `camera_joint` | немає | `fixed`, поза `0.35 0 -0.06`, нахил 0.15 рад | камера жорстко дивиться вперед |

Історія підбору тяги: 838 → 3500 → 1200 → **2500**.
(Проміжний варіант з 3500 і revolute-камерою лежав у `~/Desktop/model1.sdf`.)

### `models/iris_with_standoffs/model.sdf`
| Що | Було | Стало |
|---|---|---|
| `<mass>` корпусу `base_link` | `1.5` | `0.6` |

## 2. Параметри ArduCopter: `config/ardupilot/interceptor.parm`

Повний знімок параметрів з останнього робочого запуску автора (MAVProxy зберіг його в
`ArduCopter/mav.parm` при підключенні), без рядків статистики `STAT_*`.
`run_sim.sh` передає його як `--add-param-file` з прапорцем `-w`, тобто **кожен старт починається
саме з цих параметрів**, незалежно від того, що змінювалося в попередньому польоті.
(`./run_sim.sh --no-wipe` залишає параметри з попереднього запуску.)

Найважливіші відмінності від стандартних значень:

| Параметр | Стандартно* | У проєкті |
|---|---|---|
| `ATC_ACC_P_MAX` / `ATC_ACC_R_MAX` | 1100 | 50000 |
| `ATC_ACC_Y_MAX` | 270 | 100000 |
| `ATC_ANGLE_MAX` | 55 | 75 |
| `ATC_ANG_YAW_P` | 4.5 | 8.0 |
| `ATC_INPUT_TC` | 0.1 | 0.05 |
| `WP_SPD` / `LOIT_SPEED_MS` | — | 45 м/с |
| `WP_SPD_UP` / `WP_SPD_DN` | — | 25 м/с |
| `MOT_SPIN_MIN` / `MOT_SPIN_ARM` | — | 0.02 |
| `AVOID_ENABLE` | 0 | 3 |
| `FS_CRASH_CHECK`, `FS_EKF_ACTION` | 0 | 1 |
| `MNT1_*` (підвіс камери) | вимкнено | налаштовано |
| `MAV1_*` (частоти телеметрії для MAVROS) | 0 | 4 Гц |

\* значення з першого знімка параметрів автора (21.09.2026), до тюнінгу.

Для довідки також лежать:
- `gazebo-iris.modified.parm.reference`: ручна правка `ardupilot/Tools/autotest/default_params/gazebo-iris.parm`
  (її значення вже входять у `interceptor.parm`, патчити ArduPilot не потрібно);
- `old_snapshot_2026-09-21.parm.reference`: ранній знімок, до тюнінгу.

## 3. Світ: `worlds/interceptor_runway.world`

Основа: `~/custom_runway.world` автора (злітна смуга, ODE, плагін `gazebo_ros_state` для
`/gazebo/set_entity_state` і `/gazebo/model_states`).
**Додано:** модель `cessna` з ім'ям `cessna_c172`. Раніше її додавали вручну через вкладку Insert.

## 4. Моделі в `models/`

`cessna`, `gimbal_small_2d`, `sun`, `ground_plane` скопійовано з `~/.gazebo/models`.
Колись Gazebo качав їх з онлайн-бази, але вона більше не працює, тому вони лежать у репозиторії.
