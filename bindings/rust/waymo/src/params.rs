use std::ffi::CString;
use waymo_sys as wsys;

pub struct EloopParams {
    pub(crate) inner: *mut wsys::eloop_params,
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
        let inner = Box::into_raw(Box::new(wsys::eloop_params {
            max_commands: self.max_commands,
            kbd_layout: c_layout.into_raw(),
            action_cooldown_ms: self.action_cooldown_ms,
        }));

        EloopParams { inner }
    }
}
