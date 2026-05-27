#ifndef Relay_h
#define Relay_h

#include "Arduino.h"

class Relay {
  public:
    // 构造函数：指定继电器控制引脚
    Relay(int pin);
    
    // 构造函数：指定引脚和初始状态
    Relay(int pin, bool initialState);
    
    // 初始化继电器
    void begin();
    
    // 打开继电器（吸合）
    void on();
    
    // 关闭继电器（释放）
    void off();
    
    // 切换继电器状态
    void toggle();
    
    // 获取当前继电器状态
    bool getState();
    
    // 设置继电器状态
    void setState(bool state);
    
  private:
    int _pin;           // 继电器控制引脚
    bool _state;        // 当前状态：true为打开，false为关闭
    bool _activeHigh;   // 是否为高电平触发（默认高电平触发）
};

#endif