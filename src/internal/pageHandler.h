#ifndef STEVESCH_PAGEHANDLER_INTERNAL_PAGEHANDLER_H_
#define STEVESCH_PAGEHANDLER_INTERNAL_PAGEHANDLER_H_
#include <Arduino.h>
#include <functional>

// An HTML page should reference this script and execute the following (or equivalent) script:
// <script>
//   var reflections = [ %REFL_LIST% ];
//   initPageHandler(reflections);
// </script>
//
// %REFL_LIST% is filled in by the pageHandler C++ code.  The JS code registers
// for events by those names, as well as any additional variables specified
// only in the javascript code.
// On the C++ side, VarReflector instances will automatically be communicated
// to/from the javascript and HTML (or manually, registerProcessor and registerReceiver
// can be called to provide C++ hooks for sending/receiving variables in
// custom code).

class AsyncWebServer; // from <ESPAsyncWebServer.h>

namespace stevesch
{
namespace PageHandler {
  void setup(AsyncWebServer& server);
  void loop();

  bool processAndSendRegisteredValue(const String& name); // send a single variable (if changed)
  void processAndSendUpdatedServerValues(); // send all changed variables

  typedef std::function<String (const String&)> procFn_t;
  typedef std::function<void (const String&, const String&)> recvFn_t;

  void registerProcessor(const String& var, procFn_t fn);
  void registerReceiver(const String& var, recvFn_t fn);

  // VarReflector replaces specified client HTML fields with the server variable value
  // and also receives events from client(s) to change the server variable value
  template <typename T> class VarReflector
  {
  public:
    typedef VarReflector<T> self_t;
    typedef std::function<void(self_t&, bool fromReceive)> callback_self_t;

    VarReflector(const char* varName, const T& v0) : mCurrentValue(v0)
    {
      registerAs(varName);
    }

    VarReflector(const char* varName, const T& v0, const callback_self_t& onChange) :
      mCurrentValue(v0), mOnChanged(onChange)
    {
      registerAs(varName);
    }

    String process(const String& varName) { return this->toString(mCurrentValue); }
    void receive(const String& varName, const String& value) {
      T tempValue;
      fromString(tempValue, value);
      if (mCurrentValue != tempValue) {
        std::swap(tempValue, mCurrentValue);
        notifyChanged(true);
      }
      //already done at lower-level: processAndSendRegisteredValue(varName); // reflect back to clients
    }

    // treat this object as a reference to its value
    operator T& () { return mCurrentValue; }

    VarReflector<T>& operator=(const T& src) {
      if (mCurrentValue != src) {
        mCurrentValue = src;
        notifyChanged(false);
      }
      return *this;
    }

    void registerAs(const char* varName) {
      registerProcessor(varName, [=](const String& varName){ return this->process(varName); });
      registerReceiver(varName, [=](const String& varName, const String& value){ return this->receive(varName, value); });
    }

    void setOnChanged(const callback_self_t& fn) { mOnChanged = fn; }

  protected:
    // convert value types to/from String
    String toString(const float& src) const { return String(mCurrentValue, 2); }
    String toString(const int& src) const { return String(mCurrentValue, 10); }
    String toString(const String& src) const { return String(src); }

    void fromString(float& dst, const String& src) const { dst = src.toFloat(); }
    void fromString(int& dst, const String& src) const { dst = src.toInt(); }
    void fromString(String& dst, const String& src) const { dst = src; }

    inline void notifyChanged(bool fromReceive) {
      if (mOnChanged) {
        this->mOnChanged(*this, fromReceive);
      }
    }

    T mCurrentValue;
    callback_self_t mOnChanged;
  };

}
}

#endif
