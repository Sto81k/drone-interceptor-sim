FROM osrf/ros:humble-desktop

# Копіюємо конфіги проєкту всередину контейнера для збірки
COPY versions.env /tmp/versions.env
COPY requirements.txt /tmp/requirements.txt
COPY patches/ /tmp/patches/

# 1. Базові пакети та ROS-залежності (кроки 1-3)
RUN apt-get update && apt-get install -y \
    locales software-properties-common curl wget git tmux \
    build-essential cmake python3-pip lsb-release gnupg \
    python3-colcon-common-extensions python3-rosdep \
    gazebo libgazebo-dev \
    ros-humble-gazebo-ros-pkgs ros-humble-gazebo-msgs \
    ros-humble-mavros ros-humble-mavros-extras ros-humble-mavros-msgs \
    ros-humble-cv-bridge ros-humble-vision-msgs ros-humble-trajectory-msgs \
    ros-humble-rqt-image-view \
    && rm -rf /var/lib/apt/lists/*

# 2. Набори даних GeographicLib для MAVROS
RUN wget -qO- https://raw.githubusercontent.com/mavlink/mavros/master/mavros/scripts/install_geographiclib_datasets.sh | bash

# 3. ArduPilot та ardupilot_gazebo (кроки 4-6)
# Створюємо директорію /opt/external, щоб ізолювати важкі бінарники від вашого workspace
# Устанавливаем sudo и создаем пользователя (без пароля) для правильной сборки
RUN apt-get update && apt-get install -y sudo && \
    useradd -m -s /bin/bash ardupilot_user && \
    echo "ardupilot_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /opt/external
# Отдаем новому пользователю права на папку сборки
RUN chown -R ardupilot_user:ardupilot_user /opt/external
USER ardupilot_user
ENV USER=ardupilot_user

RUN export $(grep -v '^#' /tmp/versions.env | xargs) && \
    git clone ${ARDUPILOT_REPO} ardupilot && \
    cd ardupilot && \
    git checkout ${ARDUPILOT_COMMIT} && \
    git submodule update --init --recursive --depth 1 && \
    ./Tools/environment_install/install-prereqs-ubuntu.sh -y && \
    ./waf configure --board sitl && \
    ./waf copter && \
    # Збірка плагіна ardupilot_gazebo із застосуванням патчу
    cd /opt/external && \
    git clone ${ARDUPILOT_GAZEBO_REPO} ardupilot_gazebo && \
    cd ardupilot_gazebo && \
    git checkout ${ARDUPILOT_GAZEBO_COMMIT} && \
    git apply --whitespace=nowarn /tmp/patches/ardupilot_gazebo-interceptor.patch && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

USER root
# 4. Python-залежності (крок 7)
# За замовчуванням встановлюємо CPU-версію PyTorch для універсальності,
# а також всі пакети з вашого requirements.txt
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install --index-url https://download.pytorch.org/whl/cpu torch torchvision && \
    python3 -m pip install -r /tmp/requirements.txt

# 5. Налаштування ~/.bashrc (крок 9)
# setup_env.bash буде підтягуватися автоматично при відкритті нових терміналів у VS Code
RUN echo "# >>> drone-interceptor-sim >>>" >> /root/.bashrc && \
    echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc && \
    echo "source /workspace/setup_env.bash" >> /root/.bashrc && \
    echo "# <<< drone-interceptor-sim <<<" >> /root/.bashrc

WORKDIR /workspace