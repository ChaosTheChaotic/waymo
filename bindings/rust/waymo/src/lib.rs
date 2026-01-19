use libc::{c_char, close, eventfd, read, EFD_CLOEXEC, EINTR};
use std::ffi::CString;
use std::ptr;
use waymo_sys as wsys;

#[derive(Debug, Clone, Copy)]
pub enum MouseButton {
    Left,
    Right,
    Middle,
}

impl From<MouseButton> for wsys::MBTNS {
    fn from(btn: MouseButton) -> Self {
        match btn {
            MouseButton::Left => wsys::MBTNS_MBTN_LEFT,
            MouseButton::Right => wsys::MBTNS_MBTN_RIGHT,
            MouseButton::Middle => wsys::MBTNS_MBTN_MID,
        }
    }
}

pub struct EloopParams {
    inner: *mut wsys::eloop_params,
}

impl Default for EloopParams {
    fn default() -> Self {
        EloopParamsBuilder::new().build()
    }
}

impl EloopParams {
    pub fn new() -> EloopParamsBuilder {
        EloopParamsBuilder::new()
    }
}

impl Drop for EloopParams {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe {
                let params = Box::from_raw(self.inner);
                // We must also reconstruct the CString to free the string buffer
                if !params.kbd_layout.is_null() {
                    let _ = CString::from_raw(params.kbd_layout as *mut _);
                }
            }
        }
    }
}

pub struct EloopParamsBuilder {
    max_commands: u32,
    kbd_layout: String,
    action_cooldown_ms: u32,
}

impl EloopParamsBuilder {
    pub fn new() -> Self {
        Self {
            max_commands: 50,
            kbd_layout: String::from("us"),
            action_cooldown_ms: 0,
        }
    }

    pub fn max_commands(mut self, count: u32) -> Self {
        self.max_commands = count;
        self
    }

    pub fn kbd_layout(mut self, layout: &str) -> Self {
        self.kbd_layout = layout.to_string();
        self
    }

    pub fn action_cooldown_ms(mut self, ms: u32) -> Self {
        self.action_cooldown_ms = ms;
        self
    }

    pub fn build(self) -> EloopParams {
        let c_layout = CString::new(self.kbd_layout).unwrap();
        
        // Allocate the C struct on the heap
        let inner = Box::into_raw(Box::new(wsys::eloop_params {
            max_commands: self.max_commands,
            // Note: into_raw() transfers ownership to the C pointer
            kbd_layout: c_layout.into_raw(), 
            action_cooldown_ms: self.action_cooldown_ms,
        }));

        EloopParams { inner }
    }
}


pub struct WaymoEventLoop {
    inner: *mut wsys::waymo_event_loop,
}

impl WaymoEventLoop {
    pub fn new(params: Option<EloopParams>) -> Option<Self> {
        unsafe {
            let p = match params {
                Some(v) => v.inner,
                _ => ptr::null()
            };
            let ptr = wsys::create_event_loop(p);
            if ptr.is_null() {
                return None;
            }

            if wsys::get_event_loop_status(ptr) != wsys::loop_status_STATUS_OK {
                wsys::destroy_event_loop(ptr);
                return None;
            }
            Some(Self { inner: ptr })
        }
    }

    // Emulate the WAIT_COMPLETE macro logic
    fn wait_complete<F>(&self, f: F)
    where
        F: FnOnce(i32),
    {
        unsafe {
            let efd = eventfd(0, EFD_CLOEXEC);
            if efd != -1 {
                f(efd);
                let mut res: u64 = 0;
                // Block until the event loop writes to the eventfd
                while read(efd, &mut res as *mut _ as *mut _, 8) == -1 {
                    if *libc::__errno_location() != EINTR {
                        break;
                    }
                }
                close(efd);
            }
        }
    }

    pub fn move_mouse(&self, x: u32, y: u32, relative: bool) {
        self.wait_complete(|efd| unsafe {
            let cmd = wsys::_create_mouse_move_cmd(x, y, relative);
            wsys::_send_command(self.inner, cmd, efd);
        });
    }

    pub fn click_mouse(&self, btn: MouseButton, clicks: u32, hold_ms: u32) {
        self.wait_complete(|efd| unsafe {
            let cmd = wsys::_create_mouse_click_cmd(btn.into(), clicks, hold_ms);
            wsys::_send_command(self.inner, cmd, efd);
        });
    }

    pub fn press_mouse(&self, btn: MouseButton, down: bool) {
        self.wait_complete(|efd| unsafe {
            let cmd = wsys::_create_mouse_button_cmd(btn.into(), down);
            wsys::_send_command(self.inner, cmd, efd);
        });
    }

    pub fn press_key(&self, key: char, interval_ms: Option<&mut u32>, down: bool) {
        unsafe {
            let interval_ptr = interval_ms.map_or(ptr::null_mut(), |v| v);
            let cmd = wsys::_create_keyboard_key_cmd_b(key as c_char, interval_ptr, down);
            wsys::_send_command(self.inner, cmd, -1);
        }
    }

    pub fn hold_key(&self, key: char, interval_ms: Option<&mut u32>, hold_ms: u32) {
        unsafe {
            let interval_ptr = interval_ms.map_or(ptr::null_mut(), |v| v);
            let cmd = wsys::_create_keyboard_key_cmd_uintt(key as c_char, interval_ptr, hold_ms);
            wsys::_send_command(self.inner, cmd, -1);
        }
    }

    pub fn type_text(&self, text: &str, interval_ms: Option<&mut u32>) {
        if let Ok(c_str) = CString::new(text) {
            self.wait_complete(|efd| unsafe {
                let interval_ptr = interval_ms.map_or(ptr::null_mut(), |v| v);
                let cmd = wsys::_create_keyboard_type_cmd(c_str.as_ptr(), interval_ptr);
                wsys::_send_command(self.inner, cmd, efd);
            });
        }
    }
}

impl Drop for WaymoEventLoop {
    fn drop(&mut self) {
        unsafe {
            wsys::destroy_event_loop(self.inner);
        }
    }
}
