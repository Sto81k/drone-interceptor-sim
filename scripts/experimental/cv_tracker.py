import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from geometry_msgs.msg import Point
from cv_bridge import CvBridge
import cv2
import numpy as np
from rclpy.qos import qos_profile_sensor_data

class CVTracker(Node):
    def __init__(self):
        super().__init__('cv_tracker')
        self.bridge = CvBridge()
        # Підписуємось на реальну камеру з ПРАВИЛЬНИМ QoS (Sensor Data)
        self.sub = self.create_subscription(
            Image, '/camera/image_raw', self.image_callback, qos_profile_sensor_data)
        # Публікуємо координати для твого C++ мозку
        self.pub = self.create_publisher(Point, '/camera/target_pixels', 10)
        self.get_logger().info("Очі дрона [OpenCV] увімкнено! Шукаю червону ціль...")

    def image_callback(self, msg):
        # Перетворюємо зображення ROS у формат OpenCV
        cv_img = self.bridge.imgmsg_to_cv2(msg, 'bgr8')
        hsv = cv2.cvtColor(cv_img, cv2.COLOR_BGR2HSV)
        
        # Налаштування діапазону для ЧЕРВОНОГО кольору
        lower_red1 = np.array([0, 120, 70])
        upper_red1 = np.array([10, 255, 255])
        lower_red2 = np.array([170, 120, 70])
        upper_red2 = np.array([180, 255, 255])
        
        mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
        mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
        mask = mask1 + mask2
        
        # Шукаємо контури об'єктів
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        if contours:
            # Беремо найбільший об'єкт (найімовірніше це наш куб)
            c = max(contours, key=cv2.contourArea)
            if cv2.contourArea(c) > 100: # Відкидаємо дрібний шум
                M = cv2.moments(c)
                if M["m00"] != 0:
                    cx = int(M["m10"] / M["m00"])
                    cy = int(M["m01"] / M["m00"])
                    
                    # Відправляємо пікселі в C++ мозок
                    pt = Point()
                    pt.x = float(cx)
                    pt.y = float(cy)
                    self.pub.publish(pt)
                    
                    # Малюємо приціл на екрані
                    cv2.circle(cv_img, (cx, cy), 5, (0, 255, 0), -1)
                    cv2.rectangle(cv_img, (cx-20, cy-20), (cx+20, cy+20), (0, 255, 0), 2)
        
        # Показуємо картинку у власному вікні OpenCV
        cv2.imshow("Drone Vision (Target Lock)", cv_img)
        cv2.waitKey(1)

def main():
    rclpy.init()
    rclpy.spin(CVTracker())
    cv2.destroyAllWindows()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
