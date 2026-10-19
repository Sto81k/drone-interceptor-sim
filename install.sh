#!/usr/bin/env bash
# =============================================================================
#  Встановлення всього необхідного для симуляції дрона-перехоплювача
#  ОС: Ubuntu 22.04 (звичайна або в WSL2 на Windows)
#
#  Запуск:   ./install.sh
#  Скрипт можна запускати повторно — вже зроблені кроки пропускаються.
#  Тривалість першого запуску: ~40–90 хв (залежить від інтернету і процесора).
# =============================================================================
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXT_DIR="$REPO_DIR/external"
LOG_FILE="$REPO_DIR/install.log"
# shellcheck source=versions.env
source "$REPO_DIR/versions.env"

exec > >(tee -a "$LOG_FILE") 2>&1

step() { echo; echo "=================================================================="; echo ">>> $*"; echo "=================================================================="; }
die()  { echo "ПОМИЛКА: $*" >&2; exit 1; }

# ---------------------------------------------------------------------------
step "0/9  Перевірка системи"
# ---------------------------------------------------------------------------
[ "$(id -u)" -ne 0 ] || die "Не запускайте через sudo. Просто: ./install.sh (пароль спитає сам)."
. /etc/os-release
[ "${VERSION_ID:-}" = "22.04" ] || die "Потрібна Ubuntu 22.04, а у вас: ${PRETTY_NAME:-невідомо}. ROS 2 Humble + Gazebo 11 працюють тільки на 22.04."
case "$REPO_DIR" in
  /mnt/[a-z]/*) die "Репозиторій лежить на диску Windows ($REPO_DIR). Склонуйте його у домашню папку Linux: cd ~ && git clone ..." ;;
esac
IS_WSL=0; grep -qi microsoft /proc/version && IS_WSL=1
echo "Ubuntu $VERSION_ID, WSL=$IS_WSL, ядер CPU: $(nproc), RAM: $(free -g | awk '/Mem:/{print $2}') ГБ"
FREE_GB=$(df -BG --output=avail "$HOME" | tail -1 | tr -dc '0-9')
[ "$FREE_GB" -ge 25 ] || echo "УВАГА: вільно лише ${FREE_GB} ГБ, потрібно ~25 ГБ."
sudo -v

# ---------------------------------------------------------------------------
step "1/9  Базові пакети"
# ---------------------------------------------------------------------------
sudo apt-get update
sudo apt-get install -y locales software-properties-common curl wget git tmux \
     build-essential cmake python3-pip lsb-release gnupg
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8
sudo add-apt-repository -y universe

# ---------------------------------------------------------------------------
step "2/9  Репозиторій ROS 2"
# ---------------------------------------------------------------------------
if ! apt-cache policy | grep -q packages.ros.org/ros2; then
  # Офіційний спосіб (пакет ros2-apt-source). Якщо GitHub API недоступний — старий спосіб з ключем.
  ROS_APT_SOURCE_VERSION=$(curl -fsSL https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest \
                           | grep -F '"tag_name"' | awk -F\" '{print $4}' || true)
  if [ -n "$ROS_APT_SOURCE_VERSION" ]; then
    curl -fL -o /tmp/ros2-apt-source.deb \
      "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.${VERSION_CODENAME}_all.deb"
    sudo dpkg -i /tmp/ros2-apt-source.deb
  else
    sudo curl -fsSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu ${VERSION_CODENAME} main" \
      | sudo tee /etc/apt/sources.list.d/ros2.list >/dev/null
  fi
  sudo apt-get update
else
  echo "вже налаштовано"
fi

# ---------------------------------------------------------------------------
step "3/9  ROS 2 Humble, Gazebo 11, MAVROS та ROS-пакети (найдовший apt-крок)"
# ---------------------------------------------------------------------------
sudo apt-get upgrade -y
sudo apt-get install -y \
  ros-humble-desktop python3-colcon-common-extensions python3-rosdep \
  gazebo libgazebo-dev \
  ros-humble-gazebo-ros-pkgs ros-humble-gazebo-msgs \
  ros-humble-mavros ros-humble-mavros-extras ros-humble-mavros-msgs \
  ros-humble-cv-bridge ros-humble-vision-msgs ros-humble-trajectory-msgs \
  ros-humble-rqt-image-view

# Набори даних GeographicLib для MAVROS (без них mavros не стартує)
if [ ! -d /usr/share/GeographicLib/geoids ]; then
  if [ -x /opt/ros/humble/lib/mavros/install_geographiclib_datasets.sh ]; then
    sudo /opt/ros/humble/lib/mavros/install_geographiclib_datasets.sh
  else
    wget -qO- https://raw.githubusercontent.com/mavlink/mavros/master/mavros/scripts/install_geographiclib_datasets.sh | sudo bash
  fi
fi

# ---------------------------------------------------------------------------
step "4/9  ArduPilot (закріплена версія ${ARDUPILOT_COMMIT:0:10})"
# ---------------------------------------------------------------------------
mkdir -p "$EXT_DIR"
AP="$EXT_DIR/ardupilot"
if [ ! -d "$AP/.git" ]; then
  git init -q "$AP"
  git -C "$AP" remote add origin "$ARDUPILOT_REPO"
fi
if [ "$(git -C "$AP" rev-parse HEAD 2>/dev/null || true)" != "$ARDUPILOT_COMMIT" ]; then
  # Качаємо тільки потрібний коміт без історії (~в 5 разів швидше за повний clone)
  git -C "$AP" fetch --depth 1 origin "$ARDUPILOT_COMMIT"
  git -C "$AP" checkout -q FETCH_HEAD
fi
git -C "$AP" submodule update --init --recursive --depth 1 \
  || git -C "$AP" submodule update --init --recursive

if [ ! -f "$EXT_DIR/.ardupilot_prereqs_done" ]; then
  # Офіційний скрипт ArduPilot: ставить MAVProxy, pymavlink, компілятори тощо
  (cd "$AP" && Tools/environment_install/install-prereqs-ubuntu.sh -y)
  touch "$EXT_DIR/.ardupilot_prereqs_done"
fi
export PATH="$HOME/.local/bin:$PATH"

# ---------------------------------------------------------------------------
step "5/9  Збірка ArduCopter SITL"
# ---------------------------------------------------------------------------
if [ ! -x "$AP/build/sitl/bin/arducopter" ]; then
  (cd "$AP" && ./waf configure --board sitl && ./waf copter)
else
  echo "вже зібрано"
fi

# ---------------------------------------------------------------------------
step "6/9  ardupilot_gazebo + патч фізики і камери дрона"
# ---------------------------------------------------------------------------
AG="$EXT_DIR/ardupilot_gazebo"
PATCH="$REPO_DIR/patches/ardupilot_gazebo-interceptor.patch"
if [ ! -d "$AG/.git" ]; then
  git clone "$ARDUPILOT_GAZEBO_REPO" "$AG"
fi
if [ ! -f "$AG/.interceptor_patched" ]; then
  git -C "$AG" checkout -q "$ARDUPILOT_GAZEBO_COMMIT"
  git -C "$AG" apply --whitespace=nowarn "$PATCH"
  touch "$AG/.interceptor_patched"
  echo "Патч застосовано: тяга 838->2500, маса 1.5->0.6 кг, фронтальна камера"
else
  echo "патч уже застосовано"
fi
mkdir -p "$AG/build"
(cd "$AG/build" && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j"$(nproc)")

# ---------------------------------------------------------------------------
step "7/9  Python: YOLO (ultralytics), OpenCV, PyTorch"
# ---------------------------------------------------------------------------
python3 -m pip install --user --upgrade pip
if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi >/dev/null 2>&1; then
  echo "Знайдено NVIDIA GPU -> PyTorch з CUDA"
  python3 -m pip install --user "torch==2.14.0" "torchvision==0.29.0" \
    || python3 -m pip install --user torch torchvision
else
  echo "GPU NVIDIA не знайдено -> легкий PyTorch для CPU"
  python3 -m pip install --user --index-url https://download.pytorch.org/whl/cpu "torch==2.14.0" "torchvision==0.29.0" \
    || python3 -m pip install --user --index-url https://download.pytorch.org/whl/cpu torch torchvision
fi
python3 -m pip install --user -r "$REPO_DIR/requirements.txt"

# ---------------------------------------------------------------------------
step "8/9  Збірка ROS 2 workspace (interceptor_core, gcs_core)"
# ---------------------------------------------------------------------------
set +u; source /opt/ros/humble/setup.bash; set -u
(cd "$REPO_DIR/ros2_ws" && colcon build --symlink-install)

# ---------------------------------------------------------------------------
step "9/9  Налаштування ~/.bashrc"
# ---------------------------------------------------------------------------
MARK="# >>> drone-interceptor-sim >>>"
if ! grep -qF "$MARK" "$HOME/.bashrc"; then
  {
    echo "$MARK"
    echo "source \"$REPO_DIR/setup_env.bash\""
    echo "# <<< drone-interceptor-sim <<<"
  } >> "$HOME/.bashrc"
fi
mkdir -p "$REPO_DIR/run"

echo
echo "=================================================================="
echo "  ГОТОВО!  Відкрийте НОВИЙ термінал і запустіть:"
echo "     cd \"$REPO_DIR\" && ./run_sim.sh"
echo "  Лог встановлення: $LOG_FILE"
echo "=================================================================="
