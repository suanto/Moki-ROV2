/*

 Copyright (C) 2017 Eric van Dijken <eric@team-moki.nl>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*/

#include "main.h"

//
// Serial vars
//
String serial_command = "";
String command = "";
String value_string = "";
int value = 0;
boolean command_complete = false;

//
// Tiemout vars
//
int sensor_time = 0;
int motor_time = 0;
int ping_time = 0;
int pong_time = 0;

//
// PID vars
//
double hover_Setpoint, hover_Input, hover_Output;
double heading_Setpoint, heading_Input, heading_Output;

//Define the aggressive and conservative Tuning Parameters
//double hover_aggKp=4, hover_aggKi=0.2, hover_aggKd=1;
double hover_aggKp=2, hover_aggKi=0.1, hover_aggKd=.5;

// Depth -5 +2  Gap: 1 - 3
//double hover_consKp=1, hover_consKi=0.05, hover_consKd=0.25;

// Depth -2 +1  Gap: 0 - 1.7 
//double hover_consKp=2, hover_consKi=0.05, hover_consKd=0.25;

double hover_consKp=4, hover_consKi=0.05, hover_consKd=0.25;



double heading_aggKp=4, heading_aggKi=0.2, heading_aggKd=1;
double heading_consKp=4, heading_consKi=0.05, heading_consKd=0.25;

//Specify the links and initial tuning parameters
PID hover_PID(&hover_Input, &hover_Output, &hover_Setpoint, hover_consKp, hover_consKi, hover_consKd, DIRECT);
PID heading_PID(&heading_Input, &heading_Output, &heading_Setpoint, heading_consKp, heading_consKi, heading_consKd, DIRECT);


//
// Pilot vars
//
int pilot = 0;
int pilot_heading = 0;
//
// Hover vars
//
int hover = 0;
float hover_depth = 0;

//
// Motor vars
//
int power = 0;
struct motor_power {
    float m1;
    float m2;
    float m3;
    float m4;
    float m5;
    float m6;
};

void pid_setup()
{
    hover_PID.SetMode(AUTOMATIC);
    hover_PID.SetOutputLimits(0, 30);
    heading_PID.SetMode(AUTOMATIC);
    heading_PID.SetOutputLimits(0, 20);
}

void setup()
{
    unsigned int timeout = millis();

    motor_setup();
    light_setup();
    camera_setup();
    sensor_setup();
    pid_setup();

    // Init serial communication
    serial_command.reserve(200);
    Serial.begin(115200);

    // Wait for Serial port to be available
    while ((timeout <= millis() + 10000)) {
        if (Serial) {
            Serial.println("ARDUINO READY");
            timeout = -1;
            ping_time = millis();
            pong_time = millis();
        } else {
            timeout = millis();
        }
    }
    //  if (timeout > 0) {
    //  }

}

void sensor_loop()
{
    Serial.print("Time:");
    sensor_time = millis();
    Serial.println(sensor_time);

    Serial.print("Volt:");
    Serial.println(current_get(),4);

    Serial.print("Amps:");
    Serial.println(amp_get(),4);

    depth_loop();
    Serial.print("Pressure:");
    Serial.println(pressure_get(), 4);
    Serial.print("Temp_OUT:");
    Serial.println(temp_get());
    Serial.print("Depth:");
    Serial.println(depth_get(), 4);
    Serial.print("Altitude:");
    Serial.println(alt_get());

    imu_loop();
    Serial.print("X:");
    Serial.println(imu_X(), 4);
    Serial.print("Y:");
    Serial.println(imu_Y(), 4);
    Serial.print("Z:");
    Serial.println(imu_Z(), 4);
    Serial.print("ACCL_X:");
    Serial.println(imu_accl_X(), 4);
    Serial.print("ACCL_Y:");
    Serial.println(imu_accl_Y(), 4);
    Serial.print("ACCL_Z:");
    Serial.println(imu_accl_Z(), 4);
    Serial.print("Heading:");
    Serial.println(imu_heading(), 4);
    Serial.print("Roll:");
    Serial.println(imu_roll(), 4);
    Serial.print("Pitch:");
    Serial.println(imu_pitch(), 4);
    Serial.print("Temp_IN:");
    Serial.println(imu_temp_get());
    Serial.print("Sys:");
    Serial.println(imu_system());
    Serial.print("Gyro:");
    Serial.println(imu_gyro());
    Serial.print("Accel:");
    Serial.println(imu_accel());
    Serial.print("Mag:");
    Serial.println(imu_mag());
}

