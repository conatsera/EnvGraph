use std::{env, path::PathBuf};

extern crate bindgen;

fn main() {
    //let _fftw_wisdom_dir = env::var("FFTW_WISDOM_DIR")
    //   .unwrap_or("".to_string());
    let fftw_install_dir = env::var("FFTW_INSTALL_DIR").unwrap_or("third-party/fftw3".to_string());

    println!("cargo:rustc-link-search={fftw_install_dir}/lib");

    println!("cargo:rustc-link-lib=fftw3");

    //println!("cargo:rerun-if-changed=")

    let bindings = bindgen::Builder::default()
        .blocklist_function("fftwl_.*")
        .allowlist_function(
            "(fftw_malloc)|(fftw_plan_dft_1d)|(fftw_execute)|(fftw_destroy_plan)|(fftw_free)",
        )
        .allowlist_var("(FFTW_FORWARD)|(FFTW_BACKWARD)|(FFTW_ESTIMATE)")
        .header(format!("{fftw_install_dir}/include/fftw3.h").as_str())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("unable to generate fftw3 bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("couldn't write bindings.rs");
}
