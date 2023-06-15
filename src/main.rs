mod app;
mod page;
mod signal;

fn main() -> ! {
    let app = app::App::new();

    app.run()
}
