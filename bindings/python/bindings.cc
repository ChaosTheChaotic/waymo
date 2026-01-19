#include "waymo/actions.h"
#include "waymo/btns.h"
#include "waymo/events.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

NB_MODULE(waymo_python, m) {
  nb::enum_<MBTNS>(m, "MBTNS")
      .value("LEFT", MBTN_LEFT)
      .value("RIGHT", MBTN_RIGHT)
      .value("MID", MBTN_MID);

  nb::enum_<loop_status>(m, "LoopStatus")
      .value("OK", STATUS_OK)
      .value("INIT_FAILED", STATUS_INIT_FAILED)
      .value("KBD_FAILED", STATUS_KBD_FAILED)
      .value("PTR_FAILED", STATUS_PTR_FAILED)
      .export_values();

  nb::class_<eloop_params>(m, "EloopParams")
      .def(nb::init<>())
      .def_rw("max_commands", &eloop_params::max_commands)
      .def_rw("kbd_layout", &eloop_params::kbd_layout)
      .def_rw("action_cooldown_ms", &eloop_params::action_cooldown_ms);

  m.def("create_event_loop", &create_event_loop, nb::arg("params") = nullptr,
        nb::rv_policy::reference, "Creates an event loop for sending inputs");

  m.def("destroy_event_loop", &destroy_event_loop, nb::arg("loop"),
        "Destroys the created event loop");

  m.def("get_event_loop_status", &get_event_loop_status, nb::arg("loop"),
        "Checks the initialization status of the event loop");

  m.def(
      "move_mouse",
      [](waymo_event_loop *loop, unsigned int x, unsigned int y,
         bool relative) { move_mouse(loop, x, y, relative); },
      nb::arg("loop"), nb::arg("x"), nb::arg("y"), nb::arg("relative"),
      "Moves the mouse around the screen");

  m.def(
      "click_mouse",
      [](waymo_event_loop *loop, MBTNS btn, unsigned int clicks,
         uint32_t hold_ms) { click_mouse(loop, btn, clicks, hold_ms); },
      nb::arg("loop"), nb::arg("btn"), nb::arg("clicks"), nb::arg("hold_ms"),
      "Clicks a mouse button");

  m.def(
      "press_mouse",
      [](waymo_event_loop *loop, MBTNS btn, bool down) {
        press_mouse(loop, btn, down);
      },
      nb::arg("loop"), nb::arg("btn"), nb::arg("down"),
      "Sets a mouse button to a specific state (up/down)");

  m.def(
      "press_key",
      [](waymo_event_loop *loop, char key, std::optional<uint32_t> interval_ms,
         bool down) {
        uint32_t *interval_ptr =
            interval_ms.has_value() ? &interval_ms.value() : nullptr;
        press_key(loop, key, interval_ptr, down);
      },
      nb::arg("loop"), nb::arg("key"), nb::arg("interval_ms") = nb::none(),
      nb::arg("down"), "Sets a keyboard key to a specific state");

  m.def(
      "hold_key",
      [](waymo_event_loop *loop, char key, std::optional<uint32_t> interval_ms,
         uint32_t hold_ms) {
        uint32_t *interval_ptr =
            interval_ms.has_value() ? &interval_ms.value() : nullptr;
        hold_key(loop, key, interval_ptr, hold_ms);
      },
      nb::arg("loop"), nb::arg("key"), nb::arg("interval_ms") = nb::none(),
      nb::arg("hold_ms"), "Holds a keyboard key for a specific duration");

  m.def(
      "type",
      [](waymo_event_loop *loop, const char *text,
         std::optional<uint32_t> interval_ms) {
        uint32_t *interval_ptr =
            interval_ms.has_value() ? &interval_ms.value() : nullptr;
        type(loop, text, interval_ptr);
      },
      nb::arg("loop"), nb::arg("text"), nb::arg("interval_ms") = nb::none(),
      "Types a string of text");
}
