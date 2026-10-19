  // [ПОЧАТОК: Підключення бібліотек]


#include <rclcpp/rclcpp.hpp>


#include <std_msgs/msg/string.hpp>


#include <geometry_msgs/msg/pose_stamped.hpp>


#include <geometry_msgs/msg/twist_stamped.hpp>


#include <mavros_msgs/msg/position_target.hpp>


#include <gazebo_msgs/msg/model_states.hpp>


#include <mavros_msgs/msg/rc_out.hpp>


#include <mavros_msgs/srv/command_bool.hpp>


#include <mavros_msgs/srv/command_tol.hpp>


#include <mavros_msgs/srv/set_mode.hpp>


#include <cmath>


#include <algorithm>


#include <vision_msgs/msg/detection2_d.hpp>



using std::placeholders::_1;


using namespace std::chrono_literals;


// [КІНЕЦЬ: Підключення бібліотек]



class SimpleInterceptorNode : public rclcpp::Node {


public:


    SimpleInterceptorNode() : Node("interceptor_node"), current_state_("IDLE"), 


                              drone_x_(0), drone_y_(0), drone_z_(0), 


                              drone_vx_(0), drone_vy_(0), drone_vz_(0), 


                              drone_yaw_(0)


    {


        model_states_sub_ = this->create_subscription<gazebo_msgs::msg::ModelStates>(


            "/gazebo/model_states", 10, std::bind(&SimpleInterceptorNode::modelStatesCallback, this, _1));


            


        gcs_action_sub_ = this->create_subscription<std_msgs::msg::String>(


            "/gcs/action", 10, std::bind(&SimpleInterceptorNode::gcsActionCallback, this, _1));


            


        local_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(


            "/mavros/local_position/pose", rclcpp::SensorDataQoS(), std::bind(&SimpleInterceptorNode::poseCallback, this, _1));



        local_vel_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(


            "/mavros/local_position/velocity_local", rclcpp::SensorDataQoS(), std::bind(&SimpleInterceptorNode::velCallback, this, _1));



        rc_out_sub_ = this->create_subscription<mavros_msgs::msg::RCOut>(


            "/mavros/rc/out", rclcpp::SensorDataQoS(), std::bind(&SimpleInterceptorNode::rcOutCallback, this, _1));


            


        vision_sub_ = this->create_subscription<vision_msgs::msg::Detection2D>(


            "/vision/target_bbox", 10, std::bind(&SimpleInterceptorNode::visionCallback, this, _1));



        local_pos_pub_ = this->create_publisher<mavros_msgs::msg::PositionTarget>("/mavros/setpoint_raw/local", 10);



        arming_client_ = this->create_client<mavros_msgs::srv::CommandBool>("/mavros/cmd/arming");


        takeoff_client_ = this->create_client<mavros_msgs::srv::CommandTOL>("/mavros/cmd/takeoff");


        set_mode_client_ = this->create_client<mavros_msgs::srv::SetMode>("/mavros/set_mode");



        control_timer_ = this->create_wall_timer(100ms, std::bind(&SimpleInterceptorNode::controlLoop, this)); 


    }



private:


    std::string current_state_;


    float drone_x_, drone_y_, drone_z_, drone_vx_, drone_vy_, drone_vz_, drone_yaw_;


    float plane_x_ = 0, plane_y_ = 0, plane_z_ = 45.0, plane_vx_ = 0, plane_vy_ = 0;


    uint16_t motor_1_ = 0, motor_2_ = 0, motor_3_ = 0, motor_4_ = 0;


    


    bool target_found_ = false;


    double action_time_ = 0.0;


    double last_time_sec_ = 0.0;


    


    float smooth_vx_ = 0.0f;


    float smooth_vy_ = 0.0f;


    float smooth_vz_ = 0.0f;


    float smooth_yaw_ = 0.0f;


    double alt_warning_timer_ = 0.0;


    double dist_warning_timer_ = 0.0;



    // --- ЗМІННІ ДЛЯ КАМЕРИ ТА ЛОГЮВАННЯ ---


