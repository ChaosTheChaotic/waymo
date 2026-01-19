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
