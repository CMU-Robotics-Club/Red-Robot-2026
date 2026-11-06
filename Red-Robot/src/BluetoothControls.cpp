#include <RedRobot.h>
// TODO: Comment out the controller you use!
#define USE_BLUEFRUIT 1
// #define USE_APP2 1
// #define BLUETOOTH_CONTROLLER APP3 1
// #define BLUETOOTH_CONTROLLER APP4 1

extern bool in_remote_control;

namespace Bluefruit {
  /* DPad_Pressed is called everytime a D-Pad button is pressed or released.
   *
   * Only activates after RedRobot::StartRemoteControl() is called.
   */
  void DPad_Pressed(button_t Button, bool buttonPressedDown)
  {
    if (!in_remote_control)
      return;

    // values stay the same between calls to DPad_Pressed
    static bool button_pressed[4] = {false, false, false, false};

    button_pressed[Button] = (buttonPressedDown == true);

    // TODO: edit your speed output to the yellow motors below!
    // stop = 0, slow = 50, fast = 100
    // range [-100,100]

    int left_speed = 0;
    int right_speed = 0;

    if (button_pressed[UP])
    { // Forward
      left_speed += 100;
      right_speed += 100;
    }
    if (button_pressed[DOWN])
    { // Backward
      left_speed -= 100;
      right_speed -= 100;
    }
    if (button_pressed[LEFT])
    { // Turn left_speed
      left_speed -= 50;
      right_speed += 50;
    }
    if (button_pressed[RIGHT])
    { // Turn right_speed
      left_speed += 50;
      right_speed -= 50;
    }

    // Clamp and write speeds
    RedRobot::WriteDCMotor(2, constrain(left_speed, -100, 100));
    RedRobot::WriteDCMotor(3, constrain(right_speed, -100, 100));
  }

  void Button1_Pressed(bool buttonPressedDown)
  {
  //  if (buttonPressedDown)
  //    RedRobot::StartRemoteControl();
  }

  void Button2_Pressed(bool buttonPressedDown)
  {
    Serial.printf("Entered here\n");
    RedRobot::SetServo(0, 180);
    delay(10);
    RedRobot::SetServo(1, 180);
    delay(10);
    RedRobot::SetServo(2, 180);
    delay(10);
    RedRobot::SetServo(3, 180);
    delay(1000);
    RedRobot::SetServo(0, 0);
    RedRobot::SetServo(1, 0);
    RedRobot::SetServo(2, 0);
    RedRobot::SetServo(3, 0);
  }

  void Button3_Pressed(bool buttonPressedDown)
  {
  }

  void Button4_Pressed(bool buttonPressedDown)
  {
  }
}

// namespace AIBLuetoothApp2
// {
//   void Joystick1(uint8_t angle, uint8_t magnitude)
//   {
//   }
// 
//   void Joystick2(uint8_t angle, uint8_t magnitude)
//   {
//   }
// 
//   /* DPad_Pressed is called everytime a D-Pad button is pressed or released.
//    *
//    * Only activates after RedRobot::StartRemoteControl() is called.
//    * Button is one of {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3}
//    */
//   void DPad_Pressed(button_t Button, bool buttonPressedDown)
//   {
//     // values stay the same between calls to DPad_Pressed
//     static bool button_pressed[4] = {false, false, false, false};
// 
//     button_pressed[Button] = (buttonPressedDown == true);
// 
//     // TODO: edit your speed output to the yellow motors below!
//     // stop = 0, slow = 50, fast = 100
//     // range [-100,100]
// 
//     int left_speed = 0;
//     int right_speed = 0;
// 
//     if (button_pressed[UP])
//     { // Forward
//       left_speed += 100;
//       right_speed += 100;
//     }
//     if (button_pressed[DOWN])
//     { // Backward
//       left_speed -= 100;
//       right_speed -= 100;
//     }
//     if (button_pressed[LEFT])
//     { // Turn left_speed
//       left_speed -= 50;
//       right_speed += 50;
//     }
//     if (button_pressed[RIGHT])
//     { // Turn right_speed
//       left_speed += 50;
//       right_speed -= 50;
//     }
// 
//     // Clamp and write speeds
//     RedRobot::WriteDCMotor(2, constrain(left_speed, -100, 100));
//     RedRobot::WriteDCMotor(3, constrain(right_speed, -100, 100));
//   }
// 
//   void A_Pressed(bool buttonPressedDown)
//   {
//     if (buttonPressedDown)
//       RedRobot::StartRemoteControl();
//   }
// 
//   void B_Pressed(bool buttonPressedDown)
//   {
//   }
// 
//   void C_Pressed(bool buttonPressedDown)
//   {
//   }
// 
//   void D_Pressed(bool buttonPressedDown)
//   {
//   }
// }