void hover_loop()
{
    //
    // Hover
    //
    hover_Input = depth_get();
//    Serial.print("DEBUG_HOVER_DEPTH:");
//    Serial.println(hover_Input);
    hover_Setpoint = hover_depth;
//    Serial.print("DEBUG_HOVER_SET  :");
//    Serial.println(hover_depth);

    double gap = abs(hover_Setpoint - hover_Input);

//    Serial.print("DEBUG_HOVER_GAP:");
//    Serial.println(gap);

    if (gap < 10) {
        hover_PID.SetTunings(hover_consKp, hover_consKi, hover_consKd);
    } else {
        hover_PID.SetTunings(hover_aggKp, hover_aggKi, hover_aggKd);
    }

    hover_PID.Compute();

//    Serial.print("DEBUG_HOVER_OUTPUT:");
//    Serial.println(hover_Output);

    if (depth_get() > hover_depth) {
//        Serial.println("DEBUG_HOVER_UP:");
        motor_up(hover_Output);
        Serial.print("Motor_5:");
        Serial.println(hover_Output);
        Serial.print("Motor_6:");
        Serial.println(-hover_Output);
    } else if (depth_get() < hover_depth) {
//        Serial.println("DEBUG_HOVER_DIVE:");
        motor_dive(hover_Output);
        Serial.print("Motor_5:");
        Serial.println(-hover_Output);
        Serial.print("Motor_6:");
        Serial.println(hover_Output);
    } else {
        motor_stop();
        hover_Output = 0;
        Serial.print("Motor_5:");
        Serial.println(hover_Output);
        Serial.print("Motor_6:");
        Serial.println(hover_Output);
    }
}

void pilot_loop() {
    //
    // Pilot
    //
    int direction = 0;

    heading_Input = imu_heading() + 360;
    heading_Setpoint = pilot_heading + 360;

    double gap = abs(heading_Setpoint - heading_Input);

    Serial.print("DEBUG_PILOT_GAP:");
    Serial.println(gap);

    if (heading_Input > heading_Setpoint) {
        direction = 2;
        heading_Input = pilot_heading + 360;
        heading_Setpoint = imu_heading() + 360;
    }
    if (heading_Input < heading_Setpoint) {
        direction = 1;
    }


//        if (gap < 10) {
            heading_PID.SetTunings(heading_consKp, heading_consKi, heading_consKd);
//        } else {
//            heading_PID.SetTunings(heading_aggKp, heading_aggKi, heading_aggKd);
//        }

        heading_PID.Compute();

            Serial.print("DEBUG_PILOT_HEADING_INPUT:");
            Serial.println(heading_Input);
            Serial.print("DEBUG_PILOT_HEADING_OUTPUT:");
            Serial.println(heading_Output);
            Serial.print("DEBUG_PILOT_HEADING_SETPUT:");
            Serial.println(heading_Setpoint);

        if (heading_Output > 30) {
            //    Serial.print("DEBUG_PILOT_HEADING_OUTPUT_MAX:");
            //    Serial.println(heading_Output);
            heading_Output = 0;
        };

    if (direction == 2) {
            Serial.print("DEBUG_PILOT_HEADING_OUTPUT_LEFT:");
            Serial.println(heading_Output);
            motor_left(heading_Output);
            Serial.print("Motor_1:");
            Serial.println(-heading_Output);
            Serial.print("Motor_2:");
            Serial.println(heading_Output);
            Serial.print("Motor_3:");
            Serial.println(-heading_Output);
            Serial.print("Motor_4:");
            Serial.println(heading_Output);

    } else if (direction == 1) {
            Serial.print("DEBUG_PILOT_HEADING_OUTPUT_RIGHT:");
            Serial.println(heading_Output);
            motor_right(heading_Output);
            Serial.print("Motor_1:");
            Serial.println(heading_Output);
            Serial.print("Motor_2:");
            Serial.println(-heading_Output);
            Serial.print("Motor_3:");
            Serial.println(heading_Output);
            Serial.print("Motor_4:");
            Serial.println(-heading_Output);
        } else {
            motor_stop();
            heading_Output = 0;
            Serial.print("Motor_1:");
            Serial.println(heading_Output);
            Serial.print("Motor_2:");
            Serial.println(heading_Output);
            Serial.print("Motor_3:");
            Serial.println(heading_Output);
            Serial.print("Motor_4:");
            Serial.println(heading_Output);
        }
}

void motor_loop()
{
    motor_time = millis();
    //  motor_stop();
}

void pingpong_loop()
{
    pong_time = millis();
    Serial.println("PONG:0");
    if (pong_time >= (ping_time + 2500)) {
        motor_stop();
        Serial.println("TIMEOUT:1");
    }
}

