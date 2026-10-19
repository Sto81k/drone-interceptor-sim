import rclpy
from rclpy.node import Node
from gazebo_msgs.srv import SetEntityState
import math
import time

class TargetAnimator(Node):
    def __init__(self):
        super().__init__('target_animator')
        self.client = self.create_client(SetEntityState, '/gazebo/set_entity_state')
        while not self.client.wait_for_service(timeout_sec=1.0):
            pass
        self.timer = self.create_timer(0.02, self.update_pose) 
        self.model_name = 'cessna_c172'
        self.altitude = 40.0
        self.speed = 19.4  # ~120 км/год
        
        # Параметри квадрата (можете змінити розміри під свої потреби)
        self.x_min = -250.0
        self.x_max = 250.0
        self.y_min = 0.0
        self.y_max = 500.0
        self.turn_radius = 60.0
        
        # Початковий стан: летимо вздовж осі X
        self.state = 'FORWARD_X'
        self.current_x = self.x_min
        self.current_y = self.y_min
        self.current_heading = 0.0  
        
        self.turn_angle = 0.0
        self.turn_center_x = 0.0
        self.turn_center_y = 0.0
        
        self.last_time = time.time()

    def update_pose(self):
        now = time.time()
        dt = now - self.last_time
        self.last_time = now
        if dt > 0.1:
            dt = 0.02  

        omega = self.speed / self.turn_radius  # Кутова швидкість розвороту

        # 1. Пряма ділянка: Вперед по осі X
        if self.state == 'FORWARD_X':
            self.current_y = self.y_min
            self.current_x += self.speed * dt
            self.current_heading = 0.0
            if self.current_x >= self.x_max:
                self.state = 'TURN_1'
                self.turn_center_x = self.x_max
                self.turn_center_y = self.y_min + self.turn_radius
                self.turn_angle = -math.pi / 2.0  # Початок дуги (від -90 град)

        # 2. Поворот №1: Наліво на 90 градусів (з курсу 0 на 90)
        elif self.state == 'TURN_1':
            self.turn_angle += omega * dt
            self.current_heading = self.turn_angle + math.pi / 2.0
            self.current_x = self.turn_center_x + self.turn_radius * math.cos(self.turn_angle)
            self.current_y = self.turn_center_y + self.turn_radius * math.sin(self.turn_angle)
            if self.turn_angle >= 0.0:  # Пройшли дугу в 90 град
                self.state = 'FORWARD_Y'
                self.current_heading = math.pi / 2.0

        # 3. Пряма ділянка: Вперед по осі Y
        elif self.state == 'FORWARD_Y':
            self.current_x = self.x_max + self.turn_radius
            self.current_y += self.speed * dt
            self.current_heading = math.pi / 2.0
            if self.current_y >= self.y_max:
                self.state = 'TURN_2'
                self.turn_center_x = self.x_max
                self.turn_center_y = self.y_max
                self.turn_angle = 0.0

        # 4. Поворот №2: Наліво на 90 градусів (з курсу 90 на 180)
        elif self.state == 'TURN_2':
            self.turn_angle += omega * dt
            self.current_heading = self.turn_angle + math.pi / 2.0
            self.current_x = self.turn_center_x + self.turn_radius * math.cos(self.turn_angle)
            self.current_y = self.turn_center_y + self.turn_radius * math.sin(self.turn_angle)
            if self.turn_angle >= math.pi / 2.0:
                self.state = 'BACKWARD_X'
                self.current_heading = math.pi

        # 5. Пряма ділянка: Назад по осі X
        elif self.state == 'BACKWARD_X':
            self.current_x -= self.speed * dt
            self.current_y = self.y_max + self.turn_radius
            self.current_heading = math.pi
            if self.current_x <= self.x_min:
                self.state = 'TURN_3'
                self.turn_center_x = self.x_min
                self.turn_center_y = self.y_max
                self.turn_angle = math.pi / 2.0

        # 6. Поворот №3: Наліво на 90 градусів (з курсу 180 на 270)
        elif self.state == 'TURN_3':
            self.turn_angle += omega * dt
            self.current_heading = self.turn_angle + math.pi / 2.0
            self.current_x = self.turn_center_x + self.turn_radius * math.cos(self.turn_angle)
            self.current_y = self.turn_center_y + self.turn_radius * math.sin(self.turn_angle)
            if self.turn_angle >= math.pi:
                self.state = 'BACKWARD_Y'
                self.current_heading = 3.0 * math.pi / 2.0

        # 7. Пряма ділянка: Назад по осі Y
        elif self.state == 'BACKWARD_Y':
            self.current_y -= self.speed * dt
            self.current_x = self.x_min - self.turn_radius
            self.current_heading = 3.0 * math.pi / 2.0
            if self.current_y <= self.y_min + self.turn_radius:
                self.state = 'TURN_4'
                self.turn_center_x = self.x_min
                self.turn_center_y = self.y_min + self.turn_radius
                self.turn_angle = math.pi

        # 8. Поворот №4: Наліво на 90 градусів (з курсу 270 на 0/360)
        elif self.state == 'TURN_4':
            self.turn_angle += omega * dt
            self.current_heading = self.turn_angle + math.pi / 2.0
            self.current_x = self.turn_center_x + self.turn_radius * math.cos(self.turn_angle)
            self.current_y = self.turn_center_y + self.turn_radius * math.sin(self.turn_angle)
            if self.turn_angle >= 3.0 * math.pi / 2.0:
                self.state = 'FORWARD_X'
                self.current_heading = 0.0

        # Плавне оновлення кватерніона без ривків
        qz = math.sin(self.current_heading * 0.5)
        qw = math.cos(self.current_heading * 0.5)

        req = SetEntityState.Request()
        req.state.name = self.model_name
        req.state.reference_frame = 'world'
        req.state.pose.position.x = self.current_x
        req.state.pose.position.y = self.current_y
        req.state.pose.position.z = self.altitude
        
        req.state.pose.orientation.z = qz
        req.state.pose.orientation.w = qw
        
        req.state.twist.linear.x = self.speed * math.cos(self.current_heading)
        req.state.twist.linear.y = self.speed * math.sin(self.current_heading)
        req.state.twist.angular.z = 0.0 if ('TURN' not in self.state) else omega

        self.client.call_async(req)

def main():
    rclpy.init()
    node = TargetAnimator()
    print("Ціль переведено на патрулювання: КВАДРАТНА ТРАЄКТОРІЯ (повороти на 90 градусів).")
    try: 
        rclpy.spin(node)
    except KeyboardInterrupt: 
        pass
    finally: 
        rclpy.shutdown()

if __name__ == '__main__': 
    main()
