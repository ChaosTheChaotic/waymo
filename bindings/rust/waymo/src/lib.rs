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

pub struct WaymoEventLoop {
    inner: *mut wsys::waymo_event_loop,
}

impl WaymoEventLoop {
    pub fn new() -> Option<Self> {
        unsafe {
            let ptr = wsys::create_event_loop(ptr::null());
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
