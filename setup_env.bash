# Оточення проєкту. Підключається командою:   source setup_env.bash
# (install.sh додає цей рядок у ~/.bashrc автоматично)

INTERCEPTOR_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export INTERCEPTOR_DIR

# ROS 2 Humble + Gazebo Classic 11
[ -f /opt/ros/humble/setup.bash ] && source /opt/ros/humble/setup.bash
[ -f /usr/share/gazebo/setup.sh ] && source /usr/share/gazebo/setup.sh

# Де лежать ArduPilot та ardupilot_gazebo.
# За замовчуванням — у external/ всередині репозиторію (туди їх ставить install.sh).
# Якщо вони вже є деінде, задайте шляхи у ~/.bashrc ДО рядка source, напр.:
#   export ARDUPILOT_DIR=~/ardupilot
#   export ARDUPILOT_GAZEBO_DIR=~/ardupilot_gazebo
export ARDUPILOT_DIR="${ARDUPILOT_DIR:-$INTERCEPTOR_DIR/external/ardupilot}"
export ARDUPILOT_GAZEBO_DIR="${ARDUPILOT_GAZEBO_DIR:-$INTERCEPTOR_DIR/external/ardupilot_gazebo}"

# Моделі: спершу наші (cessna, gimbal, sun...), потім ardupilot_gazebo (iris з патчем)
export GAZEBO_MODEL_PATH="$INTERCEPTOR_DIR/models:$ARDUPILOT_GAZEBO_DIR/models${GAZEBO_MODEL_PATH:+:$GAZEBO_MODEL_PATH}"
export GAZEBO_PLUGIN_PATH="$ARDUPILOT_GAZEBO_DIR/build${GAZEBO_PLUGIN_PATH:+:$GAZEBO_PLUGIN_PATH}"
export GAZEBO_RESOURCE_PATH="$INTERCEPTOR_DIR/worlds${GAZEBO_RESOURCE_PATH:+:$GAZEBO_RESOURCE_PATH}"
# Онлайн-база моделей Gazebo Classic більше не працює — без цього старт зависає на хвилини
export GAZEBO_MODEL_DATABASE_URI=""

export PATH="$ARDUPILOT_DIR/Tools/autotest:$HOME/.local/bin:$PATH"

# Наш ROS 2 workspace (interceptor_core, gcs_core)
[ -f "$INTERCEPTOR_DIR/ros2_ws/install/setup.bash" ] && source "$INTERCEPTOR_DIR/ros2_ws/install/setup.bash"

# WSL2: якщо Gazebo показує чорний екран або падає з помилкою OpenGL,
# розкоментуйте рядок нижче (програмний рендеринг: повільніше, зате стабільно)
# export LIBGL_ALWAYS_SOFTWARE=1
