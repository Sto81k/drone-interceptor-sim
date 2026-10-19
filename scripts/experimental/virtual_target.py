import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Point
import math

class VirtualTarget(Node):
    def __init__(self):
        super().__init__('virtual_target')
        self.pixel_pub = self.create_publisher(Point, '/camera/target_pixels', 10)
        # Таймер працює незалежно від MAVROS
        self.timer = self.create_timer(0.1, self.timer_callback) 
        self.time_sec = 0.0
        self.get_logger().info("Радар цілі запущено (незалежний режим)! Малюю коло...")

    def timer_callback(self):
        self.time_sec += 0.1
        
        # Центр екрану 320x240. Ціль літає по колу навколо центру.
        u = 320.0 + 100.0 * math.sin(self.time_sec)
        v = 240.0 + 100.0 * math.cos(self.time_sec)
        
        pt = Point()
        pt.x = u
        pt.y = v
        self.pixel_pub.publish(pt)

def main():
    rclpy.init()
    rclpy.spin(VirtualTarget())
    rclpy.shutdown()

if __name__ == '__main__':
    main()


