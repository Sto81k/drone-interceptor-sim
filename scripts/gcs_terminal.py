import rclpy
from rclpy.node import Node
from mavros_msgs.msg import PositionTarget
from std_msgs.msg import String
from gazebo_msgs.msg import ModelStates
import threading
import sys
import math

class GCSTerminal(Node):
    def __init__(self):
        super().__init__('gcs_terminal')
        self.cmd_pub = self.create_publisher(PositionTarget, '/gcs/target_radar_v2', 10)
        self.action_pub = self.create_publisher(String, '/gcs/action', 10)
        
        self.model_states_sub = self.create_subscription(ModelStates, '/gazebo/model_states', self.radar_callback, 10)
        self.status_sub = self.create_subscription(String, '/interceptor/status', self.status_callback, 10)
        
        self.plane_pose = None
        self.plane_twist = None
        
        # 10 Гц передача координат дрону
        self.timer_publish = self.create_timer(0.025, self.publish_radar)
        # 0.5 Гц вивід у консоль (раз на 2 секунди)
        self.timer_console = self.create_timer(2.0, self.print_radar)

    def radar_callback(self, msg):
        try:
            p_idx = msg.name.index('cessna_c172')
            self.plane_pose = msg.pose[p_idx].position
            self.plane_twist = msg.twist[p_idx].linear
        except ValueError:
            pass

    def status_callback(self, msg):
        self.get_logger().info(f"[ДРОН]: {msg.data}")

    def publish_radar(self):
        if self.plane_pose:
            msg = PositionTarget()
            msg.coordinate_frame = PositionTarget.FRAME_LOCAL_NED
            msg.type_mask = 2503 
            msg.position.x = self.plane_pose.x
            msg.position.y = self.plane_pose.y
            msg.position.z = self.plane_pose.z
            msg.velocity.x = self.plane_twist.x
            msg.velocity.y = self.plane_twist.y
            msg.velocity.z = self.plane_twist.z
            self.cmd_pub.publish(msg)

    def print_radar(self):
        if self.plane_pose:
            speed = math.hypot(self.plane_twist.x, self.plane_twist.y)
            self.get_logger().info(f"ЦІЛЬ -> X:{self.plane_pose.x:6.1f} Y:{self.plane_pose.y:6.1f} Z:{self.plane_pose.z:4.1f} | V:{speed:4.1f}m/s")

def ros_spin_thread(node):
    rclpy.spin(node)

def main():
    rclpy.init()
    gcs_node = GCSTerminal()
    threading.Thread(target=ros_spin_thread, args=(gcs_node,), daemon=True).start()
    
    print("=======================================================")
    print("  АБСОЛЮТНИЙ РАДАР GCS (Пряме підключення до Gazebo)   ")
    print("=======================================================")
    print(" 1 - СТАРТ | 2 - ТАРАН | 3 - ШВИДКІСНЕ ПОВЕРНЕННЯ      ")
    print("=======================================================\n")
    
    while True:
        try:
            choice = input()
            if choice in ['1', '2', '3']:
                cmd = "TAKEOFF" if choice == '1' else "ENGAGE" if choice == '2' else "ABORT"
                msg = String()
                msg.data = cmd
                gcs_node.action_pub.publish(msg)
                gcs_node.get_logger().info(f">>> НАКАЗ '{cmd}' НАДІСЛАНО <<<")
        except EOFError: break
        except KeyboardInterrupt: break
            
    gcs_node.destroy_node()
    rclpy.shutdown()
    sys.exit(0)

if __name__ == '__main__':
    main()
