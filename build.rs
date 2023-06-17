use std::{
    env,
    path::{Path, PathBuf},
};

extern crate bindgen;

fn main() {
    //let _fftw_wisdom_dir = env::var("FFTW_WISDOM_DIR")
    //   .unwrap_or("".to_string());
    let fftw_install_dir = env::var("FFTW_INSTALL_DIR").unwrap_or("third-party/fftw3".to_string());

    println!("cargo:rustc-link-search={fftw_install_dir}");
    println!("cargo:rustc-link-search={fftw_install_dir}/lib");

    #[cfg(target_os = "windows")]
    println!("cargo:rustc-link-lib=libfftw3-3");
    #[cfg(target_os = "linux")]
    println!("cargo:rustc-link-lib=fftw3");

    //println!("cargo:rerun-if-changed=")

    let fftw3_header = if Path::new(format!("{fftw_install_dir}/fftw3.h").as_str()).exists() {
        format!("{fftw_install_dir}/fftw3.h")
    } else {
        format!("{fftw_install_dir}/include/fftw3.h")
    };

    let bindings = bindgen::Builder::default()
        .blocklist_function("fftwl_.*")
        .allowlist_function(
            "(fftw_malloc)|(fftw_plan_dft_1d)|(fftw_execute)|(fftw_destroy_plan)|(fftw_free)",
        )
        .allowlist_var("(FFTW_FORWARD)|(FFTW_BACKWARD)|(FFTW_ESTIMATE)")
        .header(fftw3_header)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("unable to generate fftw3 bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("couldn't write bindings.rs");
}
