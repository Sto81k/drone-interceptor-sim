# 🔧 Для розробника: як підтримувати репозиторій

## Головне правило
Після переходу на репозиторій працюйте **тільки в ньому** (`~/drone-interceptor-sim`),
а не в старих `~/ros2_ws`, `~/*.py`, `~/custom_runway.world`. Інакше копії розійдуться.

## Якщо у вас уже встановлені ArduPilot і ardupilot_gazebo (ноутбук автора)
Не обов'язково запускати `install.sh` і ставити все заново. Додайте в `~/.bashrc`:
```bash
export ARDUPILOT_DIR=~/ardupilot
export ARDUPILOT_GAZEBO_DIR=~/ardupilot_gazebo     # тут уже ваші зміни фізики
source ~/drone-interceptor-sim/setup_env.bash
```
Приберіть старі рядки `export GAZEBO_MODEL_PATH=...` / `GAZEBO_PLUGIN_PATH=...` з `~/.bashrc`:
`setup_env.bash` виставляє їх сам. Потім один раз:
```bash
cd ~/drone-interceptor-sim/ros2_ws && colcon build --symlink-install
```

## Змінили фізику дрона (`model.sdf`)
Оновіть патч (з папки, де лежить змінений ardupilot_gazebo):
```bash
cd "$ARDUPILOT_GAZEBO_DIR"
git diff > ~/drone-interceptor-sim/patches/ardupilot_gazebo-interceptor.patch
cp models/iris_with_ardupilot/model.sdf  ~/drone-interceptor-sim/patches/full_copies/iris_with_ardupilot/
cp models/iris_with_standoffs/model.sdf ~/drone-interceptor-sim/patches/full_copies/iris_with_standoffs/
```
У тих, хто вже встановив проєкт: `rm external/ardupilot_gazebo/.interceptor_patched && git -C external/ardupilot_gazebo checkout . && ./install.sh`.

## Змінили параметри ArduCopter (`param set ...`)
У консолі MAVProxy (панель 2) збережіть усі параметри у файл:
```
param save /home/<ім'я>/drone-interceptor-sim/config/ardupilot/interceptor.parm
```
Потім приберіть статистику: `sed -i '/^STAT_/d' config/ardupilot/interceptor.parm`.
⚠️ Пам'ятайте: `run_sim.sh` за замовчуванням скидає параметри до цього файлу при кожному старті.

## Додали Python-бібліотеку
Допишіть її з точною версією в `requirements.txt` (`pip3 show <пакет>` покаже версію).

## Додали apt-пакет
Допишіть його в крок 3 `install.sh`.

## Оновили ArduPilot до нової версії
Змініть `ARDUPILOT_COMMIT` у `versions.env` і перевірте, що все літає.
