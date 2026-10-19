#!/usr/bin/env bash
# Зупиняє всю симуляцію і добиває процеси, що могли зависнути.
tmux kill-session -t interceptor 2>/dev/null || true
pkill -9 -f gzserver        2>/dev/null || true
pkill -9 -f gzclient        2>/dev/null || true
pkill -9 -f arducopter      2>/dev/null || true
pkill -9 -f mavproxy.py     2>/dev/null || true
pkill -9 -f sim_vehicle.py  2>/dev/null || true
pkill -9 -f mavros_node     2>/dev/null || true
pkill -9 -f interceptor_node 2>/dev/null || true
pkill -9 -f yolo_tracker_ros.py 2>/dev/null || true
pkill -9 -f target_animator.py  2>/dev/null || true
pkill -9 -f gcs_terminal.py     2>/dev/null || true
echo "Симуляцію зупинено."
