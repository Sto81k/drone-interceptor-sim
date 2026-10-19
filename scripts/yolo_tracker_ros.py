import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from vision_msgs.msg import Detection2D
from cv_bridge import CvBridge
import cv2
from ultralytics import YOLO
from rclpy.qos import qos_profile_sensor_data

class YoloTrackerNode(Node):
    def __init__(self):
        super().__init__('yolo_tracker')
        self.get_logger().info("Завантаження моделі YOLOv8...")
        self.model = YOLO("yolov8n.pt")
        self.bridge = CvBridge()
        
        # Підписка на відеопотік з віртуальної камери дрона
        self.subscription = self.create_subscription(
            Image,
            '/front/image_raw',
            self.image_callback,
            qos_profile_sensor_data)
            
        # Публікація Bounding Box для C++ Мозку
        self.publisher = self.create_publisher(Detection2D, '/vision/target_bbox', 10)
        
        self.get_logger().info("YOLO Tracker Node запущено. Очікування кадрів з Gazebo...")

    def image_callback(self, msg):
        try:
            # Конвертація ROS-повідомлення у формат OpenCV (BGR)
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        except Exception as e:
            self.get_logger().error(f"Помилка конвертації відео: {e}")
            return

        # Обробка кадру через нейромережу
        results = self.model.predict(source=cv_image, verbose=False)
        result = results[0]
        
        best_conf = 0.0
        best_box = None
        
        # Пошук об'єкта з найвищим рівнем впевненості
        for box in results[0].boxes:
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])
            class_name = self.model.names[cls_id]

            # Пропускаємо все, що не літак/змій АБО має впевненість менше 65%
            if conf < 0.65 or class_name not in ['airplane', 'kite']:
                continue
            
            # ОНОВЛЕННЯ: Запам'ятовуємо об'єкт з максимальною впевненістю
            if conf > best_conf:
                best_conf = conf
                best_box = box

        # Якщо знайдено підходящу ціль
        if best_box is not None:
            x_center, y_center, width, height = best_box.xywh[0]
            
            # Формування повідомлення Detection2D
            det_msg = Detection2D()
            det_msg.bbox.center.position.x = float(x_center)
            det_msg.bbox.center.position.y = float(y_center)
            det_msg.bbox.size_x = float(width)
            det_msg.bbox.size_y = float(height)
            
            self.publisher.publish(det_msg)
            
        # Візуалізація для моніторингу
        annotated_frame = result.plot()
        cv2.imshow("Drone Vision (YOLOv8)", annotated_frame)
        cv2.waitKey(1)

def main(args=None):
    rclpy.init(args=args)
    node = YoloTrackerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
