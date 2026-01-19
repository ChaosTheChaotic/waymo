use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let proot = manifest_dir.ancestors().nth(3).unwrap().to_path_buf();

    pkg_config::probe_library("wayland-client").expect("libwayland-client not found");
    pkg_config::probe_library("xkbcommon").expect("libxkbcommon not found");

    // Build the static library
    let _dst = cmake::Config::new(&proot)
        .define("BUILD_STATIC", "ON")
        .define("BUILD_SHARED", "OFF")
        .define("DO_INSTALL", "OFF")
        .define("BUILD_TESTING", "OFF")
        // This prevents the 'make: *** No rule to make target install' error
        .no_build_target(true) 
        .build();

    println!(
        "cargo:rustc-link-search=native={}",
        proot.join("lib/static").display()
    );
    println!("cargo:rustc-link-lib=static=waymo");

    let header_path = proot.join("include/waymo/actions.h");
    let include_path = proot.join("include");

    println!("cargo:rerun-if-changed={}", header_path.display());

    let bindings = bindgen::Builder::default()
        .header(header_path.display().to_string())
        .clang_arg(format!("-I{}", include_path.display()))
        .clang_arg(format!("-I{}/build/generated/proto/include", _dst.display()))
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out.join("bindings.rs"))
        .expect("Failed to write bindings");
}
