#include "relay.h"

// 构造函数：默认初始状态为关闭
Relay::Relay(int pin) {
  _pin = pin;
  _state = false;
  _activeHigh = true;  // 默认高电平触发
}

// 构造函数：指定初始状态
Relay::Relay(int pin, bool initialState) {
  _pin = pin;
  _state = initialState;
  _activeHigh = true;
}

// 初始化继电器引脚
void Relay::begin() {
  pinMode(_pin, OUTPUT);
  setState(_state);  // 设置初始状态
}

// 打开继电器
void Relay::on() {
  _state = true;
  digitalWrite(_pin, _activeHigh ? HIGH : LOW);
}

// 关闭继电器
void Relay::off() {
  _state = false;
  digitalWrite(_pin, _activeHigh ? LOW : HIGH);
}

// 切换继电器状态
void Relay::toggle() {
  if (_state) {
    off();
  } else {
    on();
  }
}

// 获取当前状态
bool Relay::getState() {
  return _state;
}

// 设置继电器状态
void Relay::setState(bool state) {
  if (state) {
    on();
  } else {
    off();
  }
}