import rclpy
from rclpy.node import Node
from std_srvs.srv import SetBool

class GCSNode(Node):
    def __init__(self):
        super().__init__('gcs_node')
        self.srv = self.create_service(SetBool, '/gcs/authorize_strike', self.authorize_callback)
        self.get_logger().info('Наземна станція готова. Чекаю запит на таран...')

    def authorize_callback(self, request, response):
        self.get_logger().warn('УВАГА: ОТРИМАНО ЗАПИТ НА ТАРАН! Надаю дозвіл...')
        response.success = True
        response.message = "Дозвіл на знищення підтверджено"
        return response

def main():
    rclpy.init()
    rclpy.spin(GCSNode())
    rclpy.shutdown()

if __name__ == '__main__':
    main()