    double total_cam_time_ = 0.0;       // Сумарний час, коли ціль в кадрі


    double continuous_cam_time_ = 0.0;  // Безперервний час у кадрі


    double camera_lost_timer_ = 0.0;    // Таймер втрати з кадру


    bool block2_active_ = false;        // Чи активовано розрахунок Блоку 2


    bool vision_engaged_ = false;       // НОВЕ: Чи перейшов дрон на ПОВНЕ автономне управління камерою


    


    double last_vision_time_ = 0.0;     // Час останнього кадру від YOLO


    float cam_yaw_offset_ = 0.0f;       // Відхилення літака від центру кадру по горизонталі (радіани)


    float cam_pitch_offset_ = 0.0f;     // НОВЕ: Відхилення літака від центру кадру по вертикалі (радіани)


    float cam_bbox_width_ = 0.0f;


    


    float last_gazebo_dist_ = 0.0f;


    float last_cam_dist_ = 0.0f;


    float cam_plane_last_x_ = 0.0f;


    float cam_plane_last_y_ = 0.0f;


    bool first_cam_calc_ = true;


    float cam_plane_last_z_ = 45.0f;


    


    // Змінні фільтрації шумів


    float smooth_cam_dist_ = 0.0f;


    float smooth_cam_vx_ = 0.0f;


    float smooth_cam_vy_ = 0.0f;


    float smooth_cam_dist_rate_ = 0.0f;



    rclcpp::Subscription<gazebo_msgs::msg::ModelStates>::SharedPtr model_states_sub_;


    rclcpp::Subscription<vision_msgs::msg::Detection2D>::SharedPtr vision_sub_; // Підписник на YOLO


    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gcs_action_sub_;


    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr local_pose_sub_;


    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr local_vel_sub_;


    rclcpp::Subscription<mavros_msgs::msg::RCOut>::SharedPtr rc_out_sub_;


    rclcpp::Publisher<mavros_msgs::msg::PositionTarget>::SharedPtr local_pos_pub_;


    rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedPtr arming_client_;


    rclcpp::Client<mavros_msgs::srv::CommandTOL>::SharedPtr takeoff_client_;


    rclcpp::Client<mavros_msgs::srv::SetMode>::SharedPtr set_mode_client_;


    rclcpp::TimerBase::SharedPtr control_timer_;



    void rcOutCallback(const mavros_msgs::msg::RCOut::SharedPtr msg) {


        if (msg->channels.size() >= 4) {


            motor_1_ = msg->channels[0];


            motor_2_ = msg->channels[1];


            motor_3_ = msg->channels[2];


            motor_4_ = msg->channels[3];


        }


    }



    void poseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {


        drone_x_ = msg->pose.position.y;


        drone_y_ = -msg->pose.position.x;


        drone_z_ = msg->pose.position.z;



        float qx = msg->pose.orientation.x;


        float qy = msg->pose.orientation.y;


        float qz = msg->pose.orientation.z;


        float qw = msg->pose.orientation.w;


        float siny_cosp = 2.0f * (qw * qz + qx * qy);


        float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);


