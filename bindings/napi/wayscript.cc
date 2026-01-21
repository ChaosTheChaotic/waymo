#include "waymo/actions.h"
#include "waymo/events.h"
#include <napi.h>

class WaymoLoop : public Napi::ObjectWrap<WaymoLoop> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func =
        DefineClass(env, "WaymoLoop",
                    {
                        InstanceMethod("moveMouse", &WaymoLoop::MoveMouse),
                        InstanceMethod("clickMouse", &WaymoLoop::ClickMouse),
                        InstanceMethod("pressMouse", &WaymoLoop::PressMouse),
                        InstanceMethod("pressKey", &WaymoLoop::PressKey),
                        InstanceMethod("holdKey", &WaymoLoop::HoldKey),
                        InstanceMethod("type", &WaymoLoop::Type),
                    });
    exports.Set("WaymoLoop", func);

    Napi::Object mbtns = Napi::Object::New(env);
    mbtns.Set("MBTN_LEFT", Napi::Number::New(env, MBTN_LEFT));
    mbtns.Set("MBTN_RIGHT", Napi::Number::New(env, MBTN_RIGHT));
    mbtns.Set("MBTN_MID", Napi::Number::New(env, MBTN_MID));
    exports.Set("MBTNS", mbtns);

    return exports;
  }

  WaymoLoop(const Napi::CallbackInfo &info)
      : Napi::ObjectWrap<WaymoLoop>(info) {
    this->loop = create_event_loop(NULL);
  }

  ~WaymoLoop() {
    if (this->loop)
      destroy_event_loop(this->loop);
  }

private:
  waymo_event_loop *loop;

  // Extract optional interval pointers
  uint32_t *GetOptionalInterval(const Napi::Value &value, uint32_t *storage) {
    if (value.IsUndefined() || value.IsNull())
      return nullptr;
    *storage = value.As<Napi::Number>().Uint32Value();
    return storage;
  }

  Napi::Value MoveMouse(const Napi::CallbackInfo &info) {
    uint32_t x = info[0].As<Napi::Number>().Uint32Value();
    uint32_t y = info[1].As<Napi::Number>().Uint32Value();
    bool relative = info[2].As<Napi::Boolean>().Value();
    move_mouse(this->loop, x, y, relative);
    return info.Env().Undefined();
  }

  Napi::Value ClickMouse(const Napi::CallbackInfo &info) {
    MBTNS btn = static_cast<MBTNS>(info[0].As<Napi::Number>().Uint32Value());
    uint32_t clicks = info[1].As<Napi::Number>().Uint32Value();
    uint32_t hold_ms = info[2].As<Napi::Number>().Uint32Value();
    click_mouse(this->loop, btn, clicks, hold_ms);
    return info.Env().Undefined();
  }

  Napi::Value PressMouse(const Napi::CallbackInfo &info) {
    MBTNS btn = static_cast<MBTNS>(info[0].As<Napi::Number>().Uint32Value());
    bool down = info[1].As<Napi::Boolean>().Value();
    press_mouse(this->loop, btn, down);
    return info.Env().Undefined();
  }

  Napi::Value PressKey(const Napi::CallbackInfo &info) {
    char key = info[0].As<Napi::String>().Utf8Value()[0];
    bool down = info[1].As<Napi::Boolean>().Value();
    uint32_t interval_val;
    uint32_t *interval_ptr = GetOptionalInterval(info[2], &interval_val);
    press_key(this->loop, key, interval_ptr, down);
    return info.Env().Undefined();
  }

  Napi::Value HoldKey(const Napi::CallbackInfo &info) {
    char key = info[0].As<Napi::String>().Utf8Value()[0];
    uint32_t hold_ms = info[1].As<Napi::Number>().Uint32Value();
    uint32_t interval_val;
    uint32_t *interval_ptr = GetOptionalInterval(info[2], &interval_val);
    hold_key(this->loop, key, interval_ptr, hold_ms);
    return info.Env().Undefined();
  }

  Napi::Value Type(const Napi::CallbackInfo &info) {
    std::string text = info[0].As<Napi::String>().Utf8Value();
    uint32_t interval_val;
    uint32_t *interval_ptr = GetOptionalInterval(info[1], &interval_val);
    type(this->loop, text.c_str(), interval_ptr);
    return info.Env().Undefined();
  }
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return WaymoLoop::Init(env, exports);
}

NODE_API_MODULE(waymo_js, InitAll)
