#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "std_srvs/srv/set_bool.hpp"

class GroundControlStation : public rclcpp::Node {
public:
    GroundControlStation() : Node("gcs_node") {
        // Канал безперервної передачі координат цілі
        target_pub_ = this->create_publisher<geometry_msgs::msg::Point>("/gcs/interception_point", 10);
        
        // Сервіс для підтвердження команди на знищення
        strike_srv_ = this->create_service<std_srvs::srv::SetBool>(
            "/gcs/authorize_strike",
            std::bind(&GroundControlStation::strike_callback, this, std::placeholders::_1, std::placeholders::_2)
        );

        // Таймер: оновлення координат кожні 500 мілісекунд (2 Гц)
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(500),
            std::bind(&GroundControlStation::publish_coordinates, this)
        );

        RCLCPP_INFO(this->get_logger(), "Наземну станцію розгорнуто. Початок безперервної трансляції координат радара.");
    }

private:
    void publish_coordinates() {
        auto msg = geometry_msgs::msg::Point();
        // У майбутньому ці змінні оновлюватимуться з реальних розрахунків або GUI.
        // Зараз задаємо точку в просторі, куди має вилетіти перехоплювач.
        msg.x = 100.0; 
        msg.y = 50.0;
        msg.z = 20.0;
        
        target_pub_->publish(msg);
    }

    void strike_callback(const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
                         std::shared_ptr<std_srvs::srv::SetBool::Response> response) {
        RCLCPP_WARN(this->get_logger(), "[GCS ALERT] ОТРИМАНО ЗАПИТ НА ТАРАН ВІД ПЕРЕХОПЛЮВАЧА!");
        
        // Масштабоване місце для логіки перевірки "свій-чужий" або дії оператора.
        response->success = true;
        response->message = "ДОЗВІЛ НА ЗНИЩЕННЯ ПІДТВЕРДЖЕНО";
        
        RCLCPP_INFO(this->get_logger(), "[GCS COMMAND] Дозвіл на таран надіслано.");
    }

    rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr target_pub_;
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr strike_srv_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<GroundControlStation>());
    rclcpp::shutdown();
    return 0;
}