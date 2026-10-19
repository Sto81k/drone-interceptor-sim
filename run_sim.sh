#!/usr/bin/env bash
# =============================================================================
#  Запуск усієї симуляції однією командою:   ./run_sim.sh
#
#  Відкривається tmux з 7 панелями (порядок і затримки як у ручному запуску):
#    1. Gazebo (світ + дрон iris + літак Cessna)
#    2. ArduPilot SITL (польотний контролер) + MAVProxy
#    3. MAVROS (міст ArduPilot <-> ROS 2)
#    4. YOLOv8 трекер (вікно "Drone Vision")
#    5. Аніматор цілі (літак летить квадратом)
#    6. interceptor_node (C++ мозок перехоплювача)
#    7. GCS-термінал  <- тут вводите команди: 1 СТАРТ, 2 ТАРАН, 3 ПОВЕРНЕННЯ
#
#  Мишкою можна клацати по панелях. Зупинка: ./stop_sim.sh
#  Опції:  --no-wipe   не скидати параметри ArduPilot до config/ardupilot/interceptor.parm
# =============================================================================
set -euo pipefail
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SESSION=interceptor
WIPE="-w"
[ "${1:-}" = "--no-wipe" ] && WIPE=""

command -v tmux >/dev/null || { echo "Немає tmux: sudo apt install tmux"; exit 1; }
if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "Симуляція вже запущена — під'єднуюсь. (Зупинити: ./stop_sim.sh)"
  exec tmux attach -t "$SESSION"
fi
mkdir -p "$REPO_DIR/run"

ENV="source '$REPO_DIR/setup_env.bash'"
CMDS=(
  "$ENV && gazebo --verbose '$REPO_DIR/worlds/interceptor_runway.world' -s libgazebo_ros_init.so"
  "$ENV && sleep 8 && cd '$REPO_DIR/run' && sim_vehicle.py -v ArduCopter -f gazebo-iris --console $WIPE --add-param-file='$REPO_DIR/config/ardupilot/interceptor.parm'"
  "$ENV && sleep 20 && ros2 launch mavros apm.launch fcu_url:=udp://127.0.0.1:14550@14555"
  "$ENV && sleep 12 && cd '$REPO_DIR/scripts' && python3 yolo_tracker_ros.py"
  "$ENV && sleep 12 && python3 '$REPO_DIR/scripts/target_animator.py'"
  "$ENV && sleep 28 && ros2 run interceptor_core interceptor_node"
  "$ENV && sleep 30 && python3 '$REPO_DIR/scripts/gcs_terminal.py'"
)
TITLES=("1 Gazebo" "2 ArduPilot SITL" "3 MAVROS" "4 YOLO" "5 Target" "6 Interceptor" "7 GCS  <-- type 1 / 2 / 3 here")

tmux new-session -d -s "$SESSION" -n sim -x "$(tput cols 2>/dev/null || echo 200)" -y "$(tput lines 2>/dev/null || echo 50)"
tmux set -t "$SESSION" -g mouse on
tmux set -t "$SESSION" -g pane-border-status top
tmux set -t "$SESSION" -g pane-border-format " #{pane_title} "
for i in "${!CMDS[@]}"; do
  [ "$i" -gt 0 ] && tmux split-window -t "$SESSION:sim" && tmux select-layout -t "$SESSION:sim" tiled >/dev/null
  tmux select-pane -t "$SESSION:sim.$i" -T "${TITLES[$i]}"
  tmux send-keys  -t "$SESSION:sim.$i" "${CMDS[$i]}" C-m
done
tmux select-layout -t "$SESSION:sim" tiled >/dev/null
tmux select-pane -t "$SESSION:sim.6"

echo "Запуск... (~30 с до готовності). Під'єднуюсь до tmux."
exec tmux attach -t "$SESSION"
