use validation_macro::include_validated;

fn print(x: impl std::fmt::Display) {
    println!("{x}")
}

include_validated!("kek.rs");
