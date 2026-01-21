# Waymo
A high-level library for automation and macroing on Wayland

## Usage
```c
#include "waymo/events.h"
#include "waymo/actions.h"

int main() {
    // You can optionally create custom parameters here
    // Below are the defaults
    // const eloop_params params = {
    //  .kbd_layout = "us", .max_commands = 50, .action_cooldown_ms = 0
    // }
    // Create the event loop
    // You can optionally pass a pointer to params if you created custom params
    waymo_event_loop *loop = create_event_loop(NULL);
    // Check its status and handle errors if needed
    loop_status status = get_event_loop_status(loop);
    // Call one of the API functions to do something inside actions.h
    uint32_t ms = 10;
    type(loop, "Hello, world!", &ms);
    // Cleanup the event loop
    destroy_event_loop(loop);
}
```

## Building
To build this, first clone the repo and navigate to it
```bash
git clone https://github.com/ChaosTheChaotic/waymo.git && cd waymo
```
Then run the configure script and build it
Note that there are configure flags you could use
The configure script is just a bash script
```bash
# Optionally to install (if not on nixos; for installation on nixos, just add this repo as a flake input)
./configure -DDO_INSTALL=ON
```
```bash
cmake --build build
```
The output libraries are in the `lib/` directory

## Other missing features
- Screen capture/recording is missing as a feature due to the low support for the new protocol (ext-image-copy-capture-v1) across major Wayland programs, and my unwillingness to spend time implementing a deprecated protocol only to have to replace it soon. I would suggest just using grim as they have maintainers and a project that already works well
- Input reception is also not implemented due to the lack of support for reading global input with Wayland calling it a "security feature" and my unwillingness to force people to install extra programs (like xdg-desktop-portal) just to have it work

## Bindings for other languages
Currently the library has bindings to:
- Bash
- Golang
- Typescript/Javascript
- Python
- Rust

You install all of these through the langauges method of installing packages from github. Most of these have not been extensively tested. You may likely need dependencies for many of these as they must build the static library to link it.

## Architecture and workings
This library works by using a custom thread that runs an event loop. This is done due to automation often requiring spespfic ordered inputs and having these mixed up by things like race conditions would make this unreliable. The event loop uses an internal queue and mutex for managing commands. The event loop also has a linked list for pending events where it uses timerfd to schedule events without blocking the event loop.
