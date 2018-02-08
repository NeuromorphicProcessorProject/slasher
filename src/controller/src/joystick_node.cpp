#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ros/ros.h>
#include <sensor_msgs/Joy.h>      // Joy msg for Jetson JoyStick to subscribe signals from joystick.
#include <rally_msgs/Pwm.h>


/*
 * Jetson Joy Stick Object:
 *
 * Translate LogiTech F710 Joystick signals into commands for controlling Jetson Car.
 *
 * Tutorials:
 *      - How to validate joystick working on Linux (http://wiki.ros.org/joy/Tutorials/ConfiguringALinuxJoystick)
 *      - How to create simple Joystick TeleOp on ROS (http://wiki.ros.org/joy/Tutorials/WritingTeleopNode)
 *
 * Dependencies:
 *        roscpp, joy, std_msgs
 *
 * Parameters:
 *   Linear_:  axes  of joy stick to control the ESC speed
 *   Angular_: axes of joy stick to control the steering servo
 */
#define START_BUTTON  6
#define STEERING_JOY  0
#define THROTTLE_JOY  3


class JoyStick{
    public:
        // Default Constructor
        JoyStick();
    private:
        enum {OFF, ON};
        ros::NodeHandle node_handle_;
        ros::Publisher  car_pub_;
        ros::Subscriber joy_sub_;
        int             linear_;
        int             angular_;
        double          l_scale_;
        double          a_scale_;
        float           previous[2];
    
        bool            emergency_brake;
        bool            running_autonomous;
        bool            is_recording;

        void            joyCallback(const sensor_msgs::Joy::ConstPtr& joy);
        void            _manual_mode(const sensor_msgs::Joy::ConstPtr& joy);
        void            _autonomous_mode(const sensor_msgs::Joy::ConstPtr& joy);

};

JoyStick::JoyStick():
        linear_(THROTTLE_JOY) ,
        angular_(STEERING_JOY)
{
    a_scale_ = 0.9;
    l_scale_ = 0.3;
    emergency_brake = 1;

    //Initialize values
    running_autonomous = 0;
    is_recording       = 0;
     
    //Set up parameters
    node_handle_.param("axis_linear", linear_, linear_);
    node_handle_.param("axis_angular",angular_, angular_);
    node_handle_.param("scale_angular", a_scale_, a_scale_);
    node_handle_.param("scale_linear", l_scale_, l_scale_);

    // Setup Ros Publisher and Subscriber
    car_pub_ = node_handle_.advertise<rally_msgs::Pwm>("drive_pwm", 1, true);
    joy_sub_ = node_handle_.subscribe<sensor_msgs::Joy>("joy", 10, &JoyStick::joyCallback, this);
}

void JoyStick::joyCallback(const sensor_msgs::Joy::ConstPtr &joy)
{
    // Determine if car is in manual or autonomous mode
    if (joy->buttons[7] == ON){
        if(running_autonomous==OFF)
            running_autonomous=ON;
    }
    if (joy->buttons[6] ==ON){
        if(running_autonomous==ON)
            running_autonomous =OFF;
    }

    if (running_autonomous==OFF)
        _manual_mode(joy);
    else
        _autonomous_mode(joy);

}
void JoyStick::_autonomous_mode(const sensor_msgs::Joy::ConstPtr &joy){
    // don't implement
}

void JoyStick::_manual_mode(const sensor_msgs::Joy::ConstPtr& joy){
    rally_msgs::Pwm car;
    // EMERGENCY BRAKE
    if (joy->buttons[4] == ON || joy->buttons[5] == ON){
        emergency_brake = ON;
        car.throttle = 0;
        car.steering = 0;
        ROS_INFO("Emergency Brake");
    }
    // START THE CAR
    if (joy->buttons[START_BUTTON] == ON){
        emergency_brake = OFF;
        ROS_INFO("Manual Mode");
    }
    if (emergency_brake == OFF){
        car.steering = a_scale_*joy->axes[angular_];
        car.throttle = l_scale_*joy->axes[linear_];
    }

    car_pub_.publish(car);
    previous[0] =  car.steering;
    previous[1] =  car.throttle;
}

int main(int argc, char** argv) 
{
    ros::init(argc, argv, "jetson_teleop");
    JoyStick jetson_teleop;
    ROS_INFO("Jetson Joystick Node activated");
    ros::spin();
}
