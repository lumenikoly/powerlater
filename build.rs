fn main() {
    slint_build::compile("ui/app.slint").expect("failed to compile Slint UI");

    #[cfg(target_os = "macos")]
    {
        println!("cargo:rustc-link-lib=framework=ApplicationServices");
        println!("cargo:rustc-link-lib=framework=Carbon");
    }

    #[cfg(windows)]
    {
        let mut resource = winresource::WindowsResource::new();
        resource.set_icon("packaging/power-timer.ico");
        resource.set_manifest_file("packaging/windows.manifest");
        resource.set("FileDescription", "Power Timer");
        resource.set("ProductName", "Power Timer");
        resource.set("FileVersion", env!("CARGO_PKG_VERSION"));
        resource
            .compile()
            .expect("failed to compile Windows resources");
    }
}
