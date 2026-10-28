FROM osrf/ros:humble-desktop

COPY versions.env /tmp/versions.env
COPY requirements.txt /tmp/requirements.txt
COPY patches/ /tmp/patches/

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

RUN wget -qO- https://raw.githubusercontent.com/mavlink/mavros/master/mavros/scripts/install_geographiclib_datasets.sh | bash

RUN apt-get update && apt-get install -y sudo && \
    useradd -m -s /bin/bash ardupilot_user && \
    echo "ardupilot_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /opt/external
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
    cd /opt/external && \
    git clone ${ARDUPILOT_GAZEBO_REPO} ardupilot_gazebo && \
    cd ardupilot_gazebo && \
    git checkout ${ARDUPILOT_GAZEBO_COMMIT} && \
    git apply --whitespace=nowarn /tmp/patches/ardupilot_gazebo-interceptor.patch && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

USER root

RUN apt-get update && apt-get install -y mesa-utils && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install --ignore-installed --index-url https://download.pytorch.org/whl/cpu torch torchvision && \
    python3 -m pip install --ignore-installed -r /tmp/requirements.txt

RUN echo "# >>> drone-interceptor-sim >>>" >> /root/.bashrc && \
    echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc && \
    echo "source /workspace/setup_env.bash" >> /root/.bashrc && \
    echo "# <<< drone-interceptor-sim <<<" >> /root/.bashrc

WORKDIR /workspace