        drone_yaw_ = std::atan2(siny_cosp, cosy_cosp);


    }



    void velCallback(const geometry_msgs::msg::TwistStamped::SharedPtr msg) {


        drone_vx_ = msg->twist.linear.y;


        drone_vy_ = -msg->twist.linear.x;


        drone_vz_ = msg->twist.linear.z;


    }


    


    void visionCallback(const vision_msgs::msg::Detection2D::SharedPtr msg) {


        last_vision_time_ = this->now().seconds();


        


        float x_center = msg->bbox.center.position.x;


        float y_center = msg->bbox.center.position.y;


        


        // ВИПРАВЛЕНО: Додано мінус! Якщо ціль зліва (x < 320), кут має зростати (поворот вліво)


        cam_yaw_offset_ = -((x_center - 320.0f) / 640.0f) * 1.4f; 


        cam_pitch_offset_ = ((y_center - 240.0f) / 480.0f) * 1.05f; 


        


        cam_bbox_width_ = msg->bbox.size_x; 


    }


    


    void modelStatesCallback(const gazebo_msgs::msg::ModelStates::SharedPtr msg) {


        auto it = std::find(msg->name.begin(), msg->name.end(), "cessna_c172");


        if (it != msg->name.end()) {


            size_t idx = std::distance(msg->name.begin(), it);


            plane_x_ = msg->pose[idx].position.x;


            plane_y_ = msg->pose[idx].position.y;


            plane_z_ = msg->pose[idx].position.z;


            plane_vx_ = msg->twist[idx].linear.x;


            plane_vy_ = msg->twist[idx].linear.y;


            target_found_ = true; 


        } else {


            target_found_ = false;


        }


    } 



    void gcsActionCallback(const std_msgs::msg::String::SharedPtr msg) {


        if (msg->data == "TAKEOFF" && current_state_ == "IDLE") {


            auto mode_req = std::make_shared<mavros_msgs::srv::SetMode::Request>();


            mode_req->custom_mode = "GUIDED";


            set_mode_client_->async_send_request(mode_req);


            action_time_ = this->now().seconds();


            current_state_ = "WAIT_ARM";


        } 


        else if (msg->data == "ABORT" && current_state_ != "RETURN_BASE" && current_state_ != "BRAKING") {


            current_state_ = "BRAKING";


        }


    }



    void controlLoop() {


        double now_sec = this->now().seconds();


        float dt = now_sec - last_time_sec_;


        if (dt <= 0.0f || dt > 1.0f) dt = 0.1f;


        last_time_sec_ = now_sec;



        if (current_state_ == "IDLE") return;



        if (current_state_ == "WAIT_ARM" && (now_sec - action_time_ >= 1.0)) {


            auto arm_req = std::make_shared<mavros_msgs::srv::CommandBool::Request>();


            arm_req->value = true;


            arming_client_->async_send_request(arm_req);


            action_time_ = now_sec;


            current_state_ = "WAIT_TAKEOFF";


        }


        else if (current_state_ == "WAIT_TAKEOFF" && (now_sec - action_time_ >= 1.0)) {


            auto takeoff_req = std::make_shared<mavros_msgs::srv::CommandTOL::Request>();


            takeoff_req->altitude = 45.0; 


            takeoff_client_->async_send_request(takeoff_req);


            current_state_ = "TAKEOFF_PHASE";


        }


        else if (current_state_ == "TAKEOFF_PHASE") {


            if (drone_z_ >= 35.0) current_state_ = "FOLLOW";


        }


        else if (current_state_ == "FOLLOW") {


            if (!target_found_ && !vision_engaged_) return; // Якщо Gazebo втратив ціль, а камера ще не захопила



            // =========================================================


            // 1. ОНОВЛЕННЯ ТАЙМЕРІВ ТА СИСТЕМИ YOLO


            // =========================================================


            bool target_in_cam = (now_sec - last_vision_time_) < 0.3;



            if (target_in_cam) {


                total_cam_time_ += dt;


                continuous_cam_time_ += dt;


                camera_lost_timer_ = 0.0;


            } else {


                continuous_cam_time_ = 0.0; 


                if (block2_active_) camera_lost_timer_ += dt;


            }



            if (continuous_cam_time_ >= 3.0) block2_active_ = true;


            


            // ТРИГЕР АВТОНОМНОСТІ: Якщо ціль 7 секунд в кадрі - ПОВНІСТЮ ВІДКЛЮЧАЄМО GAZEBO


            if (continuous_cam_time_ >= 7.0) vision_engaged_ = true;



            // =========================================================


            // 2. РОЗРАХУНОК ОПТИЧНОЇ ДИСТАНЦІЇ ТА ФІЛЬТРАЦІЯ ШУМІВ


            // =========================================================


            float cam_plane_x = 0.0f, cam_plane_y = 0.0f, cam_plane_z = 0.0f;


            float cam_plane_speed = 0.0f, cam_dist = 0.0f, cam_dist_rate = 0.0f;



            if (block2_active_) {


                if (target_in_cam && cam_bbox_width_ > 0.0f) {


                    float focal_length_px = 380.0f; 


                    float real_plane_width = 11.0f; 


                    float raw_cam_dist = (real_plane_width * focal_length_px) / cam_bbox_width_;


                    


                    smooth_cam_dist_ = first_cam_calc_ ? raw_cam_dist : (0.15f * raw_cam_dist + 0.85f * smooth_cam_dist_);


                    cam_dist = smooth_cam_dist_;


                    


                    float target_absolute_yaw = drone_yaw_ + cam_yaw_offset_;


                    cam_plane_x = drone_x_ + cam_dist * std::sin(target_absolute_yaw);


                    cam_plane_y = drone_y_ - cam_dist * std::cos(target_absolute_yaw);


                    


                    float dz_cam = cam_dist * std::tan(cam_pitch_offset_);


                    cam_plane_z = drone_z_ - dz_cam; 


                    


                    // Скалярне диференціювання (захист від фантомних сплесків на віражах)


                    if (!first_cam_calc_) {


                        float raw_dist_rate = std::clamp((cam_dist - last_cam_dist_) / dt, -45.0f, 45.0f);


                        smooth_cam_dist_rate_ = 0.1f * raw_dist_rate + 0.9f * smooth_cam_dist_rate_;


                        cam_dist_rate = smooth_cam_dist_rate_;


                        


                        // Швидкість цілі = наша швидкість + швидкість зміни дистанції


                        float current_drone_speed = std::hypot(drone_vx_, drone_vy_);


                        float est_target_speed = current_drone_speed + smooth_cam_dist_rate_;


                        cam_plane_speed = std::clamp(est_target_speed, 0.0f, 35.0f); 


                        


                        // Вектори для логування (без впливу на кінематику)


                        float target_absolute_yaw = drone_yaw_ + cam_yaw_offset_;


                        smooth_cam_vx_ = cam_plane_speed * std::sin(target_absolute_yaw);


                        smooth_cam_vy_ = cam_plane_speed * -std::cos(target_absolute_yaw);


                    } else {


                        smooth_cam_dist_rate_ = 0.0f;


                        cam_plane_speed = 0.0f;


                        smooth_cam_vx_ = smooth_cam_vy_ = 0.0f;


                    }


                    


                    cam_plane_last_x_ = cam_plane_x;


                    cam_plane_last_y_ = cam_plane_y;


                    cam_plane_last_z_ = cam_plane_z;


                    last_cam_dist_ = cam_dist;


                    first_cam_calc_ = false;


                } else {


                    // Пам'ять інерції без гальмування


                    cam_plane_x = cam_plane_last_x_;


                    cam_plane_y = cam_plane_last_y_;


                    cam_plane_z = cam_plane_last_z_;


                    cam_dist = last_cam_dist_;


                    cam_dist_rate = smooth_cam_dist_rate_;


                }


            }



            // =========================================================


            // 3 & 4. МУЛЬТИПЛЕКСОР ТА КІНЕМАТИКА (Pure Pursuit vs Gazebo)


            // =========================================================


            float drone_speed = std::hypot(drone_vx_, drone_vy_);


            float dist_3d = std::sqrt(std::pow(plane_x_ - drone_x_, 2) + std::pow(plane_y_ - drone_y_, 2) + std::pow(plane_z_ - drone_z_, 2));



            float raw_vx = 0.0f;


            float raw_vy = 0.0f;


            float raw_vz = 0.0f;


            std::string active_source_name;



            if (vision_engaged_) {


                // --- РЕЖИМ ПОВНОЇ АВТОНОМІЇ КАМЕРИ (Синергія "Дрифту") ---


                active_source_name = "YOLO VISION (Автономно)";


                


                float err_dist = cam_dist - 40.0f; 


                float Kp_dist = 0.45f;     


                float target_forward_speed = 0.0f;



                if (err_dist > 0.0f) {


                    target_forward_speed = err_dist * Kp_dist; 


                    


                    // БУСТ НА ВИХОДІ З ВІРАЖУ:


                    // Якщо ціль почала віддалятися (швидкість віддалення > 0.5 м/с) - 


                    // дрон розуміє, що ціль вирівнялась, і дає різке пришвидшення.


                    if (smooth_cam_dist_rate_ > 0.5f) {


                        target_forward_speed += smooth_cam_dist_rate_ * 2.0f; 


                    }


                } else {


                    target_forward_speed = err_dist * 0.5f; 


                }


                


                target_forward_speed = std::clamp(target_forward_speed, -5.0f, 18.0f);


                


                float forward_cmd = 0.0f;


                float lateral_cmd = 0.0f;



                if (target_in_cam) {
                    forward_cmd = cam_plane_speed + target_forward_speed;
                    
                    // --- НОВА ЛОГІКА: Перехід у прямий політ при віддаленні цілі ---
                    // smooth_cam_dist_rate_ > 0 означає, що ціль почала віддалятися.
                    // Зробимо плавний перехід: якщо ціль віддаляється зі швидкістю 0 до 2.5 м/с,
                    // ми поступово множимо бокове ковзання на 0.
                    float escape_rate = std::max(0.0f, smooth_cam_dist_rate_);
                    
                    // drift_multiplier: 1.0 (ковзаємо на повну), 0.0 (летимо тільки прямо)
                    float drift_multiplier = std::clamp(1.0f - (escape_rate / 2.5f), 0.0f, 1.0f);
                    
                    // Застосовуємо множник до бокового руху
                    lateral_cmd = (cam_yaw_offset_ * 22.0f) * drift_multiplier; 
                    
                    // Базове "гальмування" вектора вперед під час нормального віражу
                    float base_turn_attenuation = std::max(0.4f, std::cos(cam_yaw_offset_));
                    
                    // Якщо ціль тікає (drift_multiplier -> 0), ми плавно прибираємо 
                    // гальмування віражу і повертаємо 100% (1.0f) тяги вперед
                    float final_attenuation = 1.0f - (drift_multiplier * (1.0f - base_turn_attenuation));
                    
                    forward_cmd *= final_attenuation; 
                } else {
                    forward_cmd = cam_plane_speed; 
                    lateral_cmd = 0.0f; 
                }



                forward_cmd = std::clamp(forward_cmd, 0.0f, 32.0f);


                lateral_cmd = std::clamp(lateral_cmd, -15.0f, 15.0f);



                // Переводимо локальні команди (Вперед/Вбік) у глобальні осі X/Y


                raw_vx = forward_cmd * std::sin(drone_yaw_) + lateral_cmd * std::cos(drone_yaw_);


                raw_vy = forward_cmd * -std::cos(drone_yaw_) + lateral_cmd * std::sin(drone_yaw_);



            } else {


                // --- РЕЖИМ GAZEBO ---


                active_source_name = "GAZEBO GROUND TRUTH";


                


                float dx = plane_x_ - drone_x_;


                float dy = plane_y_ - drone_y_;


                float dist_xy = std::hypot(dx, dy); 


                float dir_x = (dist_xy > 0.1f) ? dx / dist_xy : 0.0f;


                float dir_y = (dist_xy > 0.1f) ? dy / dist_xy : 0.0f;



                float err_dist = dist_xy - 40.0f; 


                float a_brake_xy = 0.6f; 


                float Kp_xy = 0.35f;     


                float target_catch_speed = 0.0f;



                if (err_dist > 0.0f) {


                    float kin_vel = std::sqrt(2.0f * a_brake_xy * err_dist);


                    target_catch_speed = std::min(kin_vel, err_dist * Kp_xy); 


                } else {


                    target_catch_speed = err_dist * 0.4f; 


                }



                target_catch_speed = std::clamp(target_catch_speed, -4.0f, 10.0f);


                raw_vx = plane_vx_ + dir_x * target_catch_speed;


                raw_vy = plane_vy_ + dir_y * target_catch_speed;



                float cross_err = dist_xy - 40.0f; 


                if (std::abs(cross_err) > 6.0f) {


                    raw_vx += dir_x * (cross_err * 0.15f);


                    raw_vy += dir_y * (cross_err * 0.15f);


                }


            }



            // =========================================================


            // 5. ПРОФЕСІЙНА КІНЕМАТИКА ВИСОТИ (Ось Z)


            // =========================================================


            float dz_error = 45.0f - drone_z_; 


            float a_brake_z = 0.5f;      


            float Kp_z = 0.4f;



            if (drone_z_ > 65.0f) {


                float kin_vz = -std::sqrt(2.0f * a_brake_z * std::abs(dz_error));


                float p_vz = dz_error * Kp_z;


                raw_vz = std::max(kin_vz, p_vz); 


                alt_warning_timer_ = 0.0;


                


                float current_h_speed = std::hypot(raw_vx, raw_vy);


                if (current_h_speed > 12.0f) {


                    raw_vx = (raw_vx / current_h_speed) * 12.0f;


                    raw_vy = (raw_vy / current_h_speed) * 12.0f;


                }


            } else if (drone_z_ < 25.0f) {


                float kin_vz = std::sqrt(2.0f * a_brake_z * std::abs(dz_error));


                float p_vz = dz_error * Kp_z;


                raw_vz = std::min(kin_vz, p_vz);


                alt_warning_timer_ = 0.0;


            } else {


                if (drone_z_ >= 55.0f && drone_z_ <= 65.0f) {


                    alt_warning_timer_ += dt;


                    if (alt_warning_timer_ > 7.0) {


                        float kin_vz = -std::sqrt(2.0f * a_brake_z * std::abs(dz_error));


                        float p_vz = dz_error * Kp_z;


                        raw_vz = std::max(kin_vz, p_vz);


                    }


                } else {


                    alt_warning_timer_ = 0.0;


                }


            }



            raw_vz = std::clamp(raw_vz, -4.0f, 4.0f);



            // =========================================================


            // 6. ПЛАВНІСТЬ І СИНЕРГІЯ МОТОРІВ


            // =========================================================


            // Менше прискорення для ефекту плавного "ковзання" боком


            float max_accel = vision_engaged_ ? 2.5f : 3.5f; 


            smooth_vx_ += std::clamp(raw_vx - smooth_vx_, -max_accel * dt, max_accel * dt);


            smooth_vy_ += std::clamp(raw_vy - smooth_vy_, -max_accel * dt, max_accel * dt);


            smooth_vz_ += std::clamp(raw_vz - smooth_vz_, -max_accel * dt, max_accel * dt);



            float target_yaw;


            if (vision_engaged_) {


                if (target_in_cam) {


                    // Камера: ніс завжди жорстко прив'язаний до кута рамки


                    target_yaw = drone_yaw_ + cam_yaw_offset_;


                } else {


                    target_yaw = smooth_yaw_;


                }


            } else {


                float dx_yaw = plane_x_ - drone_x_;


                float dy_yaw = plane_y_ - drone_y_;


                target_yaw = std::atan2(dx_yaw, -dy_yaw);


            }



            float yaw_err = target_yaw - smooth_yaw_;


            while (yaw_err > M_PI) yaw_err -= 2.0f * M_PI;


            while (yaw_err < -M_PI) yaw_err += 2.0f * M_PI;


            


            // Гіперагресивний P-регулятор: камера має миттєво реагувати на віраж,


            // але плавно сповільнюватися в центрі (завдяки пропорційності yaw_err)


            float yaw_p_gain = vision_engaged_ ? 4.0f : 2.5f;   


            float max_yaw_rate = vision_engaged_ ? 2.0f : 1.0f; // До 115 град/сек 


            


            float yaw_step = std::clamp(yaw_err * yaw_p_gain, -max_yaw_rate, max_yaw_rate) * dt;


            smooth_yaw_ += yaw_step;



            mavros_msgs::msg::PositionTarget pt;


            pt.coordinate_frame = mavros_msgs::msg::PositionTarget::FRAME_LOCAL_NED;


            pt.type_mask = 2503; 


                           


            pt.velocity.x = -smooth_vy_;


            pt.velocity.y = smooth_vx_;


            pt.velocity.z = -smooth_vz_; 


            pt.yaw = smooth_yaw_;



            // =========================================================


            // 7. ФОРМУВАННЯ ЛОГУ


            // =========================================================


            float plane_speed = std::hypot(plane_vx_, plane_vy_);


            float actual_yaw_deg = drone_yaw_ * 180.0f / M_PI;


            


            float gazebo_dist_rate = (dist_3d - last_gazebo_dist_) / dt;


            last_gazebo_dist_ = dist_3d;



            std::string cam_status = target_in_cam ? "АКТИВНО (В кадрі)" : "ПОЗА КАДРОМ";


            std::string tracking_status = target_in_cam ? "Спостерігається" : "Ціль втрачено";


            


            char log_buffer[4096];


            int offset = snprintf(log_buffer, sizeof(log_buffer),


                "\n=======================================================\n"


                "[УНІВЕРСАЛЬНІ ДАНІ]\n"


                "  Джерело Управління : %s\n"


                "  Статус роботи      : %s\n"


                "  Статус камери YOLO : %s\n"


                "  Спостереження      : %s\n"


                "  Ціль у кадрі (сум) : %.1f сек\n",


                active_source_name.c_str(), current_state_.c_str(), cam_status.c_str(), tracking_status.c_str(), total_cam_time_);



            if (block2_active_ && !target_in_cam) {


                offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,


                "  Ціль ВТРАЧЕНО      : %.1f сек\n", camera_lost_timer_);


            }



            offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,


                "  Мотори (PWM)       : [1]:%4d [2]:%4d [3]:%4d [4]:%4d\n"


                "-------------------------------------------------------\n"


                "[БЛОК 1: ДАНІ НАПРЯМУ З GAZEBO]\n"


                "  Координати цілі    : X:%6.1f Y:%6.1f Z:%5.1f\n"


                "  Координати дрона   : X:%6.1f Y:%6.1f Z:%5.1f\n"


                "  Курс дрона (Yaw)   : %6.1f°\n"


                "  Швидкість цілі     : %6.1f м/с\n"


                "  Швидкість дрона    : %6.1f м/с\n"


                "  Дистанція до цілі  : %6.1f м\n"


                "  Зміна дистанції    : %6.1f м/с\n",


                motor_1_, motor_2_, motor_3_, motor_4_,


                plane_x_, plane_y_, plane_z_,


                drone_x_, drone_y_, drone_z_,


                actual_yaw_deg,


                plane_speed,


                drone_speed,


                std::sqrt(std::pow(plane_x_-drone_x_,2) + std::pow(plane_y_-drone_y_,2) + std::pow(plane_z_-drone_z_,2)),


                gazebo_dist_rate);



            if (block2_active_) {


                offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,


                "-------------------------------------------------------\n"


                "[БЛОК 2: РОЗРАХУНОК ПО ВЛАСНІЙ КАМЕРІ ТА GPS]\n"


                "  Координати цілі    : X:%6.1f Y:%6.1f Z:%5.1f\n"


                "  Координати дрона   : X:%6.1f Y:%6.1f Z:%5.1f\n"


                "  Курс дрона (Yaw)   : %6.1f°\n"


                "  Швидкість цілі     : %6.1f м/с\n"


                "  Швидкість дрона    : %6.1f м/с\n"


                "  Дистанція до цілі  : %6.1f м\n"


                "  Зміна дистанції    : %6.1f м/с\n",


                cam_plane_x, cam_plane_y, cam_plane_z,


                drone_x_, drone_y_, drone_z_,


                actual_yaw_deg,


                cam_plane_speed,


                drone_speed,


                cam_dist,


                cam_dist_rate);


            }



            snprintf(log_buffer + offset, sizeof(log_buffer) - offset, "=======================================================");


            RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 500, "%s", log_buffer);



            local_pos_pub_->publish(pt);


        }


    }


};



int main(int argc, char **argv) {


    rclcpp::init(argc, argv);


    rclcpp::spin(std::make_shared<SimpleInterceptorNode>());


    rclcpp::shutdown();


    return 0;


}   
