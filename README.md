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
    type(loop, "Hello, world!");
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
# Optionally to install (if not on nixos)
./configure -DDO_INSTALL=ON
```
```bash
cmake --build build
```
The output libraries are in the `lib/` directory

## Architecture and workings
This library works by using a custom thread that runs an event loop. This is done due to automation often requiring spespfic ordered inputs and having these mixed up by things like race conditions would make this unreliable. The event loop uses an internal queue and mutex for managing commands. The event loop also has a linked list for pending events where it uses timerfd to schedule events without blocking the event loop.
