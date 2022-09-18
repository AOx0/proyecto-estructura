use server::*;
use std::io::stdin;

fn main() {
    let server = start();

    stdin().read_line(&mut String::new()).unwrap();

    unsafe { stop(server) };
}