void loop()
{
    //
    // Run at "fixed" times
    //
    int time = millis();
    //
    // Every 500 μs run the sensor loop
    //
    if ((time >= (sensor_time + 500)) || (time < sensor_time)) {
        sensor_loop();
    }
    //
    // Every 2500 μs run the ping loop
    //
    if (time >= (pong_time + 2500)) {
        pingpong_loop();
    }
    //
    // Every 150 μs run the motor loop.
    //
    if ((time >= (motor_time + 150)) || (time < motor_time)) {
        if (pilot == true) {
            pilot_loop();
        }
        if (hover == true) {
            hover_loop();
        }
        motor_loop();
    }

    //
    // Command
    //
    if (command_complete) {
        serial_command.trim();
        int pos = serial_command.indexOf(':');
        command = serial_command.substring(0, pos);
        value_string = serial_command.substring(pos + 1);
        value = value_string.toInt();

        if (command != "PING") {
            Serial.print("Command: ");
            Serial.print(command);
            Serial.print(" Value: ");
            Serial.println(value);
        }

        if (command == "ARM") {
            motor_arm(true);
        } else if (command == "PING") {
            ping_time = millis();
        } else if (command == "DISARM") {
            motor_stop();
            light_off();
            power = 0;
            Serial.print("Power:");
            Serial.println(power);
            motor_arm(false);

        } else if (command == "Stop") {
            motor_stop();
            Serial.print("Motor_1:");
            Serial.println(0);
            Serial.print("Motor_2:");
            Serial.println(0);
            Serial.print("Motor_3:");
            Serial.println(0);
            Serial.print("Motor_4:");
            Serial.println(0);
            Serial.print("Motor_5:");
            Serial.println(0);
            Serial.print("Motor_6:");
            Serial.println(0);

        } else if (command == "Pilot") {
            if (value >= 400) {
                pilot = false;
                motor_stop();
            } else {
                pilot = true;
                pilot_heading = value;
            }
        } else if (command == "Hover") {
            if (value >= 0) {
                hover = true;
                hover_depth = value;
            } else {
                hover = false;
                motor_stop();
            }
        } else if (command == "Power") {
            power = value;

            // Right Left Reverse Forward Strafe_r Strafe_l Dive Up
        } else if (command == "Forward") {
            motor_forward(power);
            //      Serial.println("Debug_Forward");
            Serial.print("Motor_1:");
            Serial.println(-power);
            Serial.print("Motor_2:");
            Serial.println(power);
            Serial.print("Motor_3:");
            Serial.println(power);
            Serial.print("Motor_4:");
            Serial.println(-power);

        } else if (command == "Reverse") {
            motor_reverse(power);
            //      Serial.println("Debug_Reverse");
            Serial.print("Motor_1:");
            Serial.println(power);
            Serial.print("Motor_2:");
            Serial.println(-power);
            Serial.print("Motor_3:");
            Serial.println(-power);
            Serial.print("Motor_4:");
            Serial.println(power);

        } else if (command == "Right") {
            motor_right(power);
            Serial.print("Motor_1:");
            Serial.println(power);
            Serial.print("Motor_2:");
            Serial.println(-power);
            Serial.print("Motor_3:");
            Serial.println(power);
            Serial.print("Motor_4:");
            Serial.println(-power);

        } else if (command == "Left") {
            motor_left(power);
            Serial.print("Motor_1:");
            Serial.println(-power);
            Serial.print("Motor_2:");
            Serial.println(power);
            Serial.print("Motor_3:");
            Serial.println(-power);
            Serial.print("Motor_4:");
            Serial.println(power);

        } else if (command == "Dive") {
            motor_dive(power);
            Serial.print("Motor_5:");
            Serial.println(power);
            Serial.print("Motor_6:");
            Serial.println(-power);

        } else if (command == "Up") {
            motor_up(power);
            Serial.print("Motor_5:");
            Serial.println(-power);
            Serial.print("Motor_6:");
            Serial.println(power);

        } else if (command == "Strafe_r") {
            motor_strafe_right(power);
            Serial.print("Motor_1:");
            Serial.println(power);
            Serial.print("Motor_2:");
            Serial.println(power);
            Serial.print("Motor_3:");
            Serial.println(power);
            Serial.print("Motor_4:");
            Serial.println(power);

        } else if (command == "Strafe_l") {
            motor_strafe_left(power);
            Serial.print("Motor_1:");
            Serial.println(-power);
            Serial.print("Motor_2:");
            Serial.println(-power);
            Serial.print("Motor_3:");
            Serial.println(-power);
            Serial.print("Motor_4:");
            Serial.println(-power);

        } else if (command == "Roll_r") {
            motor_roll_right(power);
            Serial.print("Motor_5:");
            Serial.println(-power);
            Serial.print("Motor_6:");
            Serial.println(power);

        } else if (command == "Roll_l") {
            motor_roll_left(power);
            Serial.print("Motor_5:");
            Serial.println(power);
            Serial.print("Motor_6:");
            Serial.println(-power);

        } else if (command == "Light1") {
            if (value == LIGHT1_OFF) {
                light_off();
            } else {
                light_on();
            }
        } else if (command == "Light2") {
            if (value == LIGHT1_OFF) {
                light_off();
            } else {
                light_on();
            }
        } else if (command == "Camx") {
            camera_movex(value);
        } else if (command == "Camy") {
            camera_movey(value);
        }

        serial_command = "";
        command_complete = false;
    }
}

void serialEvent()
{
    while (Serial.available()) {
        // get the new byte:
        char inChar = (char) Serial.read();
        // add it to the inputString:
        serial_command += inChar;
        // if the incoming character is a newline, set a flag
        // so the main loop can do something about it:
        if (inChar == '\n') {
            command_complete = true;
        }
    }
}
