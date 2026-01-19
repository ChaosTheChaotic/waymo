use std::env;
use std::path::PathBuf;

fn main() {
    let proot = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .ancestors()
        .nth(3)
        .unwrap()
        .to_path_buf();

    // Link libwaymo statically from the lib/static directory
    println!(
        "cargo:rustc-link-search=native={}",
        proot.join("lib/static").display()
    );
    println!("cargo:rustc-link-lib=static=waymo");

    println!("cargo:rustc-link-lib=dylib=xkbcommon");
    println!("cargo:rustc-link-lib=dylib=wayland-client");

    let bindings = bindgen::Builder::default()
        .header(proot.join("include/waymo/actions.h").display().to_string())
        .clang_arg(format!("-I{}", proot.join("include").display()))
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out.join("bindings.rs"))
        .expect("Failed to write bindings");
}
